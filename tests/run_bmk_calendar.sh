#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

for board in pico pico2; do
	"$bmk" makeapp \
		-a -r \
		-l pico -g arm -board "$board" -heap 16k \
		-o "$work_dir/calendar-$board" \
		"$module_root/examples/calendar.bmx"

	test -s "$work_dir/calendar-$board.elf"
	test -s "$work_dir/calendar-$board.uf2"

	"$bmk" makeapp \
		-a -r \
		-l pico -g arm -board "$board" -heap 32k \
		-o "$work_dir/calendar-unit-$board" \
		"$module_root/tests/fixtures/calendar_unit.bmx"

	test -s "$work_dir/calendar-unit-$board.elf"
	test -s "$work_dir/calendar-unit-$board.uf2"
done

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

for board in pico pico2; do
	read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/calendar-$board.elf" | awk 'NR == 2')
	test "$text_size" -le 60000
	test "$bss_size" -le 24000

	symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/calendar-$board.elf")"
	rg -q ' T bmx_pico_calendar_start$' <<<"$symbols"
	rg -q ' T bmx_pico_calendar_get$' <<<"$symbols"
	rg -q ' T bmx_pico_calendar_set_alarm$' <<<"$symbols"
	rg -q ' T bmx_current_unix_time$' <<<"$symbols"
	rg -q ' T bmx_current_datetime_format$' <<<"$symbols"

	echo "Pico calendar image ($board): text=$text_size bss=$bss_size"

	read -r unit_text_size _ unit_bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/calendar-unit-$board.elf" | awk 'NR == 2')
	test "$unit_text_size" -le 85000
	test "$unit_bss_size" -le 42000
	echo "Pico calendar unit-test image ($board): text=$unit_text_size bss=$unit_bss_size"
done
