#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
board="${PICO_TEST_BOARD:-pico}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

"$bmk" makeapp -a -r -l pico -g arm -board "$board" -heap 16k \
	-o "$work_dir/pio_fifo_led" \
	"$module_root/examples/pio_fifo_led.bmx"

test -s "$work_dir/pio_fifo_led.elf"
test -s "$work_dir/pio_fifo_led.uf2"

build_dir="$module_root/examples/.bmx/pio_fifo_led.release.pico.arm.$board"
generated_header="$build_dir/generated/pio/0/pio_fifo_led.pio.h"
generated_registry="$build_dir/generated/blitzmax_pio_registry.c"
test -s "$generated_header"
test -s "$generated_registry"
rg -q '#define fifo_led_pio_version 0' "$generated_header"
rg -q '[.]length = 6' "$generated_header"
rg -q '0xa0c7.*mov    isr, osr' "$generated_header"
rg -q '0x8020.*push   block' "$generated_header"
rg -q 'sm_config_set_wrap' "$generated_header"
rg -q '"fifo_led"' "$generated_registry"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/pio_fifo_led.elf" | awk 'NR == 2')
test "$text_size" -le 62000
test "$bss_size" -le 24000

symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/pio_fifo_led.elf")"
rg -q ' T bmx_pico_pio_sm_get$' <<<"$symbols"
rg -q ' T bmx_pico_pio_sm_put$' <<<"$symbols"
rg -q ' T bmx_pico_pio_sm_put_blocking$' <<<"$symbols"
rg -q ' T bmx_pico_pio_sm_rx_empty$' <<<"$symbols"
rg -q ' T bmx_pico_pio_sm_rx_level$' <<<"$symbols"
rg -q ' T bmx_pico_pio_sm_tx_full$' <<<"$symbols"
rg -q ' T bmx_pico_pio_sm_tx_level$' <<<"$symbols"

echo "Pico PIO FIFO image: board=$board text=$text_size bss=$bss_size"
