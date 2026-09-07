#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

# The public API remains importable on RP2040 and reports that PSRAM is absent.
"$bmk" makeapp -a -r -l pico -g arm -board pico -heap 16k \
	-o "$work_dir/psram-info-rp2040" "$module_root/examples/psram_info.bmx"

# Importing the API links SDK PSRAM initialization without moving the managed
# heap unless the application explicitly requests that placement.
"$bmk" makeapp -a -r -l pico -g arm -board pimoroni_pico_plus2_rp2350 \
	-heap 16k -o "$work_dir/psram-info-rp2350" "$module_root/examples/psram_info.bmx"

"$bmk" makeapp -a -r -l pico -g arm -board pimoroni_pico_plus2_rp2350 \
	-heap-region psram -heap auto -o "$work_dir/psram-heap" \
	"$module_root/tests/fixtures/psram_heap.bmx"

test -s "$work_dir/psram-info-rp2040.uf2"
test -s "$work_dir/psram-info-rp2350.uf2"
test -s "$work_dir/psram-heap.uf2"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

psram_section_size=$("$toolchain/bin/arm-none-eabi-size" -A "$work_dir/psram-heap.elf" | awk '$1 == ".psram_noload" { print $2 }')
test "$psram_section_size" -eq 8323072

symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/psram-heap.elf")"
rg -q ' T bmx_pico_psram_available$' <<<"$symbols"
rg -q ' T bmx_pico_psram_capacity$' <<<"$symbols"
rg -q ' T bmx_pico_psram_contains$' <<<"$symbols"

if "$toolchain/bin/arm-none-eabi-size" -A "$work_dir/psram-info-rp2350.elf" | \
	awk '$1 == ".psram_noload" && $2 != 0 { found = 1 } END { exit found ? 0 : 1 }'; then
	echo "PSRAM API-only build unexpectedly placed its managed heap in PSRAM" >&2
	exit 1
fi

echo "Pico PSRAM build checks passed (RP2040 stubs, 8 MiB board API, 8128 KiB managed heap)"
