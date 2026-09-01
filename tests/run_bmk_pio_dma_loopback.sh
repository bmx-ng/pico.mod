#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
board="${PICO_TEST_BOARD:-pico}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

"$bmk" makeapp -a -r -l pico -g arm -board "$board" -heap 16k \
	-o "$work_dir/pio_dma_loopback" \
	"$module_root/examples/pio_dma_loopback.bmx"

test -s "$work_dir/pio_dma_loopback.elf"
test -s "$work_dir/pio_dma_loopback.uf2"

build_dir="$module_root/examples/.bmx/pio_dma_loopback.release.pico.arm.$board"
generated_header="$build_dir/generated/pio/0/pio_dma_loopback.pio.h"
test -s "$generated_header"
rg -q '#define dma_loopback_pio_version 0' "$generated_header"
rg -q '[.]length = 6' "$generated_header"
rg -q 'push   block' "$generated_header"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/pio_dma_loopback.elf" | awk 'NR == 2')
test "$text_size" -le 68000
test "$bss_size" -le 26000

symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/pio_dma_loopback.elf")"
rg -q ' T bmx_pico_dma_configure$' <<<"$symbols"
rg -q ' T bmx_pico_dma_set_irq_enabled$' <<<"$symbols"
rg -q ' T bmx_pico_dma_take_completion_events$' <<<"$symbols"
rg -q ' T bmx_pico_pio_sm_dreq$' <<<"$symbols"
rg -q ' T bmx_pico_pio_sm_rx_fifo_address$' <<<"$symbols"
rg -q ' T bmx_pico_pio_sm_tx_fifo_address$' <<<"$symbols"

echo "Pico PIO DMA image: board=$board text=$text_size bss=$bss_size"
