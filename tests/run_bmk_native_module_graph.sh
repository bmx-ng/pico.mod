#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
board="${PICO_TEST_BOARD:-pico2}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

"$bmk" makeapp \
	-a -r \
	-l pico -g arm -board "$board" -heap 16k \
	-o "$work_dir/native_module_graph" \
	"$module_root/examples/native_module_graph.bmx"

test -s "$work_dir/native_module_graph.elf"
test -s "$work_dir/native_module_graph.uf2"

generated_dir="$module_root/examples/.bmx/native_module_graph.release.pico.arm.$board/generated"
native_cmake="$generated_dir/blitzmax_pico_native.cmake"
test -s "$native_cmake"
grep -q 'native_value.c' "$native_cmake"
grep -q 'native_value.cpp' "$native_cmake"
grep -q 'native_application_value.c' "$native_cmake"
grep -q -- '-DBMX_PICO_NATIVE_COMMON=11' "$native_cmake"
grep -q -- '-DBMX_PICO_NATIVE_C_ONLY=13' "$native_cmake"
grep -q -- '-DBMX_PICO_NATIVE_CPP_ONLY=17' "$native_cmake"
grep -q -- '-DBMX_PICO_NATIVE_LEXICAL=19' "$native_cmake"
grep -q -- '--defsym,bmx_pico_native_link_marker=29' "$native_cmake"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

"$toolchain/bin/arm-none-eabi-nm" "$work_dir/native_module_graph.elf" | grep -q ' bmx_pico_native_c_value$'
"$toolchain/bin/arm-none-eabi-nm" "$work_dir/native_module_graph.elf" | grep -q ' bmx_pico_native_cpp_value$'
"$toolchain/bin/arm-none-eabi-nm" "$work_dir/native_module_graph.elf" | grep -q ' bmx_pico_native_application_value$'
"$toolchain/bin/arm-none-eabi-nm" "$work_dir/native_module_graph.elf" | grep -q ' A bmx_pico_native_link_marker$'
symbol_address="$("$toolchain/bin/arm-none-eabi-nm" -n "$work_dir/native_module_graph.elf" | awk '/_ib_bb_pico_tests_nativegraph_.*_1_data$/ { print $1; exit }')"
test -n "$symbol_address"
case "$symbol_address" in
	10*) ;;
	*) echo "Imported module Incbin payload is not linked into Pico XIP flash: $symbol_address" >&2; exit 1 ;;
esac

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/native_module_graph.elf" | awk 'NR == 2')
test "$text_size" -le 55000
test "$bss_size" -le 22000

echo "Pico native module graph passed: board=$board text=$text_size bss=$bss_size payload=0x$symbol_address"
