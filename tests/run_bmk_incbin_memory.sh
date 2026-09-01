#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
board="${PICO_TEST_BOARD:-pico}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

"$bmk" makeapp \
	-a -r \
	-l pico -g arm -board "$board" -heap 16k \
	-o "$work_dir/incbin_memory" \
	"$module_root/examples/incbin_memory.bmx"

test -s "$work_dir/incbin_memory.elf"
test -s "$work_dir/incbin_memory.uf2"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

symbol_address="$($toolchain/bin/arm-none-eabi-nm -n "$work_dir/incbin_memory.elf" | awk '/_ib_bb_main_1_data$/ { print $1; exit }')"
test -n "$symbol_address"
case "$symbol_address" in
	10*) ;;
	*) echo "Incbin payload is not linked into Pico XIP flash: $symbol_address" >&2; exit 1 ;;
esac

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/incbin_memory.elf" | awk 'NR == 2')
test "$text_size" -le 65000
test "$bss_size" -le 24000

echo "Pico Incbin image: board=$board text=$text_size bss=$bss_size payload=0x$symbol_address"
