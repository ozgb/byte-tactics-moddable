#!/usr/bin/env bash
set -eo pipefail

bindir=${1:?usage: ci_replays.sh <bindir> [runner]}
runner=${2:-}
cd "$(dirname "$0")/.."
logs=${RUNNER_TEMP:-${TMPDIR:-/tmp}}
rec=tests/replay

run() {
  local log=$1 want=$2
  shift 2
  # shellcheck disable=SC2086
  $runner "$@" | tee "$logs/$log.log"
  grep -q "^$want: all passed" "$logs/$log.log"
}

for t in block_test ref_test replay_test; do
  run "$t" "$t" "$bindir/$t.exe"
done
run replay_native replay_native_test "$bindir/replay_native_test.exe" "$rec"/wind_meteors/*.txt
run replay_economy replay_native_test "$bindir/replay_native_test.exe" "$rec"/economy/*.txt
run replay_targeting replay_native_test "$bindir/replay_native_test.exe" "$rec"/targeting/*.txt
for _ in $(seq "${REPLAY_SELF_REPEAT:-1}"); do
  run replay_self replay_native_test "$bindir/replay_native_test.exe" --self-test "$rec/wind_meteors/meteors-hailstorm.txt"
done
run replay_inject replay_native_test "$bindir/replay_inject_test.exe" --expect 2 SimGlobals.g_meteorScheduleGap "$rec/wind_meteors/meteors-hailstorm.txt"
run replay_economy_inject replay_native_test "$bindir/replay_inject_test.exe" --expect 5501 Player#0.energy "$rec/economy/duel-shore-to-shore.txt"
run replay_targeting_inject replay_native_test "$bindir/replay_inject_test.exe" --expect 352 pos.x "$rec/targeting/skirmish4-long.txt"
run replay_wasm replay_native_test "$bindir/replay_native_test.exe" --wasm "$bindir/wind_meteors.wasm" "$rec"/wind_meteors/*.txt
for _ in $(seq "${REPLAY_SELF_REPEAT:-1}"); do
  run replay_wasm_self replay_native_test "$bindir/replay_native_test.exe" --wasm "$bindir/wind_meteors.wasm" --self-test "$rec/wind_meteors/meteors-hailstorm.txt"
done
run replay_wasm_inject replay_native_test "$bindir/replay_native_test.exe" --wasm "$bindir/wind_meteors_inject.wasm" --expect 2 SimGlobals.g_meteorScheduleGap "$rec/wind_meteors/meteors-hailstorm.txt"
