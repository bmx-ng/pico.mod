#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

"$bmk" makeapp -a -r -l pico -g arm -board pico2 -heap 16k \
	-o "$work_dir/string_float_split_join_memory" \
	"$module_root/examples/string_float_split_join_memory.bmx"
"$bmk" makeapp -a -r -l pico -g arm -board pico2 -heap 16k \
	-o "$work_dir/string_float_join_fixed" \
	"$module_root/examples/string_float_join_fixed.bmx"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/string_float_split_join_memory.elf" | awk 'NR == 2')
test "$text_size" -le 90000
test "$bss_size" -le 24000
symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/string_float_split_join_memory.elf")"
for symbol in split_floats split_doubles join_floats_default join_doubles_default; do
	rg -q " bmx_pico_string_${symbol}$" <<<"$symbols"
done
if rg -q ' T snprintf$|bmx_pico_string_join_(floats|doubles)_fixed' <<<"$symbols"; then
	echo "Fixed formatting leaked into the default floating Split/Join image" >&2
	exit 1
fi

read -r fixed_text_size _ fixed_bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/string_float_join_fixed.elf" | awk 'NR == 2')
test "$fixed_text_size" -le 65000
test "$fixed_bss_size" -le 24000
fixed_symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/string_float_join_fixed.elf")"
rg -q ' bmx_pico_string_join_floats_fixed$' <<<"$fixed_symbols"
rg -q ' bmx_pico_string_join_doubles_fixed$' <<<"$fixed_symbols"
rg -q ' T snprintf$' <<<"$fixed_symbols"

echo "Pico floating String Split/Join image: text=$text_size bss=$bss_size"
echo "Pico fixed floating String Join image: text=$fixed_text_size bss=$fixed_bss_size"
