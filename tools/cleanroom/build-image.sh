#!/bin/sh
set -eu
here=$(cd "$(dirname "$0")" && pwd)
out=${1:?usage: build-image.sh OUTPUT-DIR}
smolvm=${SMOLVM:-$HOME/.local/bin/smolvm}
name=cleanroom-build-$$
mkdir -p "$out"
"$smolvm" machine create --net --name "$name" -s "$here/Smolfile"
trap '"$smolvm" machine delete --name "$name" --force >/dev/null 2>&1 || true' EXIT
"$smolvm" machine start --name "$name"
"$smolvm" machine exec --name "$name" -- sh -c 'rustc --version && wasm-tools --version && claude --version'
"$smolvm" machine stop --name "$name"
"$smolvm" pack create --from-vm "$name" -o "$out/cleanroom"
