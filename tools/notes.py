"""Run one Redis command on the leads' and maintainer's note streams, over Upstash's REST API.

    uv run tools/notes.py GET bt:cursor:maintainer
    uv run tools/notes.py XRANGE bt:to-lead '(1795711489734-0' +
    uv run tools/notes.py XADD bt:to-maintainer '*' from lead type note ref '#12' body 'Lead: ...'

The credentials are UPSTASH_REDIS_REST_URL and UPSTASH_REDIS_REST_TOKEN, from the environment or else from
~/.local/share/bt-upstash/rest.env (docs/team.md, the Redis paragraph). A string or number result prints
as is, nil prints nothing, and anything else prints as one JSON line. Exits 2 when the database can't be
reached, so a caller can tell that apart from an empty stream.
"""

import json
import os
import sys
import urllib.error
import urllib.request
from pathlib import Path

ENV_FILE = Path.home() / ".local/share/bt-upstash/rest.env"


def credentials() -> tuple[str, str]:
    values = {}
    if ENV_FILE.exists():
        for line in ENV_FILE.read_text().splitlines():
            key, sep, value = line.partition("=")
            if sep:
                values[key.strip()] = value.strip()
    values.update({k: v for k, v in os.environ.items() if k.startswith("UPSTASH_REDIS_REST_")})
    try:
        return values["UPSTASH_REDIS_REST_URL"], values["UPSTASH_REDIS_REST_TOKEN"]
    except KeyError as missing:
        sys.exit(f"notes.py: {missing.args[0]} is not set and not in {ENV_FILE}")


def run(command: list[str]) -> object:
    url, token = credentials()
    request = urllib.request.Request(
        url,
        data=json.dumps(command).encode(),
        headers={"Authorization": f"Bearer {token}", "Content-Type": "application/json"},
    )
    try:
        with urllib.request.urlopen(request, timeout=20) as response:
            reply = json.load(response)
    except urllib.error.HTTPError as error:
        reply = json.loads(error.read() or b"{}")
        sys.exit(f"notes.py: {reply.get('error', error)}")
    except (urllib.error.URLError, TimeoutError) as error:
        print(f"notes.py: can't reach {url}: {error}", file=sys.stderr)
        sys.exit(2)
    if "error" in reply:
        sys.exit(f"notes.py: {reply['error']}")
    return reply["result"]


def main() -> None:
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    result = run(sys.argv[1:])
    if result is None:
        return
    print(result if isinstance(result, (str, int)) else json.dumps(result))


if __name__ == "__main__":
    main()
