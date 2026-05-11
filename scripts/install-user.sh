#!/bin/sh
set -eu

if [ "$#" -ne 4 ]; then
    echo "usage: install-user.sh RELEASE_MASH16 DEBUG_MASH16 DEBUG_MASH16_INSPECT USER_BIN" >&2
    exit 2
fi

release_mash16=$1
debug_mash16=$2
debug_inspect=$3
user_bin=$4

check_executable() {
    path=$1
    label=$2
    hint=$3

    if [ ! -x "$path" ]; then
        echo "error: missing executable $label: $path" >&2
        echo "hint: $hint" >&2
        exit 1
    fi
}

check_executable "$release_mash16" "release mash16" "build it with: ninja -C build mash16"
check_executable "$debug_mash16" "debug mash16" "build it with: ninja -C build-debug mash16"
check_executable "$debug_inspect" "debug mash16-inspect" "build it with: ninja -C build-debug mash16-inspect"

mkdir -p "$user_bin"
cp "$release_mash16" "$user_bin/mash16"
cp "$debug_inspect" "$user_bin/mash16-inspect"
ln -sfn "$debug_mash16" "$user_bin/mash16-debug"

echo "installed $user_bin/mash16"
echo "installed $user_bin/mash16-inspect"
echo "installed $user_bin/mash16-debug -> $debug_mash16"
