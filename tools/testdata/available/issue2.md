## Goal

Run every simulation subsystem as a WASM module (#9, #10).

## Order

**Priority now** (the human, 2026-11-11: general interfaces before the clean room, on the scheduler's rules): these, in this order.

**What is claimable** (the human, 2026-11-24, replacing "and nothing else"): every open issue with no assignee and no open blockers, unless it is labelled `parked`, `needs-grooming` or `needs-human`, is a parent with open sub-issues, or is a tracker (#2, #180, #392, #632). An issue the human files is claimable without being listed here.

~~#1022~~ done, PR #1023 (2026-11-22, merge 0689a835e): `data/coverage.csv` rewritten on b09813586, after #1021.

0. ~~#943~~ done, PR #945 (2026-11-11): each apt job probes the runner's mirrors.
1. #933: 3.1c's order of effects, `docs/schedule.md` (done, PR #940, 2026-11-11).
11. ~~#967: players on the scheduler~~ done (PR #987, merged 2026-11-17 at 324eed7a5, `interfaces-v16`).
24. ~~#1034: the two check gaps #1028 left.~~ done (PR #1035, merged 2026-11-24 at 2f273978a).
24a. #1056: more than one lead at once (the human, 2026-11-24). No gate. Until it merges, that lead takes only #1043's sub-issues.
25. #1036: one general `Emit(effect, size)` in `interfaces/services.h` replaces `WindMeteorsEmit`, so the btm#27 spec can say how targeting publishes.

The engine core (#1038, the human, 2026-11-24): #1038 is their parent, not an issue to claim; #1036 above is its first part.

26. #1037: the end-of-tick `tick` row (trace format 9).
27. #1039: sim events judged for each tick, without phases.
28. #1040: the commands of a tick as events.
29. #1041: the gate for a clean-room core. Blocked by #1036, #1037, #1039 and #1040.
30. #1042: the core spec, written to ozgb/btm. Blocked by #1036, #1037, #1039, #1040 and #1041.

Module recordings (#1043, filed for the human on 2026-11-24; it replaces #389): #1043 is their parent, not an issue to claim.

31. #1044: the recorder form for a module's region at `Init`.
32. #1045: recordings for the seven modules left, plus weapons' three slots left from #711.
33. #897: a feature reclaim scenario and its recording. After #1045.
34. #1046: the oracle a ported data loader is judged by. Filed under #180.

Open to claim (2026-11-24): #1056, #1040, #1044, #1045, #1046 (#1036, #1037 and #1039 claimed). #1034 (PR #1035), #1028 (PR #1033).

Every other open issue is labelled `parked`: the pull requests already open (#924 to #932) finish as usual, and so does #938.

**Earlier priority** (the human, 2026-10-24: the source cleanup first):

1. #3 Conformity harness and reference traces (#51, then #52 and #53)
2. #4 Bit-exact CRT and maths

## Decisions taken

- #7 before #8.
