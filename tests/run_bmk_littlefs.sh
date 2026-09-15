#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

for board in pico pico2; do
	"$bmk" makeapp -a -r -l pico -g arm -board "$board" -heap 32k -storage 128k \
		-o "$work_dir/littlefs-$board" \
		"$module_root/tests/fixtures/littlefs_compile.bmx"

	test -s "$work_dir/littlefs-$board.elf"
	test -s "$work_dir/littlefs-$board.uf2"
done

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

for board in pico pico2; do
	symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/littlefs-$board.elf")"
	rg -q ' T bmx_pico_littlefs_mount$' <<<"$symbols"
	rg -q ' T bmx_pico_littlefs_open$' <<<"$symbols"
	rg -q ' T bmx_pico_littlefs_directory_open$' <<<"$symbols"

	read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/littlefs-$board.elf" | awk 'NR == 2')
	test "$text_size" -le 150000
	echo "Pico LittleFS image ($board): text=$text_size bss=$bss_size"
done
