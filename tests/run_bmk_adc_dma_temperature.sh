#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
board="${PICO_TEST_BOARD:-pico}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

"$bmk" makeapp -a -r -l pico -g arm -board "$board" -heap 16k \
	-o "$work_dir/adc_dma_temperature" \
	"$module_root/examples/adc_dma_temperature.bmx"

test -s "$work_dir/adc_dma_temperature.elf"
test -s "$work_dir/adc_dma_temperature.uf2"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/adc_dma_temperature.elf" | awk 'NR == 2')
test "$text_size" -le 68000
test "$bss_size" -le 26000

symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/adc_dma_temperature.elf")"
rg -q ' T bmx_pico_adc_fifo_address$' <<<"$symbols"
rg -q ' T bmx_pico_adc_dreq$' <<<"$symbols"
rg -q ' T bmx_pico_dma_configure$' <<<"$symbols"
rg -q ' T bmx_pico_dma_take_completion_events$' <<<"$symbols"

echo "Pico ADC DMA image: board=$board text=$text_size bss=$bss_size"
