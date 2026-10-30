"""Run `wasm_maths.py check --embedded` many times at once, each in its own fresh Wine prefix (#585).

    uv run tools/wasm_loop.py [--runs N] [--debug] [--host DIR] [--dir DIR]

Each run is `uv run tools/wasm_maths.py check --embedded --host HOST --prefix DIR/run-I/prefix`, with
--debug added and RUST_BACKTRACE=1 set for the debug host. A run's output goes to DIR/run-I/output.txt
(default DIR: build/wasm_loop, or build/wasm_loop_debug with --debug). The summary prints the Wine
version, the AeDebug setting of the first prefix, the count of failed runs and, for each failure, its
exit code, the last 40 lines of its output and any panic message and backtrace. The exit code is 1 when
a run failed. Prefixes are kept; delete DIR when done.
"""

import argparse
import os
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TAIL = 40
AEDEBUG = r"HKLM\Software\Microsoft\Windows NT\CurrentVersion\AeDebug"
PANIC_MARKERS = ("panicked at", "assertion failed", "stack backtrace")


def run_env(prefix):
    env = dict(os.environ, WINEARCH="win64", WINEPREFIX=str(prefix), WINEDEBUG="-all")
    for name in ("DISPLAY", "WAYLAND_DISPLAY"):
        env.pop(name, None)
    return env


def command(index, host, debug, base):
    cmd = ["uv", "run", str(ROOT / "tools" / "wasm_maths.py"), "check", "--embedded", "--prefix", str(base / f"run-{index}" / "prefix")]
    if host:
        cmd += ["--host", str(host)]
    if debug:
        cmd.append("--debug")
    return cmd


def panic_lines(lines):
    """The lines from the first panic message to the end, or an empty list."""
    for number, line in enumerate(lines):
        if any(marker in line for marker in PANIC_MARKERS):
            return lines[number:]
    return []


def summarise(results, version, aedebug, seconds):
    """The summary text for results, a list of (index, exit code, output lines)."""
    failed = [result for result in results if result[1] != 0]
    out = [f"wine: {version}", f"AeDebug: {aedebug}", f"runs: {len(results)}, failed: {len(failed)}, seconds: {seconds:.0f}"]
    for index, code, lines in failed:
        out.append(f"--- run {index}: exit code {code}, last {TAIL} lines")
        out.extend(lines[-TAIL:])
        panic = panic_lines(lines)
        if panic:
            out.append(f"--- run {index}: panic and backtrace")
            out.extend(panic)
    return "\n".join(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--runs", type=int, default=30)
    parser.add_argument("--debug", action="store_true")
    parser.add_argument("--host", type=Path)
    parser.add_argument("--dir", type=Path)
    options = parser.parse_args()
    base = (options.dir or ROOT / "build" / ("wasm_loop_debug" if options.debug else "wasm_loop")).resolve()
    base.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    if options.debug:
        env["RUST_BACKTRACE"] = "1"
    started = time.time()
    procs = []
    for index in range(options.runs):
        (base / f"run-{index}").mkdir(parents=True, exist_ok=True)
        handle = open(base / f"run-{index}" / "output.txt", "w")
        procs.append((index, handle, subprocess.Popen(command(index, options.host, options.debug, base), stdout=handle, stderr=subprocess.STDOUT, env=env, cwd=ROOT)))
    results = []
    for index, handle, proc in procs:
        code = proc.wait()
        handle.close()
        results.append((index, code, (base / f"run-{index}" / "output.txt").read_text(errors="replace").splitlines()))
    wine = subprocess.run(["wine", "--version"], capture_output=True, text=True).stdout.strip()
    query = subprocess.run(["wine", "reg", "query", AEDEBUG], env=run_env(base / "run-0" / "prefix"), capture_output=True, text=True)
    aedebug = " | ".join(line.strip() for line in query.stdout.splitlines() if line.strip()) or query.stderr.strip()
    text = summarise(results, wine, aedebug, time.time() - started)
    (base / "summary.txt").write_text(text + "\n")
    print(text)
    sys.exit(1 if any(code for _, code, _ in results) else 0)


if __name__ == "__main__":
    main()
