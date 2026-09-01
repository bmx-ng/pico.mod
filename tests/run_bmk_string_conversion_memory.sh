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
	-o "$work_dir/string_integer_conversion" \
	"$module_root/examples/string_integer_conversion.bmx"

"$bmk" makeapp \
	-a -r \
	-l pico -g arm -board pico2 -heap 16k \
	-o "$work_dir/string_float_conversion" \
	"$module_root/examples/string_float_conversion.bmx"

"$bmk" makeapp \
	-a -r \
	-l pico -g arm -board pico2 -heap 16k \
	-o "$work_dir/string_conversion_memory" \
	"$module_root/examples/string_conversion_memory.bmx"

test -s "$work_dir/string_conversion_memory.elf"
test -s "$work_dir/string_conversion_memory.uf2"
test -s "$work_dir/string_integer_conversion.elf"
test -s "$work_dir/string_float_conversion.elf"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

read -r integer_text_size _ integer_bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/string_integer_conversion.elf" | awk 'NR == 2')
test "$integer_text_size" -le 60000
test "$integer_bss_size" -le 24000
if "$toolchain/bin/arm-none-eabi-nm" "$work_dir/string_integer_conversion.elf" | rg -q 'fast_float|f2s_buffered|d2s_buffered|__real_snprintf|bmx_pico_string_(from|to)_(float|double)'; then
	echo "Floating-point conversion code leaked into the integer-only image" >&2
	exit 1
fi

read -r float_text_size _ float_bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/string_float_conversion.elf" | awk 'NR == 2')
test "$float_text_size" -le 105000
test "$float_bss_size" -le 24000
if "$toolchain/bin/arm-none-eabi-nm" "$work_dir/string_float_conversion.elf" | rg -q '__real_snprintf|bmx_pico_string_from_(float|double)_fixed'; then
	echo "Fixed formatting leaked into the default floating-point image" >&2
	exit 1
fi

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/string_conversion_memory.elf" | awk 'NR == 2')
test "$text_size" -le 125000
test "$bss_size" -le 24000

echo "Pico integer String image: text=$integer_text_size bss=$integer_bss_size"
echo "Pico default floating String image: text=$float_text_size bss=$float_bss_size"
echo "Pico complete String conversion image: text=$text_size bss=$bss_size"
