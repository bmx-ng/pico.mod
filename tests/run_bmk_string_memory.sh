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
	-o "$work_dir/string_memory" \
	"$module_root/examples/string_memory.bmx"

test -s "$work_dir/string_memory.elf"
test -s "$work_dir/string_memory.uf2"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/string_memory.elf" | awk 'NR == 2')
test "$text_size" -le 41000
test "$bss_size" -ge 19000
test "$bss_size" -le 21000

if "$toolchain/bin/arm-none-eabi-nm" "$work_dir/string_memory.elf" | rg -q 'bmx_pico_unicode_(enable|fold_character|to_lower|to_upper)|bmx_pico_to_(lower|upper)_data'; then
	echo "Optional Unicode casing leaked into the core String image" >&2
	exit 1
fi

echo "Pico core String methods image: text=$text_size bss=$bss_size"
