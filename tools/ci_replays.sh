#!/usr/bin/env bash
set -eo pipefail

bindir=${1:?usage: ci_replays.sh <bindir> [runner]}
runner=${2:-}
cd "$(dirname "$0")/.."
logs=${RUNNER_TEMP:-${TMPDIR:-/tmp}}
rec=tests/replay

py=
for candidate in python3 python; do
  if "$candidate" -c '' 2>/dev/null; then
    py=$candidate
    break
  fi
done
[ -n "$py" ] || { echo "ci_replays.sh: no python3 or python on PATH" >&2; exit 1; }
"$py" tools/replay_subsystems.py check

run() {
  local log=$1 want=$2
  shift 2
  # shellcheck disable=SC2086
  $runner "$@" </dev/null | tee "$logs/$log.log"
  grep -q "^$want: all passed" "$logs/$log.log"
}

rows() {
  "$py" tools/replay_subsystems.py list "$1"
}

for t in block_test ref_test replay_test; do
  run "$t" "$t" "$bindir/$t.exe"
done
while IFS=$'\t' read -r log name; do
  run "$log" replay_native_test "$bindir/replay_native_test.exe" "$rec/$name"/*.txt
done < <(rows native)
for _ in $(seq "${REPLAY_SELF_REPEAT:-1}"); do
  while IFS=$'\t' read -r log name file; do
    run "$log" replay_native_test "$bindir/replay_native_test.exe" --self-test "$rec/$name/$file"
  done < <(rows self)
done
while IFS=$'\t' read -r log name expect field file; do
  run "$log" replay_native_test "$bindir/replay_inject_test.exe" --expect "$expect" "$field" "$rec/$name/$file"
done < <(rows inject)
while IFS=$'\t' read -r log name module; do
  run "$log" replay_native_test "$bindir/replay_native_test.exe" --wasm "$bindir/$module.wasm" "$rec/$name"/*.txt
done < <(rows wasm)
for _ in $(seq "${REPLAY_SELF_REPEAT:-1}"); do
  while IFS=$'\t' read -r log name module file; do
    run "$log" replay_native_test "$bindir/replay_native_test.exe" --wasm "$bindir/$module.wasm" --self-test "$rec/$name/$file"
  done < <(rows wasm-self)
done
while IFS=$'\t' read -r log name variant expect field file; do
  run "$log" replay_native_test "$bindir/replay_native_test.exe" --wasm "$bindir/$variant.wasm" --expect "$expect" "$field" "$rec/$name/$file"
done < <(rows wasm-inject)
