#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

for board in pico pico2; do
	output="$work_dir/allocation_failure_memory-$board"
	"$bmk" makeapp \
		-a -r \
		-l pico -g arm -board "$board" -heap 16k \
		-o "$output" \
		"$module_root/examples/allocation_failure_memory.bmx"

	test -s "$output.elf"
	test -s "$output.uf2"

	symbols="$("$toolchain/bin/arm-none-eabi-nm" "$output.elf")"
	rg -q ' t bmx_embedded_raise_allocation_failure$' <<<"$symbols"
	rg -q ' T bmx_embedded_exception_throw$' <<<"$symbols"

	read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$output.elf" | awk 'NR == 2')
	test "$text_size" -le 50000
	test "$bss_size" -ge 21000
	test "$bss_size" -le 26000

	echo "Pico allocation failure image: board=$board text=$text_size bss=$bss_size"
done
