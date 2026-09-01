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
	-o "$work_dir/generic_scalar_memory" \
	"$module_root/examples/generic_scalar_memory.bmx"

test -s "$work_dir/generic_scalar_memory.elf"
test -s "$work_dir/generic_scalar_memory.uf2"

build_dir="$module_root/examples/.bmx/generic_scalar_memory.release.pico.arm.pico2"
manifest="$build_dir/bcc/application/bcc-build.manifest"
test -s "$manifest"
test "$(grep -c '^file generic-specialization-c ' "$manifest")" -eq 5
test "$(grep -c '^link ' "$manifest")" -eq 5

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/generic_scalar_memory.elf" | awk 'NR == 2')
test "$text_size" -le 40000
# The managed generic Struct tier imports Pico.Runtime.Memory and therefore
# includes the configured 16 KiB collector arena in BSS.
test "$bss_size" -le 24000

echo "Pico generic scalar/Struct image: text=$text_size bss=$bss_size"
