#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

for board in pico pico2; do
	"$bmk" makeapp -a -r -l pico -g arm -board "$board" -heap 32k \
		-o "$work_dir/dma-advanced-$board" \
		"$module_root/examples/dma_advanced_event_queue.bmx"

	test -s "$work_dir/dma-advanced-$board.elf"
	test -s "$work_dir/dma-advanced-$board.uf2"
done

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/dma-advanced-pico2.elf")"
rg -q ' T bmx_pico_dma_configure_advanced$' <<<"$symbols"
rg -q ' T bmx_pico_dma_timer_configure$' <<<"$symbols"
rg -q ' T bmx_pico_dma_timer_unclaim$' <<<"$symbols"
