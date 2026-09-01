#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

"$bmk" makeapp -a -r -l pico -g arm -board pico2 -heap 16k \
	-o "$work_dir/adc_temperature" \
	"$module_root/examples/adc_temperature.bmx"

test -s "$work_dir/adc_temperature.elf"
test -s "$work_dir/adc_temperature.uf2"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/adc_temperature.elf" | awk 'NR == 2')
test "$text_size" -le 52000
test "$bss_size" -le 24000

symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/adc_temperature.elf")"
rg -q ' T bmx_pico_adc_read$' <<<"$symbols"
rg -q ' T bmx_pico_adc_fifo_setup$' <<<"$symbols"
rg -q ' T bmx_pico_adc_fifo_get$' <<<"$symbols"
rg -q ' T bmx_pico_adc_fifo_get_blocking$' <<<"$symbols"

echo "Pico ADC temperature image: text=$text_size bss=$bss_size"
