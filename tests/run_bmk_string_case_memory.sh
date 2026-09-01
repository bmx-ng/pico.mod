#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

"$bmk" makeapp -a -r -l pico -g arm -board pico2 -heap 16k \
	-o "$work_dir/string_case_memory" \
	"$module_root/examples/string_case_memory.bmx"
"$bmk" makeapp -a -r -l pico -g arm -board pico2 -heap 16k \
	-o "$work_dir/string_unicode_case_memory" \
	"$module_root/examples/string_unicode_case_memory.bmx"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

read -r compact_text _ compact_bss _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/string_case_memory.elf" | awk 'NR == 2')
test "$compact_text" -le 42000
test "$compact_bss" -le 24000
compact_symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/string_case_memory.elf")"
for symbol in compare_case equals_case hash_case; do
	rg -q " bmx_pico_string_${symbol}$" <<<"$compact_symbols"
done
if rg -q 'bmx_pico_unicode_enable|bmx_pico_to_(lower|upper)_data|bmx_pico_unicode_fold_character' <<<"$compact_symbols"; then
	echo "Optional Unicode case implementation leaked into the compact String image" >&2
	exit 1
fi

read -r unicode_text _ unicode_bss _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/string_unicode_case_memory.elf" | awk 'NR == 2')
test "$unicode_text" -le 70000
test "$unicode_bss" -le 24000
unicode_symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/string_unicode_case_memory.elf")"
rg -q ' T bmx_pico_unicode_enable$' <<<"$unicode_symbols"
rg -q ' r bmx_pico_to_lower_data$' <<<"$unicode_symbols"
rg -q ' r bmx_pico_to_upper_data$' <<<"$unicode_symbols"
rg -q ' T bmx_pico_unicode_fold_character$' <<<"$unicode_symbols"
test $((unicode_bss - compact_bss)) -le 512

echo "Pico compact String case image: text=$compact_text bss=$compact_bss"
echo "Pico optional Unicode String case image: text=$unicode_text bss=$unicode_bss"
