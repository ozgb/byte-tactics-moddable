# Running a lead on your own machine

A lead is a Claude Code session that takes issues from the plan in #2, implements each one through subagents and opens a pull request for the maintainer to review. This page sets one up on your machine, under your own GitHub account. `docs/team.md` has how the leads and the maintainer work together.

## What you need

- Linux (or WSL) with the build tools in `CONTRIBUTING.md`, "Setting up": clang and LLVM **22.1.8**, `cmake`, `ninja`, `wine`, `7z`, `cabextract`, `curl`, `git`, `gh` and [uv](https://docs.astral.sh/uv/).
- Total Annihilation from Steam, installed.
- Claude Code, logged in, with Opus.
- From the human who runs the maintainer:
  - a GitHub collaborator invite to `ozgb/byte-tactics-moddable` (accept it at <https://github.com/ozgb/byte-tactics-moddable/invitations>);
  - your lead name, `lead-c`, `lead-d` and so on, unique among the leads;
  - the two lines of `rest.env`, the credentials for the leads' and maintainer's notes. Keep them private: they give full access to the notes database.

## 1. Clone and set up

```sh
gh auth login                       # your own account
git clone https://github.com/ozgb/byte-tactics-moddable ~/source/repos/byte-tactics-moddable-lead
cd ~/source/repos/byte-tactics-moddable-lead
tools/setup_toolchain.sh            # MSVC 5 headers into toolchain/, TotalA.exe from Steam (set STEAM_TA if it is elsewhere)
clang-cl --version                  # must report 22.1.8
```

Commit as your own account, never with another person's name or email. Set the identity in every clone the lead commits from, ozgb/btm's included (worktrees share their clone's setting):

```sh
git config user.name "<your name>"
git config user.email "<id>+<account>@users.noreply.github.com"   # your noreply address, from GitHub's Settings, Emails
```

After the lead's first commit, and before its first push, check that the author is your account:

```sh
git log -1 --format='%an <%ae>'
```

The attribution line stays as `AGENTS.md`, "Writing style", says: each commit message ends with `Assisted-by: <agent>:<model>`, and the author is your account.

## 2. Save the notes credentials

Paste the two lines you were sent:

```sh
mkdir -p ~/.local/share/bt-upstash
install -m 600 /dev/stdin ~/.local/share/bt-upstash/rest.env <<'EOF'
UPSTASH_REDIS_REST_URL=https://<host>.upstash.io
UPSTASH_REDIS_REST_TOKEN=<token>
EOF
uv run tools/notes.py PING          # prints PONG
```

## 3. Check that the gate runs

The lead's subagents run this before every pull request, so it has to pass on your machine first. Wine runs the game as a 32-bit program, which needs i386 graphics libraries. On a WSL (Ubuntu) machine where every run ended at the `dialog-directx` error, these were missing (not yet confirmed to be all of it):

```sh
sudo dpkg --add-architecture i386 && sudo apt update
sudo apt install libgl1:i386 libgl1-mesa-dri:i386 libxrandr2:i386 libxinerama1:i386 libxcursor1:i386 libxi6:i386
```

On WSL, `/tmp/.X11-unix` is WSLg's read-only mount, and Xvfb has started there only with `-nolisten unix`.

```sh
export WINEARCH=win64
cmake -S . -B build/modern -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/clang-cl-i686.cmake
cmake --build build/modern
cmake --build build/modern --target wasm_runtime
uv run tools/reference.py --check --tier --exe build/modern/TotalA.exe
```

It must end with every tier scenario equal. If it doesn't, stop and tell the human, with the output.

## 4. Start the lead

```sh
cd ~/source/repos/byte-tactics-moddable-lead
BT_ROLE=lead BT_LEAD_NAME=lead-c claude
```

Then type `/lead`. It reads its notes, picks up issues, claims them as `lead-c` and opens pull requests; leave it running. Set `BT_LEAD_LANE=<parent issue>` as well if the human gives you a lane.

The [bwai sandbox](https://github.com/ozgb/bubblewrap-ai) is recommended: run the same command inside it. Without it, Claude Code asks before each command it runs.

To stop for good, first tell it to release the issues it holds, so another lead can take them.
