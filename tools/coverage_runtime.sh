#!/bin/sh
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
version=$(sed -n "s/^set(MODERN_LLVM_VERSION \(.*\))$/\1/p" "$root/cmake/clang-cl-i686.cmake")
dest=${1:-$root/build/compiler-rt}
if [ -d "$dest/compiler-rt/lib/profile" ]; then
  exit 0
fi
git clone -q -c advice.detachedHead=false --depth 1 --branch "llvmorg-$version" --filter=blob:none --sparse https://github.com/llvm/llvm-project.git "$dest"
git -C "$dest" sparse-checkout set compiler-rt/lib/profile compiler-rt/include/profile
