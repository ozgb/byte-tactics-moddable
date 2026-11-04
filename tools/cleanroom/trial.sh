#!/bin/sh
set -eu
start=$(date +%s)
cd /work
git config user.name >/dev/null
date +%s > trial.txt
git add trial.txt
git commit -q -m "Add: Trial file"
mkdir -p /tmp/w && cd /tmp/w
cargo new --lib --vcs none t >/dev/null 2>&1
cd t
printf '[package]\nname = "t"\nversion = "0.1.0"\nedition = "2021"\n\n[lib]\ncrate-type = ["cdylib"]\n\n[dependencies]\nitoa = "1"\n' > Cargo.toml
cargo build --target wasm32-unknown-unknown --release 2>&1 | tail -3
wasm=target/wasm32-unknown-unknown/release/t.wasm
ls -l "$wasm"
wasm-tools validate "$wasm"
mkdir -p /srv/outbox/req1
cp "$wasm" /srv/outbox/req1/module.wasm
echo parser > /srv/outbox/req1/subsystem
touch /srv/outbox/req1/ready
i=0
while [ ! -f /srv/inbox/req1.json ] && [ $i -lt 100 ]; do sleep 0.2; i=$((i + 1)); done
cat /srv/inbox/req1.json
cd /work
git branch -f session-trial HEAD
git bundle create -q /srv/outbox/.btm.tmp session-trial
mv /srv/outbox/.btm.tmp /srv/outbox/btm.bundle
echo "trial: done in $(($(date +%s) - start))s"
