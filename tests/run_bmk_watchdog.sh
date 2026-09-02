#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

for board in pico pico2; do
	"$bmk" makeapp -a -r -l pico -g arm -board "$board" -heap 16k \
		-o "$work_dir/watchdog-$board" \
		"$module_root/examples/watchdog.bmx"

	test -s "$work_dir/watchdog-$board.elf"
	test -s "$work_dir/watchdog-$board.uf2"
done

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/watchdog-pico2.elf" | awk 'NR == 2')
test "$text_size" -le 56000
test "$bss_size" -le 24000

symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/watchdog-pico2.elf")"
rg -q ' T bmx_pico_watchdog_maximum_delay_ms$' <<<"$symbols"
rg -q ' T bmx_pico_watchdog_enable$' <<<"$symbols"
rg -q ' T bmx_pico_watchdog_disable$' <<<"$symbols"
rg -q ' T bmx_pico_watchdog_feed$' <<<"$symbols"
rg -q ' T bmx_pico_watchdog_caused_reboot$' <<<"$symbols"
rg -q ' T bmx_pico_watchdog_enable_caused_reboot$' <<<"$symbols"
rg -q ' T bmx_pico_watchdog_time_remaining_us$' <<<"$symbols"
rg -q ' T bmx_pico_watchdog_time_remaining_ms$' <<<"$symbols"
rg -q ' T bmx_pico_watchdog_reboot$' <<<"$symbols"

echo "Pico watchdog images passed: text=$text_size bss=$bss_size"
