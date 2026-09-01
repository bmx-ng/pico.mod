#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

"$bmk" makeapp \
	-a -r \
	-l pico -g arm -board pico2 -heap 16k \
	-o "$work_dir/inheritance_memory" \
	"$module_root/examples/inheritance_memory.bmx"

test -s "$work_dir/inheritance_memory.elf"
test -s "$work_dir/inheritance_memory.uf2"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/inheritance_memory.elf" | awk 'NR == 2')
test "$text_size" -le 39000
test "$bss_size" -ge 19000
test "$bss_size" -le 21000
