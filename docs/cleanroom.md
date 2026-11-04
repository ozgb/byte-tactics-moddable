# Clean-room sandbox

Implementer agents for the clean-room engine (#180) run in a microVM that can't reach this
repository. The tooling is in `tools/cleanroom/` and runs on the host, outside the VM.

## Host setup

- smolvm 1.33.0, a user install at `~/.local/bin/smolvm` (no sudo; `sudo` in the agent containers
  needs a password, so nothing here may depend on it). Check with `smolvm --version`.
- `/dev/kvm` readable and writable by your user.
- `uv`, and `git` on the host. Nothing is installed on the host for the VM's toolchain.

## Build the image

```sh
tools/cleanroom/build-image.sh build/scratch/cleanroom-image
```

`Smolfile` pins the base (`alpine:3.22` by its index digest), Rust 1.87.0 with its wasm32 targets (`rust-wasm`),
wasm-tools 1.262.0 (a release tarball checked against its sha256) and Claude Code 2.1.311 (npm).
The build is the only step with network, and it runs in a throwaway build machine whose network is
opened by `--net` on the command line, not by the Smolfile: smolvm bakes a machine's network setting
into the packed image, and `launch.py` switches it off again for every session. The result is
`cleanroom.smolmachine` (about 620 MB), packed from the build machine. The build also copies
`tools/cleanroom/managed-settings.json` to `/etc/claude-code/managed-settings.json`, the managed
settings file Claude Code reads on Linux. It denies the `WebSearch` and `WebFetch` tools, and the
agent (an unprivileged user) can't change it.

## Start a session

```sh
uv run tools/cleanroom/launch.py --tree DIR --datakit DIR --image FILE.smolmachine \
    [--session DIR] [--credential-file FILE] [--credential-kind oauth|api-key] -- COMMAND [ARG ...]
```

`--tree` is the clean-room working tree (its own git repository), `--datakit` the data kit. The
command runs in the VM as an unprivileged user with the host user's uid, in `/work`. A trial that
builds a wasm module, uses the mailbox and hands over a git bundle is `tools/cleanroom/trial.sh`:
`launch.py ... -- sh -c "$(cat tools/cleanroom/trial.sh)"`.

The launcher refuses (exit 2) before creating a VM when: a mount is, contains or lies inside this
repository or any of its worktrees (only `build/scratch` below a worktree is allowed, not that
directory itself), or overlaps `~/.claude*`, `~/.ta`, `~/.ssh`, `~/.config/gh`, `~/.config/bt-cleanroom`, `~/.gnupg` or
`~/source/repos`; the tree has no `.git` directory of its own or its git config names
byte-tactics; the tree and data kit overlap; either mount holds a file whose hash is in the canary
manifest; the allowlist is empty, lists an IP literal or lists GitHub, GitLab, Bitbucket, ghcr.io
or similar hosts; the credential file is missing, empty or readable by anyone but its owner, or holds a credential the host's
Claude Code uses (see Credentials); the image, smolvm or `/dev/kvm` is
missing; the session directory is, contains or lies inside this repository or `~/.claude*`
and the like, overlaps the tree or data kit, or holds a manifest file. The `smolvm machine create`
argv is checked against the same rules in code (no `--net`, `--ssh-agent` or other grants, exactly
the four mounts below, one socket mount), then the launcher
turns the machine's network off with `machine update --no-net` and refuses to go on if smolvm still
reports one.

## What the VM sees and what crosses

| In the VM | Host side | Direction |
| --- | --- | --- |
| `/work` | the clean-room tree | read-write |
| `/data` | the data kit | read-only |
| `/srv/outbox` | `SESSION/outbox` | read-write: gate requests and the git bundle |
| `/srv/inbox` | `SESSION/inbox` | read-only: gate results, written by the broker only |
| `/srv/proxy.sock` | a Unix socket served by `proxy.py` | the only way out |

The VM has no network device that reaches anything. `launch.py` starts `socat` in the VM so that
`127.0.0.1:3128` forwards to `/srv/proxy.sock`, and sets `HTTPS_PROXY` to it. The home directory
(`/home/agent`) is fresh in every VM, with no Claude Code memory or credentials, and a git config
with an identity and no remote. The credential is passed to the agent
command by `--secret-env` and never written to disk (see Credentials).

- **Web tools:** server-side web search and fetch run on Anthropic's side, so the allowlisted
  API host would give the agent the web. The managed settings deny the two tools and the canary
  checks the file. With a subscription token there is no organisation setting to switch the
  server-side tools off, so these managed settings are the only control (see Credentials).
- **Network:** `proxy.py` accepts only `CONNECT host:port` for the exact pairs in
  `tools/cleanroom/allowlist.txt` (the Anthropic API and the crates.io index and download hosts),
  resolves them on the host, refuses names that resolve to loopback or private addresses, and logs
  every request as a JSON line in `SESSION/proxy.log`. A tunnel to a host name must open with a
  TLS ClientHello whose server name is that host, else the proxy closes it and logs
  `sni_refused`. After the session `launch.py` runs
  `proxy.py --check-log`, which fails if any logged request is off the allowlist. The canary
  probes go to `SESSION/canary-proxy.log` (the log is rotated by SIGHUP once the canary passes).
- **Git:** the agent runs `git bundle create /srv/outbox/.btm.tmp BRANCH ...` and renames the
  file to `/srv/outbox/btm.bundle`. No repository is shared: the broker opens the bundle with
  `O_NOFOLLOW` through a descriptor of the outbox directory (a symlink is refused), checks that it
  is a regular file of at most 1 GiB, copies its bytes into a file the VM can't write, runs
  `git bundle verify` against `SESSION/bare/btm.git` (a bundle whose prerequisite commits the bare
  repository lacks fails) and fetches `refs/heads/*` from the copy without force, with hooks,
  system and global git config off and `transfer.fsckObjects` on. A bundle is a pack and refs
  only, so it carries no alternates, hooks or config, and the guest can't make the broker read any
  path on the host. Each distinct bundle is processed once. The bare repository accepts only
  `refs/heads/impl/<name>`, where `<name>` is one or more `/`-separated segments matching
  `[a-z0-9][a-z0-9._-]*` (no `..`, no segment ending in `.` or `.lock`): `main`, a name starting with `-`, tags
  and everything else are refused and logged. The broker then pushes each such branch whose commit changed to
  `ozgb/btm` and opens a pull request into its `main`, with the token from `~/.config/bt-cleanroom/btm-token`
  (`--btm-token-file`), read when it pushes and given to git through `GIT_ASKPASS` for that one process
  (`tools/cleanroom/askpass.sh` prints it from an environment variable) and to the API in one request header. It is
  never in the broker's environment, a git config file or a log line. A token file that is missing, empty or readable
  by group or others means nothing is pushed. A branch is pushed again only when its commit changes, a failed push or
  pull request is retried on the next pass, and `--no-push` turns pushing off.
  Before each push the broker checks every blob in the branch's history and refuses the push (logging the path, never content,
  and not recording it, so it is retried only when the commit changes) when a path ends in `.wasm`, a blob's sha256 is in the
  session's `manifest.txt` or equals a `*.wasm` under `build/modern` or `build/wasm-modules`, or the session has no manifest.
- **Gate:** the agent creates `/srv/outbox/ID/module.wasm` and `/srv/outbox/ID/subsystem`, then
  `/srv/outbox/ID/ready`. The broker opens the request directory and each file with `O_NOFOLLOW`
  relative to a directory descriptor, checks the opened descriptors (regular files only), reads
  the module once, checks id, subsystem name, size and wasm magic, and runs the real gate on the host:
  `tools/oraclegate.py SESSION/gate/ID/module.wasm --subsystem NAME --out SESSION/gate/ID/out` (#731), in its own
  directory outside the mailbox, with a timeout (`--gate-timeout`, default four hours). It copies only
  `out/result.json` to `SESSION/inbox/ID.json`: the reports in `out/logs/` and the runtime stay on the host, and a
  run that writes no usable result (or one over 1 MiB, or not a JSON object) answers with an `error` and no log text.
  The result is the one of `docs/module-abi.md` section 15: `verdict` `equal` when every scenario and recording is
  equal.

## Credentials

Two secrets live on the host, in `~/.config/bt-cleanroom/` (mode 700, files mode 600). They are files, not environment
variables: a variable in a shell profile would reach every Claude Code session on the host. A tool reads a file
when it needs the secret and hands it only to the process that uses it.

| File | Holds | Read by |
| --- | --- | --- |
| `anthropic-impl-key` | the implementer's `CLAUDE_CODE_OAUTH_TOKEN` (`claude setup-token`) | `launch.py`, for the agent command |
| `btm-token` | a fine-grained GitHub token for `ozgb/btm` only | `broker.py`, when it pushes |
| `gate-token` | (later, #797) the gate agent's own btm token | the gate agent's tooling |

**The btm token:** repository access to `ozgb/btm` and nothing else. Contents read and write (push the session's
branches), Pull requests read and write (open the pull requests), Metadata read, everything else "No access". Workflows
stays off, so GitHub refuses any push that changes `.github/workflows/` and a session can't change the CI that builds the
artifact the gate runs. Administration stays off. The token has an expiry date. #797's gate token is a second token
(Actions read, Pull requests read and write, Contents read), so that a leak of one exposes only its own permissions.

**The session credential:** `launch.py --credential-file FILE` (default `~/.config/bt-cleanroom/anthropic-impl-key`)
`--credential-kind oauth|api-key` (default `oauth`). It reads the file itself and gives the value to the agent command with
`--secret-env` as `CLAUDE_CODE_OAUTH_TOKEN` (`oauth`) or `ANTHROPIC_API_KEY` (`api-key`). It refuses to start (exit 2) when
the file is missing, empty, not owned by you or readable by anyone but its owner, or when the value equals a credential the
host's Claude Code uses: `ANTHROPIC_API_KEY` or `CLAUDE_CODE_OAUTH_TOKEN` in the launcher's environment, or the access
token in `~/.claude/.credentials.json`. The refusal never prints the value.

**Never set `ANTHROPIC_API_KEY` or `CLAUDE_CODE_OAUTH_TOKEN` on the host**, in a profile or a shell: Claude Code reads
either one in preference to its normal login, so every host session would use that credential, and the launcher refuses a
file that holds the same value.

**What a subscription token changes:**

- Web search: a subscription has no organisation setting that switches off server-side web search and fetch. The managed
  settings that deny `WebSearch` and `WebFetch`, checked by the canary, are the only control. Claude Code doesn't offer a
  denied tool to the model, so that holds, but it is one layer where an API key's organisation would have given two.
- Usage: implementer sessions draw on the same limits as the human's own sessions.
- Hosts: the allowlist (`api.anthropic.com` and the crates.io hosts) should be enough for OAuth. The first session's
  `proxy.log` shows whether Claude Code needs another host.

**Host deny rules:** to keep a host agent from reading the secrets by accident, add this to `~/.claude/settings.json`
(a guard against accidents, not a boundary: every agent runs as the same user, and a separate Unix user for the broker
would be the real one):

```json
{
  "permissions": {
    "deny": [
      "Read(~/.config/bt-cleanroom/**)",
      "Edit(~/.config/bt-cleanroom/**)",
      "Bash(cat ~/.config/bt-cleanroom:*)",
      "Bash(cat ~/.config/bt-cleanroom/*:*)",
      "Bash(head ~/.config/bt-cleanroom/*:*)",
      "Bash(tail ~/.config/bt-cleanroom/*:*)",
      "Bash(less ~/.config/bt-cleanroom/*:*)",
      "Bash(grep * ~/.config/bt-cleanroom/*:*)",
      "Bash(ls ~/.config/bt-cleanroom:*)",
      "Bash(cp ~/.config/bt-cleanroom/*:*)",
      "Bash(base64 ~/.config/bt-cleanroom/*:*)"
    ]
  }
}
```

A rule that matches a command's text can't see a path built in a script, so it stops accidents only. The launcher also
never mounts `~/.config/bt-cleanroom` (or a directory that contains or lies inside it), and the canary checks that
the path doesn't exist in the VM, in the same way as `~/.config/gh`.

## First real session

Run these on the host, from a worktree of this repository, with `build/modern` built (the real gate needs it, and
`~/.ta` as `--game-dir`). Do not set `ANTHROPIC_API_KEY` or `CLAUDE_CODE_OAUTH_TOKEN` in the shell.

```sh
export UV_PYTHON=/usr/bin/python3.14 WINEARCH=win64 WINEDEBUG=-all
unset DISPLAY WAYLAND_DISPLAY
D=build/scratch/cleanroom-first
# 1. the image (once)
tools/cleanroom/build-image.sh build/scratch/cleanroom-image
# 2. the data kit and a working tree that is its own git repository
uv run tools/oraclekit.py export $D/datakit
git clone https://github.com/ozgb/btm.git $D/tree
# 3. start the session: the VM, the proxy, the broker (real gate, pushes to ozgb/btm) and the agent command
uv run tools/cleanroom/launch.py --tree $D/tree --datakit $D/datakit --session $D/session \
    --image build/scratch/cleanroom-image/cleanroom.smolmachine --game-dir ~/.ta -- COMMAND
```

The real gate needs `--game-dir` when the game is not in a Steam or GOG default place; without it the gate fails with
"no Total Annihilation install found". `launch.py` forwards it to `broker.py --game-dir`.

1. Builds the image: Rust, wasm-tools and Claude Code, with network, once.
2. `oraclekit.py export` writes the data kit the agent reads at `/data`; the clone is the agent's `/work`. On a commit
   without an `interfaces-v*` and `reference-traces-v*` tag it refuses; `--untagged` writes a kit without a version.
3. `launch.py` runs the canary checks (it stops before the agent on any failure), then COMMAND as the unprivileged
   user with the implementer credential. COMMAND is the implementer's Claude Code invocation, for example
   `claude -p "Implement wind_meteors from /data; submit it through /srv/outbox"`.

In the VM, the agent submits the 3.1c wind/meteors module through the mailbox. The module is `build/modern/wind_meteors.wasm`
(the build's own copy of `build/wasm-modules/wind_meteors.wasm`, #9); to test the path without an agent, run COMMAND as
`sleep 900` and, in a second shell, copy it into `$D/session/outbox/` as the agent would:

```sh
R=$D/session/outbox/req1; mkdir $R
cp build/modern/wind_meteors.wasm $R/module.wasm
echo wind_meteors > $R/subsystem
touch $R/ready
```

4. The broker runs `oraclegate.py` on it; read the answer when it appears (the module is the shipped one, so the verdict is
   `equal` for every scenario and recording):

```sh
cat $D/session/inbox/req1.json
```

5. The pushed branch and its pull request: the agent commits on a branch `impl/<name>` in `/work`, writes a bundle
   (`git bundle create /srv/outbox/.btm.tmp impl/<name>`, renamed to `/srv/outbox/btm.bundle`), and the broker pushes it and
   opens the pull request on `ozgb/btm`. `cat $D/session/broker.log` shows `push:` and `pull request:` lines.
6. After the session, `launch.py` has already run the proxy log check; run it again and read the log:
   `uv run tools/cleanroom/proxy.py --check-log $D/session/proxy.log --allow tools/cleanroom/allowlist.txt` prints
   `all on the allowlist` and exits 0 when no request left the allowlist (and fails on a refused server name).

## Canary checks

`canary.py` runs in the VM as root, before the agent starts, with `manifest.py` copied beside it
and the manifest built on the host from this repository's `src/`, `include/`, `orig/TotalA.exe`
and `~/.ta/TotalA.exe`. A failing check stops the session with exit 3, and so does a canary directory that is still in
the VM after `launch.py` deleted it (the agent never sees the manifest). The checks: the host's
paths (home, `~/.claude`, `~/.ta`, `~/.ssh`, `~/.config/bt-cleanroom`, `~/work`, each worktree) do not exist; no default
route; `github.com` does not resolve and direct TCP to public addresses fails; the proxy answers
403 to `github.com`, `api.github.com`, `raw.githubusercontent.com`, port 22, loopback and the
metadata address; no GitHub or SSH variables in the environment and no `.ssh`, `gh`, `bt-cleanroom`, `.claude` or
credential files in the home directory; `/etc/claude-code/managed-settings.json` denying
`WebSearch` and `WebFetch`; a planted copy under another name is found by the scanner
(so a broken scanner can't pass); and no file anywhere in the VM, mounts included, hashes to a
file of the manifest (files over 64 MiB are skipped and counted).

## Limits

- The proxy checks the server name of the first TLS record and then tunnels the rest without
  looking inside it, so an agent can send anything to the Anthropic API or crates.io (publishing a crate would need a token the VM
  doesn't have). The allowlist is the control; keep it short.
- Rust's registry hosts are allowed, so a crate that embeds decompiled code could be fetched if one
  existed; the data kit and spec rules (#180) cover what the agent is told, not this tooling.
- The hash scan finds copies, not rewrites or recollections. It is a backstop to never mounting
  the repository.
- The outbox is written by the VM and read by the host. The broker reads it as data (opened
  with `O_NOFOLLOW`, regular files only, size-capped, names validated; the bundle goes through
  `git bundle verify` and `fsckObjects` with hooks and config off), but a git or filesystem bug
  on the host would be reachable from the VM.
- smolvm runs the VM as the host user; virtiofs shares expose that user's ownership, which is why
  the agent runs with the host uid. smolvm's state directory holds the machine
  until `launch.py` deletes it (`--keep` leaves it).
- The VM's CPU, memory and the 20 GiB disk are smolvm's defaults (4 vCPUs, 4 GiB here); nothing
  limits time except the command's own.
- Starting a VM needs KVM, so the CI tests (`tools/test_cleanroom.py`) cover the refusal rules,
  the manifest, the proxy, the canary functions and the broker, not a VM boot.
