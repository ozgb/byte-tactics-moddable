"""Egress proxy for the clean-room sandbox: CONNECT to an allowlist of host:port pairs, logged.

    uv run tools/cleanroom/proxy.py --allow FILE --log FILE [--listen HOST:PORT | unix:PATH]
    uv run tools/cleanroom/proxy.py --check-log LOG --allow FILE

Only `CONNECT host:port` is served. Any other method gets 403 and a malformed request line gets 400.
The allowlist file holds one `host[:port]` per line (port 443 by default), with `#` lines and blank
lines ignored. Hosts match case-insensitively and exactly: there is no suffix or wildcard match, and
an IP literal is refused unless it is listed. An allowed host is resolved on the host side, and the
bytes are spliced both ways until either side closes. Every request is one JSON line in the log,
and a tunnel adds a second line with `event: "close"` and the bytes moved. The request header is
capped at 8 KiB and must arrive within 10 seconds.

A host name that resolves only to loopback, private or link-local addresses gets a 502, so an
allowed name can't be pointed back at the host or its network. Addresses written in the allowlist
as IP literals are not checked. `--listen unix:PATH` serves on a Unix socket, which is how the
session VM reaches the proxy with no network device at all. SIGHUP reopens the log file, so a
launcher can rename it and start a fresh one.

A tunnel to a host name must open with a TLS ClientHello whose server name is that host (case
folded), else the tunnel is closed and an `event: "sni_refused"` line is logged. Tunnels to
IP literals in the allowlist are not checked.

--check-log exits 1 and prints each logged request whose target is not on the allowlist, else
prints a summary and exits 0.
"""

import argparse
import asyncio
import ipaddress
import json
import os
import signal
import socket
import sys
from datetime import datetime, timezone
from pathlib import Path

HEADER_CAP = 8192
HEADER_TIMEOUT = 10.0
HELLO_CAP = 16384
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


def is_ip_literal(host):
    try:
        ipaddress.ip_address(host)
    except ValueError:
        return False
    return True


async def resolve_public(host, port):
    """Return the global addresses of a name, or an empty list."""
    infos = await asyncio.get_running_loop().getaddrinfo(host, port, type=socket.SOCK_STREAM)
    return [i[4][0] for i in infos if ipaddress.ip_address(i[4][0].split("%")[0]).is_global]


def now():
    return datetime.now(timezone.utc).isoformat()


class Proxy:
    def __init__(self, allowed, log_path):
        self.allowed = allowed
        self.log_path = log_path
        self.log = open(log_path, "a", encoding="utf-8")

    def reopen(self):
        self.log.close()
        self.log = open(self.log_path, "a", encoding="utf-8")

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
        client = f"{peer[0]}:{peer[1]}" if isinstance(peer, tuple) else "unix"
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
                if is_ip_literal(pair[0]):
                    addrs = [pair[0]]
                else:
                    addrs = await asyncio.wait_for(resolve_public(*pair), HEADER_TIMEOUT)
                if not addrs:
                    raise OSError("no public address")
                up_reader, up_writer = await asyncio.wait_for(asyncio.open_connection(addrs[0], pair[1]), HEADER_TIMEOUT)
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
            if not is_ip_literal(pair[0]):
                try:
                    hello = await read_client_hello(reader)
                except (asyncio.TimeoutError, asyncio.IncompleteReadError):
                    hello = b""
                sni = parse_sni(hello)
                if sni != pair[0]:
                    self.write({"time": now(), "client": client, "event": "sni_refused", "target": target, "sni": sni})
                    up_writer.close()
                    return
                counts["up"] += len(hello)
                up_writer.write(hello)
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


def parse_sni(record):
    """Return the lowercase server name of a TLS ClientHello record, or None."""
    try:
        if record[0] != 0x16 or record[5] != 1:
            return None
        pos = 5 + 4 + 2 + 32
        pos += 1 + record[pos]
        pos += 2 + int.from_bytes(record[pos : pos + 2], "big")
        pos += 1 + record[pos]
        end = pos + 2 + int.from_bytes(record[pos : pos + 2], "big")
        pos += 2
        while pos + 4 <= end:
            kind = int.from_bytes(record[pos : pos + 2], "big")
            size = int.from_bytes(record[pos + 2 : pos + 4], "big")
            body = record[pos + 4 : pos + 4 + size]
            pos += 4 + size
            if kind == 0:
                if body[2] != 0:
                    return None
                length = int.from_bytes(body[3:5], "big")
                return body[5 : 5 + length].decode("ascii").lower()
    except (IndexError, UnicodeDecodeError):
        return None
    return None


async def read_client_hello(reader):
    """Read the first TLS record from the client and return it."""
    head = await asyncio.wait_for(reader.readexactly(5), HEADER_TIMEOUT)
    length = int.from_bytes(head[3:5], "big")
    if head[0] != 0x16 or length > HELLO_CAP:
        return head
    return head + await asyncio.wait_for(reader.readexactly(length), HEADER_TIMEOUT)


async def start(allowed, log_path, host, port=None):
    """Start the proxy on host:port, or on the Unix socket path when port is None."""
    proxy = Proxy(allowed, log_path)
    if port is None:
        if os.path.exists(host):
            os.remove(host)
        server = await asyncio.start_unix_server(proxy.handle, host, limit=HEADER_CAP)
        os.chmod(host, 0o666)
    else:
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
        if record.get("event"):
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
    if args.listen.startswith("unix:"):
        host, port = args.listen[5:], None
    else:
        host, _, port = args.listen.rpartition(":")
        port = int(port)
    server, proxy = await start(load_allowlist(args.allow), args.log, host, port)
    asyncio.get_running_loop().add_signal_handler(signal.SIGHUP, proxy.reopen)
    async with server:
        await server.serve_forever()


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--allow", required=True, help="allowlist file, one host[:port] per line")
    ap.add_argument("--log", help="JSON lines log, appended")
    ap.add_argument("--listen", default="127.0.0.1:3128", help="HOST:PORT or unix:PATH (default 127.0.0.1:3128)")
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
