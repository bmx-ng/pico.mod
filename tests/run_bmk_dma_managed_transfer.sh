#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
board="${PICO_TEST_BOARD:-pico}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

"$bmk" makeapp -a -r -l pico -g arm -board "$board" -heap 16k \
	-o "$work_dir/dma_managed_transfer" \
	"$module_root/examples/dma_managed_transfer.bmx"

test -s "$work_dir/dma_managed_transfer.elf"
test -s "$work_dir/dma_managed_transfer.uf2"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/dma_managed_transfer.elf" | awk 'NR == 2')
test "$text_size" -le 76000
test "$bss_size" -le 28000

symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/dma_managed_transfer.elf")"
rg -q ' T bmx_pico_dma_configure$' <<<"$symbols"
rg -q ' T bmx_pico_dma_abort$' <<<"$symbols"
rg -q ' T bmx_pico_dma_unclaim_channel$' <<<"$symbols"
rg -q ' T bmx_pico_collect_objects$' <<<"$symbols"

echo "Pico managed DMA image: board=$board text=$text_size bss=$bss_size"
