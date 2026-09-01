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
	-o "$work_dir/collections_stack" \
	"$module_root/examples/collections_stack.bmx"

test -s "$work_dir/collections_stack.elf"
test -s "$work_dir/collections_stack.uf2"

build_dir="$module_root/examples/.bmx/collections_stack.release.pico.arm.pico2"
manifest="$build_dir/bcc/application/bcc-build.manifest"
test -s "$manifest"
test "$(grep -c '^file generic-specialization-c ' "$manifest")" -eq 5
test "$(grep -c '^link ' "$manifest")" -eq 5

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/collections_stack.elf" | awk 'NR == 2')
test "$text_size" -le 55000
test "$bss_size" -le 24000

echo "Pico Collections.Stack image: text=$text_size bss=$bss_size"
