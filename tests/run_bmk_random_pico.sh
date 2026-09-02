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
		-o "$work_dir/random_pico-$board" \
		"$module_root/examples/random_pico.bmx"

	test -s "$work_dir/random_pico-$board.elf"
	test -s "$work_dir/random_pico-$board.uf2"
done

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

for board in pico pico2; do
	read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/random_pico-$board.elf" | awk 'NR == 2')
	test "$text_size" -le 48000
	test "$bss_size" -ge 19000
	test "$bss_size" -le 22000
	echo "Pico SDK random image ($board): text=$text_size bss=$bss_size"
done
