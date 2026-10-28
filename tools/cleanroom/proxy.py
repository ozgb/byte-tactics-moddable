"""Egress proxy for the clean-room sandbox: CONNECT to an allowlist of host:port pairs, logged.

    uv run tools/cleanroom/proxy.py --allow FILE --log FILE [--listen HOST:PORT]
    uv run tools/cleanroom/proxy.py --check-log LOG --allow FILE

Only `CONNECT host:port` is served. Any other method gets 403 and a malformed request line gets 400.
The allowlist file holds one `host[:port]` per line (port 443 by default), with `#` lines and blank
lines ignored. Hosts match case-insensitively and exactly: there is no suffix or wildcard match, and
an IP literal is refused unless it is listed. An allowed host is resolved on the host side, and the
bytes are spliced both ways until either side closes. Every request is one JSON line in the log,
and a tunnel adds a second line with `event: "close"` and the bytes moved. The request header is
capped at 8 KiB and must arrive within 10 seconds.

--check-log exits 1 and prints each logged request whose target is not on the allowlist, else
prints a summary and exits 0.
"""

import argparse
import asyncio
import json
import sys
from datetime import datetime, timezone
from pathlib import Path

HEADER_CAP = 8192
HEADER_TIMEOUT = 10.0
DEFAULT_PORT = 443


def split_target(text):
    """Split `host[:port]` into a lowercase host and an int port, or None if malformed."""
    text = text.strip()
    if not text or any(c.isspace() for c in text):
        return None
    if text.startswith("["):
        host, bracket, rest = text[1:].partition("]")
        if not bracket or (rest and not rest.startswith(":")):
            return None
        port_text = rest[1:]
        has_port = bool(rest)
    elif text.count(":") > 1:
        return None
    else:
        host, colon, port_text = text.partition(":")
        has_port = bool(colon)
    if not host:
        return None
    if not has_port:
        return host.lower(), DEFAULT_PORT
    if not port_text.isdigit() or not 0 < int(port_text) < 65536:
        return None
    return host.lower(), int(port_text)


def parse_allowlist(text):
    """Return the set of (host, port) pairs listed in the allowlist text."""
    allowed = set()
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        pair = split_target(line)
        if pair is None:
            raise ValueError(f"bad allowlist line: {line!r}")
        allowed.add(pair)
    return allowed


def load_allowlist(path):
    return parse_allowlist(Path(path).read_text())


def now():
    return datetime.now(timezone.utc).isoformat()


class Proxy:
    def __init__(self, allowed, log_path):
        self.allowed = allowed
        self.log = open(log_path, "a", encoding="utf-8")

    def write(self, record):
        self.log.write(json.dumps(record) + "\n")
        self.log.flush()

    async def reply(self, writer, status, reason):
        writer.write(f"HTTP/1.1 {status} {reason}\r\nConnection: close\r\nContent-Length: 0\r\n\r\n".encode())
        try:
            await writer.drain()
        except ConnectionError:
            pass

    async def read_header(self, reader):
        return await asyncio.wait_for(reader.readuntil(b"\r\n\r\n"), HEADER_TIMEOUT)

    async def handle(self, reader, writer):
        peer = writer.get_extra_info("peername")
        client = f"{peer[0]}:{peer[1]}" if peer else "?"
        record = {"time": now(), "client": client, "method": "", "target": "", "allowed": False, "status": 0}
        try:
            try:
                header = await self.read_header(reader)
            except (asyncio.TimeoutError, asyncio.LimitOverrunError, asyncio.IncompleteReadError, ValueError):
                record["status"] = 400
                self.write(record)
                await self.reply(writer, 400, "Bad Request")
                return
            line = header.split(b"\r\n", 1)[0].decode("latin-1")
            parts = line.split(" ")
            if len(parts) != 3 or not parts[2].startswith("HTTP/") or not parts[0].isalpha():
                record["status"] = 400
                self.write(record)
                await self.reply(writer, 400, "Bad Request")
                return
            method, target = parts[0], parts[1]
            record["method"], record["target"] = method, target
            pair = split_target(target) if method == "CONNECT" else None
            if method == "CONNECT" and pair is None:
                record["status"] = 400
                self.write(record)
                await self.reply(writer, 400, "Bad Request")
                return
            if pair is None or pair not in self.allowed:
                record["status"] = 403
                self.write(record)
                await self.reply(writer, 403, "Forbidden")
                return
            try:
                up_reader, up_writer = await asyncio.wait_for(asyncio.open_connection(*pair), HEADER_TIMEOUT)
            except (OSError, asyncio.TimeoutError):
                record["allowed"] = True
                record["status"] = 502
                self.write(record)
                await self.reply(writer, 502, "Bad Gateway")
                return
            record["allowed"] = True
            record["status"] = 200
            self.write(record)
            writer.write(b"HTTP/1.1 200 Connection established\r\n\r\n")
            await writer.drain()
            counts = {"up": 0, "down": 0}
            await asyncio.gather(
                self.pipe(reader, up_writer, counts, "up"),
                self.pipe(up_reader, writer, counts, "down"),
            )
            self.write(
                {
                    "time": now(),
                    "client": client,
                    "event": "close",
                    "target": target,
                    "bytes_up": counts["up"],
                    "bytes_down": counts["down"],
                }
            )
        except ConnectionError:
            pass
        finally:
            writer.close()

    async def pipe(self, src, dst, counts, key):
        try:
            while True:
                chunk = await src.read(65536)
                if not chunk:
                    break
                counts[key] += len(chunk)
                dst.write(chunk)
                await dst.drain()
        except ConnectionError:
            pass
        finally:
            try:
                dst.close()
            except OSError:
                pass


async def start(allowed, log_path, host, port):
    """Start the proxy and return (server, proxy)."""
    proxy = Proxy(allowed, log_path)
    server = await asyncio.start_server(proxy.handle, host, port, limit=HEADER_CAP)
    return server, proxy


def check_log(log_path, allow_path):
    """Print each logged request off the allowlist; return the exit code."""
    allowed = load_allowlist(allow_path)
    total = 0
    bad = 0
    for line in Path(log_path).read_text().splitlines():
        if not line.strip():
            continue
        record = json.loads(line)
        if record.get("event") == "close":
            continue
        total += 1
        pair = split_target(record.get("target", "")) if record.get("method") == "CONNECT" else None
        if pair is None or pair not in allowed:
            bad += 1
            print(line)
    if bad:
        print(f"proxy log: {bad} of {total} requests are off the allowlist")
        return 1
    print(f"proxy log: {total} requests, all on the allowlist")
    return 0


async def serve(args):
    host, _, port = args.listen.rpartition(":")
    server, _ = await start(load_allowlist(args.allow), args.log, host, int(port))
    async with server:
        await server.serve_forever()


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--allow", required=True, help="allowlist file, one host[:port] per line")
    ap.add_argument("--log", help="JSON lines log, appended")
    ap.add_argument("--listen", default="127.0.0.1:3128", help="HOST:PORT (default 127.0.0.1:3128)")
    ap.add_argument("--check-log", metavar="LOG", help="check a log against --allow and exit")
    args = ap.parse_args(argv)
    if args.check_log:
        return check_log(args.check_log, args.allow)
    if not args.log:
        ap.error("--log is required")
    try:
        asyncio.run(serve(args))
    except KeyboardInterrupt:
        pass
    return 0


if __name__ == "__main__":
    sys.exit(main())
