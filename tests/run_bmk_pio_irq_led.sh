#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
board="${PICO_TEST_BOARD:-pico}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

"$bmk" makeapp -a -r -l pico -g arm -board "$board" -heap 16k \
	-o "$work_dir/pio_irq_led" \
	"$module_root/examples/pio_irq_led.bmx"

test -s "$work_dir/pio_irq_led.elf"
test -s "$work_dir/pio_irq_led.uf2"

build_dir="$module_root/examples/.bmx/pio_irq_led.release.pico.arm.$board"
generated_header="$build_dir/generated/pio/0/pio_irq_led.pio.h"
generated_registry="$build_dir/generated/blitzmax_pio_registry.c"
test -s "$generated_header"
test -s "$generated_registry"
rg -q '#define irq_led_pio_version 0' "$generated_header"
rg -q '[.]length = 5' "$generated_header"
rg -q 'irq    nowait 0' "$generated_header"
rg -q '"irq_led"' "$generated_registry"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/pio_irq_led.elf" | awk 'NR == 2')
test "$text_size" -le 64000
test "$bss_size" -le 25000

symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/pio_irq_led.elf")"
rg -q ' T bmx_pico_pio_interrupt_clear$' <<<"$symbols"
rg -q ' T bmx_pico_pio_interrupt_is_set$' <<<"$symbols"
rg -q ' T bmx_pico_pio_irq_pending_events$' <<<"$symbols"
rg -q ' T bmx_pico_pio_irq_rearm_sources$' <<<"$symbols"
rg -q ' T bmx_pico_pio_irq_set_sources_enabled$' <<<"$symbols"
rg -q ' T bmx_pico_pio_irq_take_events$' <<<"$symbols"

echo "Pico PIO IRQ image: board=$board text=$text_size bss=$bss_size"
