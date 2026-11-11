"""Send the human a push notification through ntfy.sh (docs/team.md, "Human review").

    uv run tools/notify.py --topic                      # print the topic and how to subscribe
    uv run tools/notify.py --title "PR #938" --click URL "Ready for your review: ..."

The topic is read from NTFY_TOPIC at the root of the main clone, shared by its
worktrees and ignored by git. Anyone who knows the topic can read and post to
it, so it is never committed or written to GitHub; when the file is missing, a
random topic is generated and saved there.
"""

import argparse
import secrets
import subprocess
import sys
import urllib.request
from pathlib import Path

SERVER = "https://ntfy.sh"
FILE = "NTFY_TOPIC"


def clone_root(cwd=None) -> Path:
    """The main clone's root, also when run from one of its worktrees."""
    common = subprocess.run(
        ["git", "rev-parse", "--path-format=absolute", "--git-common-dir"],
        cwd=cwd, capture_output=True, text=True, check=True,
    ).stdout.strip()
    return Path(common).parent


def topic(root: Path) -> tuple[str, bool]:
    """The topic in root/NTFY_TOPIC, and whether it was generated just now."""
    path = root / FILE
    if path.exists():
        name = path.read_text().strip()
        if name:
            return name, False
    name = "bt-" + secrets.token_urlsafe(24)
    path.write_text(name + "\n")
    path.chmod(0o600)
    return name, True


def request(name: str, message: str, title=None, click=None, priority=None, tags=None):
    """The POST that publishes message to the topic."""
    headers = {}
    if title:
        headers["Title"] = title
    if click:
        headers["Click"] = click
    if priority:
        headers["Priority"] = str(priority)
    if tags:
        headers["Tags"] = tags
    return urllib.request.Request(
        f"{SERVER}/{name}", data=message.encode(), headers=headers, method="POST"
    )


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("message", nargs="?")
    ap.add_argument("--topic", action="store_true", help="print the topic and subscribe URL")
    ap.add_argument("--title")
    ap.add_argument("--click", help="URL opened when the notification is tapped")
    ap.add_argument("--priority", type=int, choices=range(1, 6))
    ap.add_argument("--tags", help="comma-separated ntfy tags or emoji names")
    args = ap.parse_args(argv)

    root = clone_root()
    name, created = topic(root)
    if created:
        print(f"Generated a new topic in {root / FILE}.", file=sys.stderr)
    if args.topic:
        print(name)
        print(f"Subscribe: {SERVER}/{name} (web), or add the topic in the ntfy app.")
        return 0
    if not args.message:
        ap.error("a message is required unless --topic is given")
    req = request(name, args.message, args.title, args.click, args.priority, args.tags)
    with urllib.request.urlopen(req, timeout=30) as resp:
        if resp.status != 200:
            print(f"ntfy answered {resp.status}", file=sys.stderr)
            return 1
    if created:
        print(f"Tell the human to subscribe: {SERVER}/{name}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
