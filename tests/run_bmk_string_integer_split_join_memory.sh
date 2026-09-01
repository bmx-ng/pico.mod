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
	-o "$work_dir/string_integer_split_join_memory" \
	"$module_root/examples/string_integer_split_join_memory.bmx"

test -s "$work_dir/string_integer_split_join_memory.elf"
test -s "$work_dir/string_integer_split_join_memory.uf2"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/string_integer_split_join_memory.elf" | awk 'NR == 2')
test "$text_size" -le 45000
test "$bss_size" -le 24000

symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/string_integer_split_join_memory.elf")"
for suffix in ints bytes shorts uints longs ulongs sizes long_ints ulong_ints; do
	rg -q " bmx_pico_string_split_${suffix}$" <<<"$symbols"
	rg -q " bmx_pico_string_join_${suffix}$" <<<"$symbols"
done
if rg -q 'fast_float|f2s_buffered|d2s_buffered|__real_snprintf' <<<"$symbols"; then
	echo "Floating-point conversion code leaked into the integer Split/Join image" >&2
	exit 1
fi

echo "Pico integer String Split/Join image: text=$text_size bss=$bss_size"
