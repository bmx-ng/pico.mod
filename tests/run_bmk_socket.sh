#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

examples=(wifi_tcp_client wifi_tcp_server wifi_udp_ntp)

for board in pico_w pico2_w; do
	for example in "${examples[@]}"; do
		"$bmk" makeapp -a -r -l pico -g arm -board "$board" -heap 32k \
			-o "$work_dir/$example-$board" \
			"$module_root/examples/$example.bmx"
		test -s "$work_dir/$example-$board.elf"
		test -s "$work_dir/$example-$board.uf2"
	done
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

client_symbols="$("$toolchain/bin/arm-none-eabi-nm" \
	"$work_dir/wifi_tcp_client-pico2_w.elf")"
server_symbols="$("$toolchain/bin/arm-none-eabi-nm" \
	"$work_dir/wifi_tcp_server-pico2_w.elf")"
udp_symbols="$("$toolchain/bin/arm-none-eabi-nm" \
	"$work_dir/wifi_udp_ntp-pico2_w.elf")"
rg -q ' T socket_$' <<<"$client_symbols"
rg -q ' T connect_$' <<<"$client_symbols"
rg -q ' T send_$' <<<"$client_symbols"
rg -q ' T recv_$' <<<"$client_symbols"
rg -q ' T getaddrinfo_$' <<<"$client_symbols"
rg -q ' T bmx_embedded_net_active_socket_count$' <<<"$client_symbols"
rg -q ' T listen_$' <<<"$server_symbols"
rg -q ' T bmx_stdc_accept_$' <<<"$server_symbols"
rg -q ' T bmx_net_set_event_tokens$' <<<"$server_symbols"
rg -q ' T sendto_$' <<<"$udp_symbols"
rg -q ' T recvfrom_$' <<<"$udp_symbols"

for board in pico_w pico2_w; do
	for example in "${examples[@]}"; do
		read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" \
			"$work_dir/$example-$board.elf" | awk 'NR == 2')
		test "$text_size" -le 400000
		test "$bss_size" -le 75000
		echo "Pico socket image ($example, $board): text=$text_size bss=$bss_size"
	done
done
