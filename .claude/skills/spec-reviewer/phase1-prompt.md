# Phase 1 prompt

The prompt the pilot (#800) ran in the sandbox, with the scratch replay it did on its own made explicit. Replace `<subsystem>` throughout.

```text
You are reviewing a behavioural specification for completeness, as the engineer who will have to implement it. You have never seen the original game's code, and you must not try to find it: work only from /work/specs/<subsystem>.md (the spec), /work/specs/TEMPLATE.md, and the data kit at /data (start with /data/README.md; /data/docs/module-abi.md is the module ABI, /data/interfaces/ the interface headers, /data/layout.json the state block layout, /data/tests/replay/<subsystem>/ recordings of the reference, format in /data/docs/replay-format.md).

The target is a WebAssembly module, written in Rust for wasm32, for the <subsystem> subsystem. It must reproduce the reference bit for bit: a gate replays the recordings call by call (imports in order, state writes) and plays full scenarios comparing traces.

Answer: could you write that module from the spec and the kit alone? Write a scratch implementation of every slot (in /tmp, never in /work) and replay every recording against it, comparing imports and writes call by call. Then report:
1. every question you could not answer from the spec and the kit, including every slot or outcome no recording covers;
2. every place the spec contradicts the kit or the recordings;
3. every place the spec seems to describe how the original was coded rather than its behaviour;
4. a verdict line: `approve`, or `changes` followed by the list.
Cite the spec section and kit file for each point. Keep it under 700 words. Write the report to /work/.review/phase1.md (create the directory), and do not commit it or anything else.
```
