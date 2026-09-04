#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

for board in pico pico2; do
	"$bmk" makeapp -a -r -l pico -g arm -board "$board" -heap 16k \
		-o "$work_dir/power-$board" \
		"$module_root/tests/fixtures/power_compile.bmx"

	test -s "$work_dir/power-$board.elf"
	test -s "$work_dir/power-$board.uf2"
done

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

for board in pico pico2; do
	symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/power-$board.elf")"
	rg -q ' T bmx_pico_power_capabilities$' <<<"$symbols"
	rg -q ' T bmx_pico_power_idle$' <<<"$symbols"
	rg -q ' T bmx_pico_power_sleep_for_ms$' <<<"$symbols"
	rg -q ' T bmx_pico_power_dormant_until_gpio$' <<<"$symbols"

	read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/power-$board.elf" | awk 'NR == 2')
	test "$text_size" -le 75000
	test "$bss_size" -le 24000
	echo "Pico low-power image ($board): text=$text_size bss=$bss_size"
done
