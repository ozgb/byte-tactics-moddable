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
    [--session DIR] [--api-key-env ANTHROPIC_API_KEY_IMPL] -- COMMAND [ARG ...]
```

`--tree` is the clean-room working tree (its own git repository), `--datakit` the data kit. The
command runs in the VM as an unprivileged user with the host user's uid, in `/work`. A trial that
builds a wasm module, uses the mailbox and hands over a git bundle is `tools/cleanroom/trial.sh`:
`launch.py ... -- sh -c "$(cat tools/cleanroom/trial.sh)"`.

The launcher refuses (exit 2) before creating a VM when: a mount is, contains or lies inside this
repository or any of its worktrees (only `build/scratch` below a worktree is allowed, not that
directory itself), or overlaps `~/.claude*`, `~/.ta`, `~/.ssh`, `~/.config/gh`, `~/.gnupg` or
`~/source/repos`; the tree has no `.git` directory of its own or its git config names
byte-tactics; the tree and data kit overlap; either mount holds a file whose hash is in the canary
manifest; the allowlist is empty, lists an IP literal or lists GitHub, GitLab, Bitbucket, ghcr.io
or similar hosts; the API key variable is not `ANTHROPIC_*`; the image, smolvm or `/dev/kvm` is
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
with an identity and no remote. The API key, when given, is passed to the agent
command by `--secret-env` from the named host variable and never written to disk.

- **Web tools:** server-side web search and fetch run on Anthropic's side, so the allowlisted
  API host would give the agent the web. The managed settings deny the two tools and the canary
  checks the file. The implementer API key's organisation must also have server-side web search
  and fetch turned off (a task of #519, where the key comes in).
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
  path on the host. Each distinct bundle is processed once. A later issue (#519) pushes the bare
  repository to ozgb/btm with a credential that exists only on the host.
- **Gate:** the agent creates `/srv/outbox/ID/module.wasm` and `/srv/outbox/ID/subsystem`, then
  `/srv/outbox/ID/ready`. The broker opens the request directory and each file with `O_NOFOLLOW`
  relative to a directory descriptor, checks the opened descriptors (regular files only), reads
  the module once, checks id, subsystem name, size and wasm magic, and writes `SESSION/inbox/ID.json`. The gate is a stub with a fixed verdict until #519
  swaps in `oraclekit.py gate`.

## Canary checks

`canary.py` runs in the VM as root, before the agent starts, with `manifest.py` copied beside it
and the manifest built on the host from this repository's `src/`, `include/`, `orig/TotalA.exe`
and `~/.ta/TotalA.exe`. A failing check stops the session with exit 3, and so does a canary directory that is still in
the VM after `launch.py` deleted it (the agent never sees the manifest). The checks: the host's
paths (home, `~/.claude`, `~/.ta`, `~/.ssh`, `~/work`, each worktree) do not exist; no default
route; `github.com` does not resolve and direct TCP to public addresses fails; the proxy answers
403 to `github.com`, `api.github.com`, `raw.githubusercontent.com`, port 22, loopback and the
metadata address; no GitHub or SSH variables in the environment and no `.ssh`, `gh`, `.claude` or
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
