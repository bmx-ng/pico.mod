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
	-o "$work_dir/string_encoding_memory" \
	"$module_root/examples/string_encoding_memory.bmx"

test -s "$work_dir/string_encoding_memory.elf"
test -s "$work_dir/string_encoding_memory.uf2"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/string_encoding_memory.elf" | awk 'NR == 2')
test "$text_size" -le 56000
test "$bss_size" -le 24000

for symbol in \
	bmx_pico_string_from_c_string \
	bmx_pico_string_from_w_string \
	bmx_pico_string_from_utf8_string \
	bmx_pico_string_to_w_string_buffer \
	bmx_pico_string_to_utf8_string_buffer \
	bmx_pico_string_to_utf32_string \
	bmx_pico_string_from_utf32_string \
	bmx_pico_string_from_bytes_as_hex \
	bmx_pico_string_to_bytes_from_hex_ex; do
	"$toolchain/bin/arm-none-eabi-nm" "$work_dir/string_encoding_memory.elf" | rg -q " $symbol$"
done

echo "Pico String encoding image: text=$text_size bss=$bss_size"
