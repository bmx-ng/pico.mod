#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

for board in pico_w pico2_w; do
	"$bmk" makeapp -a -r -l pico -g arm -board "$board" -heap 32k \
		-o "$work_dir/wifi-tcp-client-$board" \
		"$module_root/examples/wifi_tcp_client.bmx"
	test -s "$work_dir/wifi-tcp-client-$board.elf"
	test -s "$work_dir/wifi-tcp-client-$board.uf2"
done

if "$bmk" makeapp -a -r -l pico -g arm -board pico2 -heap 32k \
		-o "$work_dir/socket-unsupported" \
		"$module_root/examples/wifi_tcp_client.bmx" \
		>"$work_dir/socket-unsupported.log" 2>&1; then
	echo "Pico socket networking unexpectedly built for pico2" >&2
	exit 1
fi
rg -q "Pico Wi-Fi and socket networking require" \
	"$work_dir/socket-unsupported.log"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

symbols="$("$toolchain/bin/arm-none-eabi-nm" \
	"$work_dir/wifi-tcp-client-pico2_w.elf")"
rg -q ' T socket_$' <<<"$symbols"
rg -q ' T connect_$' <<<"$symbols"
rg -q ' T send_$' <<<"$symbols"
rg -q ' T recv_$' <<<"$symbols"
rg -q ' T getaddrinfo_$' <<<"$symbols"
rg -q ' T bmx_pico_net_active_socket_count$' <<<"$symbols"

for board in pico_w pico2_w; do
	read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" \
		"$work_dir/wifi-tcp-client-$board.elf" | awk 'NR == 2')
	test "$text_size" -le 395000
	test "$bss_size" -le 75000
	echo "Pico TCP client image ($board): text=$text_size bss=$bss_size"
done
