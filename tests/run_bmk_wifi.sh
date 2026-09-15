#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

for board in pico_w pico2_w; do
	"$bmk" makeapp -a -r -l pico -g arm -board "$board" -heap 32k \
		-o "$work_dir/wifi-scan-$board" "$module_root/examples/wifi_scan.bmx"
	test -s "$work_dir/wifi-scan-$board.elf"
	test -s "$work_dir/wifi-scan-$board.uf2"
	"$bmk" makeapp -a -r -l pico -g arm -board "$board" -heap 32k \
		-o "$work_dir/wifi-connect-$board" "$module_root/examples/wifi_connect.bmx"
	test -s "$work_dir/wifi-connect-$board.elf"
done

if "$bmk" makeapp -a -r -l pico -g arm -board pico2 -heap 32k \
		-o "$work_dir/wifi-unsupported" "$module_root/examples/wifi_scan.bmx" \
		>"$work_dir/wifi-unsupported.log" 2>&1; then
	echo "Pico.Network.WiFi unexpectedly built for pico2" >&2
	exit 1
fi
rg -q "CYW43 wireless support" \
	"$work_dir/wifi-unsupported.log"

"$bmk" makeapp -a -r -l pico -g arm -board pico2_w -heap 32k \
	-o "$work_dir/no-wifi" "$module_root/tests/fixtures/event_queue_compile.bmx"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

wifi_symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/wifi-scan-pico2_w.elf")"
rg -q ' T bmx_embedded_wifi_initialize$' <<<"$wifi_symbols"
rg -q ' T bmx_embedded_wifi_start_scan$' <<<"$wifi_symbols"
rg -q ' T cyw43_wifi_scan$' <<<"$wifi_symbols"
rg -q ' T dhcp_start$' <<<"$wifi_symbols"

connect_symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/wifi-connect-pico2_w.elf")"
rg -q ' T bmx_embedded_wifi_connect$' <<<"$connect_symbols"
rg -q ' T dhcp_start$' <<<"$connect_symbols"

no_wifi_symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/no-wifi.elf")"
if rg -q ' (bmx_embedded_wifi_|cyw43_wifi_scan$)' <<<"$no_wifi_symbols"; then
	echo "Wireless driver symbols leaked into an application which does not import Pico.Network.WiFi" >&2
	exit 1
fi

for board in pico_w pico2_w; do
	read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" \
		"$work_dir/wifi-scan-$board.elf" | awk 'NR == 2')
	test "$text_size" -le 370000
	test "$bss_size" -le 70000
	echo "Pico WiFi scan image ($board): text=$text_size bss=$bss_size"
done
