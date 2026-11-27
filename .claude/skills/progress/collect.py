"""Writes data.json for the clean-room progress page: one conduit per module, from this repo to ozgb/btm."""

import base64
import json
import re
import subprocess
import sys
from datetime import datetime, timedelta, timezone
from pathlib import Path

SITE = Path(__file__).resolve().parents[3]
ROOT = Path(subprocess.run(["git", "-C", str(SITE), "rev-parse", "--path-format=absolute", "--git-common-dir"],
                           capture_output=True, text=True, check=True).stdout.strip()).parent
BTM = "ozgb/btm"
HERE = "ozgb/byte-tactics-moddable"
RECENT = timedelta(hours=48)

FOUNDATIONS = [
    dict(id="core", label="Engine core", spec="engine-core.md", btm_dir="core", issue=1207,
         heads=["spec-engine-core", "impl/engine-core"], kit="core gate"),
    dict(id="runtime", label="Runtime", spec="runtime.md", btm_dir="runtime", issue=None,
         heads=["runtime-spec"], kit="runtime vectors"),
    dict(id="files", label="Data loading", spec=None, btm_dir="files", issue=None,
         heads=["import-nanolathe", "files-port", "files-formats"], kit="files gate", spec_by_import="import-nanolathe"),
]
SUBSYSTEMS = [
    ("wind_meteors", "Wind & meteors", None),
    ("economy", "Economy", 1201),
    ("targeting", "Targeting", 1202),
    ("los_radar", "LOS & radar", 1189),
    ("mission", "Mission", 1190),
    ("pathfinding", "Pathfinding", 1191),
    ("map", "Map", 1192),
    ("players", "Players", 1193),
    ("features", "Features", 1194),
    ("ai", "AI", 1195),
    ("weapons", "Weapons", 1196),
    ("scripts", "Scripts", 1197),
    ("movement", "Movement", 1198),
    ("orders", "Orders", 1199),
    ("units", "Units", 1200),
]


def gh(*args):
    out = subprocess.run(["gh", *args], capture_output=True, text=True)
    if out.returncode:
        return None
    return json.loads(out.stdout) if out.stdout.strip() else None


def ts(s):
    return datetime.fromisoformat(s.replace("Z", "+00:00"))


def spec_pin(name):
    meta = gh("api", f"repos/{BTM}/contents/specs/{name}")
    if meta is None:
        return None
    text = base64.b64decode(meta["content"]).decode()
    pins = [int(n) for n in re.findall(r"interfaces-v(\d+)", text)]
    return max(pins) if pins else 0


def main(out):
    now = datetime.now(timezone.utc)
    prs = gh("pr", "list", "-R", BTM, "--state", "all", "--limit", "200",
             "--json", "number,headRefName,state,updatedAt,title,url") or []
    specs = {e["name"] for e in gh("api", f"repos/{BTM}/contents/specs") or []}
    dirs = {e["name"] for e in gh("api", f"repos/{BTM}/contents") or [] if e["type"] == "dir"}
    milestones = {}
    for n in (2, 180):
        s = gh("api", f"repos/{HERE}/issues/{n}", "--jq", ".sub_issues_summary") or {}
        milestones[str(n)] = {"done": s.get("completed", 0), "total": s.get("total", 0)}
    recordings_route = (gh("issue", "view", "1209", "-R", HERE, "--json", "state") or {}).get("state") == "CLOSED"
    tag = subprocess.run(["git", "-C", str(ROOT), "tag", "-l", "interfaces-v*", "--sort=-v:refname"],
                         capture_output=True, text=True).stdout.split()
    latest = int(tag[0].split("v")[-1]) if tag else 0

    core_pin = spec_pin("engine-core.md") or 0

    def matching(heads):
        return [p for p in prs if any(p["headRefName"] == h or p["headRefName"].startswith(h + "-") for h in heads)]

    def row(r):
        mine = matching(r["heads"])
        live = [p for p in mine if p["state"] == "OPEN" and now - ts(p["updatedAt"]) < RECENT]
        spec_prs = [p for p in mine if not p["headRefName"].startswith("impl/")]
        impl_prs = [p for p in mine if p["headRefName"].startswith("impl/")]

        if r.get("spec_by_import"):
            spec = "done" if any(p["headRefName"] == r["spec_by_import"] and p["state"] == "MERGED" for p in mine) else "none"
            pin = None
        elif r["spec"] in specs:
            pin = spec_pin(r["spec"])
            spec = "done" if (pin or 0) >= core_pin else "stale"
        else:
            pin = None
            spec = "none"
        if spec != "done" and any(p in live for p in spec_prs):
            spec = "active"
        elif spec == "none" and any(p["state"] == "OPEN" for p in spec_prs):
            spec = "draft"

        issue = None
        if r["issue"]:
            i = gh("issue", "view", str(r["issue"]), "-R", HERE, "--json", "number,title,state,assignees,url")
            if i:
                issue = {"number": i["number"], "title": i["title"], "url": i["url"],
                         "claimed": bool(i["assignees"]), "state": i["state"]}
                if issue["claimed"] and spec in ("none", "stale"):
                    spec = "active"

        merged_impl = any(p["state"] == "MERGED" for p in impl_prs)
        if merged_impl and r["btm_dir"] in dirs:
            impl = "built"
        else:
            impl = "none"
        if any(p in live for p in impl_prs) or (r.get("spec_by_import") and any(p in live for p in mine)):
            impl = "active" if impl == "none" else "building"
        elif impl == "none" and any(p["state"] == "OPEN" for p in (mine if r.get("spec_by_import") else impl_prs)):
            impl = "draft"

        kit = r["kit"]
        recs = r.get("recordings")
        kit_state = "ready" if kit else ("partial" if recs else "none")
        return {
            "id": r["id"], "label": r["label"], "group": r["group"],
            "kit": kit_state, "kit_note": kit or ("%d recordings, gate route waits on #1209" % recs if recs else "no recordings"),
            "recordings": recs, "spec": spec, "spec_pin": pin, "impl": impl,
            "active": bool(live) or bool(issue and issue["claimed"] and issue["state"] == "OPEN"),
            "issue": issue,
            "prs": [{"number": p["number"], "title": p["title"], "state": p["state"], "url": p["url"],
                     "live": p in live} for p in sorted(mine, key=lambda p: -p["number"])[:4]],
        }

    rows = []
    for r in FOUNDATIONS:
        rows.append(row({**r, "group": "foundation"}))
    for name, label, issue in SUBSYSTEMS:
        recs = len(list((ROOT / "tests/replay" / name).glob("*"))) if (ROOT / "tests/replay" / name).is_dir() else 0
        kit = "scenario gate" if name == "wind_meteors" else ("recordings gate" if recordings_route else None)
        rows.append(row(dict(id=name, label=label, spec=f"{name}.md", btm_dir=name, issue=issue,
                             heads=[f"spec-{name.replace('_', '-')}", f"spec-{name}", f"impl/{name.replace('_', '-')}",
                                    f"impl/{name}"],
                             kit=kit, recordings=recs, group="subsystem")))

    data = {"generated": now.isoformat(timespec="seconds"), "interfaces": latest, "core_pin": core_pin,
            "milestones": milestones, "rows": rows}
    Path(out).write_text(json.dumps(data, indent=1))
    print("wrote %s: %d conduits, %d lit" % (out, len(rows), sum(r["active"] for r in rows)))


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else SITE / "data.json")
