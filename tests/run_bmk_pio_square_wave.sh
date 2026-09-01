#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
board="${PICO_TEST_BOARD:-pico}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

"$bmk" makeapp -a -r -l pico -g arm -board "$board" -heap 16k \
	-o "$work_dir/pio_square_wave" \
	"$module_root/examples/pio_square_wave.bmx"

test -s "$work_dir/pio_square_wave.elf"
test -s "$work_dir/pio_square_wave.uf2"

build_dir="$module_root/examples/.bmx/pio_square_wave.release.pico.arm.$board"
generated_header="$build_dir/generated/pio/0/pio_square_wave.pio.h"
generated_registry="$build_dir/generated/blitzmax_pio_registry.c"
test -s "$generated_header"
test -s "$generated_registry"
rg -q '#define square_wave_pio_version 0' "$generated_header"
rg -q '0xe001' "$generated_header"
rg -q '"square_wave"' "$generated_registry"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/pio_square_wave.elf" | awk 'NR == 2')
test "$text_size" -le 60000
test "$bss_size" -le 24000

symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/pio_square_wave.elf")"
rg -q ' T bmx_pico_pio_add_imported_program$' <<<"$symbols"
rg -q ' T bmx_pico_pio_claim_unused_state_machine$' <<<"$symbols"
rg -q ' T bmx_pico_pio_find_program$' <<<"$symbols"
rg -q ' T bmx_pico_pio_sm_init_imported_program$' <<<"$symbols"
rg -q ' T bmx_pico_pio_sm_set_enabled$' <<<"$symbols"

echo "Pico PIO square-wave image: board=$board text=$text_size bss=$bss_size"
