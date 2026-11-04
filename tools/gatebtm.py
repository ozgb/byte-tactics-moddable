"""Judge the open pull requests of btm and post the verdicts (docs/cleanroom.md, "The btm gate agent").

    uv run tools/gatebtm.py [--repo ozgb/btm] [--game-dir DIR] [--full] [--dry-run] [--state FILE] [--token-file FILE]
    uv run tools/gatebtm.py --local MODULE.wasm --subsystem NAME [--game-dir DIR] [--full]

One pass over the open pull requests. It reads, per pull request, the number, the author's login, the label names and the head
commit, and nothing else: never the title, description, comments, reviews, files or diffs. A pull request is eligible when its
author has write, maintain or admin permission, or it has the label `gate`; an ineligible one is skipped without being read
further. The module is the `module` artifact of the pull request's CI run for the head commit (exactly `module.wasm` and
`subsystem`), downloaded into a fresh directory, judged by `tools/oraclegate.py`, and the summary of its `result.json` is posted
as a comment with the scoped btm token (the file `tools/cleanroom/broker.py` reads, never the host's `gh` login). The judged
(pull request, head commit) pairs are kept in a state file, so a pair is judged once and no comment is read.

`--local` judges a local module by the same path from the download on and prints the summary; it never posts.
`--dry-run` lists, checks eligibility, downloads and judges, but posts and records nothing.

Exit status: 0 when the pass finished (or `--local` judged the module equal), 1 when `--local` judged it not equal or a pull request
failed to be judged or posted, 2 when the arguments or the token were wrong.
"""

import argparse
import json
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.error
import urllib.parse
import urllib.request
import zipfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "cleanroom"))

import broker

ROOT = Path(__file__).resolve().parent.parent
ORACLEGATE = ROOT / "tools" / "oraclegate.py"
DEFAULT_STATE = Path.home() / ".cache" / "bt-gate-btm" / "judged.json"
WRITE_PERMISSIONS = ("write", "maintain", "admin")
GATE_LABEL = "gate"
ARTIFACT_NAME = "module"
ARTIFACT_FILES = ("module.wasm", "subsystem")
MODULE_LIMIT = 16 * 1024 * 1024
SUBSYSTEM_LIMIT = 64
ZIP_LIMIT = 17 * 1024 * 1024
JUDGE_TIMEOUT = 4 * 3600
LISTED_LIMIT = 20
NAME = re.compile(r"[A-Za-z0-9_.#\[\]/-]{1,200}$")
LOGIN = re.compile(r"[A-Za-z0-9][A-Za-z0-9-]{0,38}$")
SHA = re.compile(r"[0-9a-f]{40}$")


class Error(Exception):
    pass


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, *args, **kwargs):
        return None


class Api:
    """The GitHub REST API of one repository, called with one token and nothing else."""

    def __init__(self, base: str, repo: str, token: str):
        self.base, self.repo, self.token = base.rstrip("/"), repo, token

    def request(self, method: str, path: str, body: dict | None = None, redirects: bool = True):
        data = None if body is None else json.dumps(body).encode()
        req = urllib.request.Request(self.base + path, data=data, method=method, headers={
            "Authorization": f"Bearer {self.token}", "Accept": "application/vnd.github+json",
            "X-GitHub-Api-Version": "2022-11-28", "User-Agent": "bt-gate-btm", "Content-Type": "application/json"})
        opener = urllib.request.build_opener() if redirects else urllib.request.build_opener(NoRedirect)
        try:
            return opener.open(req, timeout=60)
        except urllib.error.HTTPError as err:
            if not redirects and err.code in (301, 302, 303, 307, 308):
                return err
            raise Error(f"{method} {path.split('?')[0]}: HTTP {err.code}") from None
        except OSError as err:
            raise Error(f"{method} {path.split('?')[0]}: {err}") from None

    def json(self, path: str):
        with self.request("GET", path) as r:
            return json.loads(r.read())

    def download(self, path: str, limit: int) -> bytes:
        """The body at `path`, following the one redirect to the storage host without the token."""
        r = self.request("GET", path, redirects=False)
        location = r.headers.get("Location") if r.status in (301, 302, 303, 307, 308) else None
        r.close()
        if location is None:
            raise Error(f"GET {path}: no redirect to the artifact")
        try:
            with urllib.request.urlopen(urllib.request.Request(location, headers={"User-Agent": "bt-gate-btm"}), timeout=300) as f:
                data = f.read(limit + 1)
        except OSError as err:
            raise Error(f"the artifact download failed: {err}") from None
        if len(data) > limit:
            raise Error("the artifact is larger than the limit")
        return data


def parse_pulls(page: list) -> list[dict]:
    """Four fields of each pull request in a list page, the rest dropped as it is parsed."""
    pulls = []
    for item in page:
        number, user, labels, head = item.get("number"), item.get("user") or {}, item.get("labels") or [], item.get("head") or {}
        login, sha = user.get("login"), head.get("sha")
        if not (isinstance(number, int) and isinstance(login, str) and LOGIN.match(login) and isinstance(sha, str) and SHA.match(sha)):
            continue
        pulls.append({"number": number, "login": login, "labels": sorted(str(x.get("name")) for x in labels if isinstance(x, dict)), "sha": sha})
    return pulls


def open_pulls(api: Api) -> list[dict]:
    pulls, page = [], 1
    while True:
        got = api.json(f"/repos/{api.repo}/pulls?state=open&per_page=100&page={page}")
        pulls += parse_pulls(got)
        if len(got) < 100:
            return pulls
        page += 1


def eligible(api: Api, pull: dict) -> bool:
    if GATE_LABEL in pull["labels"]:
        return True
    try:
        permission = api.json(f"/repos/{api.repo}/collaborators/{urllib.parse.quote(pull['login'])}/permission").get("permission")
    except Error:
        return False
    return permission in WRITE_PERMISSIONS


def known_subsystems() -> list[str]:
    return [m["name"] for m in json.loads((ROOT / "wasm" / "modules.json").read_text())["modules"]]


def unpack(blob: bytes, directory: Path, known: list[str]) -> tuple[Path, str]:
    """Check the zip holds exactly `module.wasm` and `subsystem`, both within their limits and the subsystem a known one; write them."""
    scratch = directory / "zip"
    scratch.write_bytes(blob)
    try:
        with zipfile.ZipFile(scratch) as z:
            infos = z.infolist()
            if sorted(i.filename for i in infos) != sorted(ARTIFACT_FILES):
                raise Error("the artifact does not hold exactly module.wasm and subsystem")
            sizes = {i.filename: i.file_size for i in infos}
            if sizes["module.wasm"] > MODULE_LIMIT or sizes["subsystem"] > SUBSYSTEM_LIMIT:
                raise Error("the artifact has a file over its size limit")
            wasm, name = z.read("module.wasm"), z.read("subsystem")
    except zipfile.BadZipFile:
        raise Error("the artifact is not a zip file") from None
    subsystem = name.decode("utf-8", errors="replace").strip()
    if subsystem not in known:
        raise Error("the artifact names an unknown subsystem")
    module = directory / "module.wasm"
    module.write_bytes(wasm)
    scratch.unlink()
    return module, subsystem


def find_artifact(api: Api, sha: str) -> int | None:
    """The id of the `module` artifact of the newest completed pull request run for the head commit, or None."""
    runs = api.json(f"/repos/{api.repo}/actions/runs?head_sha={sha}&event=pull_request&per_page=20").get("workflow_runs") or []
    for run in sorted(runs, key=lambda r: r.get("id", 0), reverse=True):
        if run.get("status") != "completed" or run.get("head_sha") != sha or not isinstance(run.get("id"), int):
            continue
        listing = api.json(f"/repos/{api.repo}/actions/runs/{run['id']}/artifacts?per_page=100").get("artifacts") or []
        for artifact in listing:
            if artifact.get("name") == ARTIFACT_NAME and not artifact.get("expired") and isinstance(artifact.get("id"), int):
                return artifact["id"]
    return None


def run_oraclegate(module: Path, subsystem: str, out: Path, game_dir: str | None, full: bool, judge: Path | None = None) -> dict:
    cmd = [sys.executable, str(judge or ORACLEGATE), str(module), "--subsystem", subsystem, "--out", str(out)]
    if game_dir:
        cmd += ["--game-dir", game_dir]
    if full:
        cmd.append("--full")
    log = out / "gatebtm.log"
    out.mkdir(parents=True, exist_ok=True)
    with log.open("w") as f:
        r = subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, text=True, cwd=ROOT, timeout=JUDGE_TIMEOUT)
    result = out / "result.json"
    if r.returncode not in (0, 1, 2) or not result.is_file():
        raise Error(f"oraclegate failed (exit {r.returncode}, log {log})")
    return json.loads(result.read_text())


def clean(text) -> str:
    return text if isinstance(text, str) and NAME.match(text) else "?"


def summary(result: dict) -> str:
    """The comment: the verdict, the first difference, the counts and the failed checks, from result.json's fields alone."""
    verdict = result.get("verdict")
    if verdict == "refused":
        lines = str(result.get("message", "")).splitlines()
        return "refused: " + (lines[0][:300] if lines else "the module was refused")
    checks = {**(result.get("scenarios") or {}), **(result.get("recordings") or {})}
    bad = {n: v for n, v in checks.items() if v.get("status") != "equal"}
    counts = f"{len(result.get('scenarios') or {})} scenarios, {len(result.get('recordings') or {})} recordings"
    if verdict == "equal" and not bad:
        return f"equal: every scenario and recording equal ({counts})"
    lines = [f"{verdict if verdict in ('differs', 'failed') else 'failed'}: {len(bad)} of {len(checks)} checks not equal ({counts})"]
    first = next((n for n, v in bad.items() if v.get("status") == "differs"), None)
    if first is not None:
        v = bad[first]
        where = ", ".join(f"{k} {clean(str(v[k]))}" if k in ("phase", "field") else f"{k} {v[k]}" for k in ("tick", "phase", "field") if v.get(k) is not None)
        lines.append(f"first difference: {clean(first)}" + (f", {where}" if where else ""))
    for name, v in list(bad.items())[:LISTED_LIMIT]:
        lines.append(f"{clean(name)}: {clean(str(v.get('status')))}")
    if len(bad) > LISTED_LIMIT:
        lines.append(f"and {len(bad) - LISTED_LIMIT} more")
    return "\n".join(lines)


def load_state(path: Path) -> dict[str, str]:
    try:
        data = json.loads(path.read_text())
    except (OSError, ValueError):
        return {}
    return {str(k): str(v) for k, v in data.items()} if isinstance(data, dict) else {}


def save_state(path: Path, state: dict[str, str]):
    path.parent.mkdir(parents=True, exist_ok=True)
    scratch = path.with_suffix(".tmp")
    scratch.write_text(json.dumps(state, indent=1, sort_keys=True) + "\n")
    scratch.replace(path)


def judge_pull(api: Api, pull: dict, game_dir: str | None, full: bool, judge: Path | None) -> dict | None:
    """Download the pull request's artifact into a fresh directory and judge it. None when there is nothing to judge."""
    artifact = find_artifact(api, pull["sha"])
    if artifact is None:
        print(f"#{pull['number']} skipped: no module artifact for the head commit")
        return None
    work = Path(tempfile.mkdtemp(prefix="gatebtm-"))
    try:
        blob = api.download(f"/repos/{api.repo}/actions/artifacts/{artifact}/zip", ZIP_LIMIT)
        module, subsystem = unpack(blob, work, known_subsystems())
        return run_oraclegate(module, subsystem, work / "out", game_dir, full, judge)
    finally:
        shutil.rmtree(work, ignore_errors=True)


def run_pass(api: Api, state_file: Path, game_dir: str | None, full: bool, dry_run: bool, judge: Path | None = None) -> int:
    state = load_state(state_file)
    failures = 0
    for pull in open_pulls(api):
        n = pull["number"]
        if not eligible(api, pull):
            print(f"#{n} skipped: not eligible")
            continue
        if state.get(str(n)) == pull["sha"]:
            print(f"#{n} skipped: already judged")
            continue
        try:
            result = judge_pull(api, pull, game_dir, full, judge)
            if result is None:
                continue
            text = summary(result)
            if dry_run:
                print(f"#{n} (dry run, not posted): {text}")
                continue
            api.request("POST", f"/repos/{api.repo}/issues/{n}/comments", {"body": text}).close()
            state[str(n)] = pull["sha"]
            save_state(state_file, state)
            print(f"#{n} posted: {text.splitlines()[0]}")
        except (Error, OSError, ValueError, KeyError, subprocess.TimeoutExpired) as err:
            failures += 1
            print(f"#{n} failed: {str(err).replace(api.token, '***')}")
    return 1 if failures else 0


def run_local(module: Path, subsystem: str, game_dir: str | None, full: bool, judge: Path | None = None) -> int:
    """Judge a local module from the download step on (a copy in a fresh directory) and print the summary. Posts nothing."""
    if subsystem not in known_subsystems():
        print(f"gatebtm: unknown subsystem {subsystem}", file=sys.stderr)
        return 2
    work = Path(tempfile.mkdtemp(prefix="gatebtm-"))
    try:
        copy = work / "module.wasm"
        shutil.copyfile(module, copy)
        result = run_oraclegate(copy, subsystem, work / "out", game_dir, full, judge)
    except (Error, OSError, ValueError) as err:
        print(f"gatebtm: {err}", file=sys.stderr)
        return 2
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print(summary(result))
    return 0 if result.get("verdict") == "equal" else 1


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--repo", default=broker.DEFAULT_REPO)
    ap.add_argument("--api-base", default=broker.DEFAULT_API)
    ap.add_argument("--token-file", default=str(broker.DEFAULT_TOKEN_FILE), help="the file holding the scoped btm token, read when the pass starts")
    ap.add_argument("--state", type=Path, default=DEFAULT_STATE, help="the judged (pull request, head commit) pairs")
    ap.add_argument("--game-dir")
    ap.add_argument("--full", action="store_true", help="play the full --check instead of the tier")
    ap.add_argument("--dry-run", action="store_true", help="judge, but post and record nothing")
    ap.add_argument("--local", type=Path, help="judge this module instead of the pull requests' artifacts and print the summary (tests only; never posts)")
    ap.add_argument("--subsystem", help="the subsystem of --local's module")
    args = ap.parse_args()
    if args.local is not None:
        if not args.subsystem:
            ap.error("--local needs --subsystem")
        if not args.local.is_file():
            ap.error(f"{args.local} is not a file")
        return run_local(args.local.resolve(), args.subsystem, args.game_dir, args.full)
    if args.subsystem:
        ap.error("--subsystem is for --local")
    if not broker.REPO_RE.match(args.repo):
        ap.error("--repo must be owner/name")
    token, why = broker.read_token(args.token_file)
    if token is None:
        print(f"gatebtm: {why}", file=sys.stderr)
        return 2
    return run_pass(Api(args.api_base, args.repo, token), args.state, args.game_dir, args.full, args.dry_run)


if __name__ == "__main__":
    sys.exit(main())
