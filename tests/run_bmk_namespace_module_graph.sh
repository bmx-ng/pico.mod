#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
board="${PICO_TEST_BOARD:-pico2}"
fixture_root="$module_root/tests/fixtures/modules/picographfixture.mod"
fixture_link="$sdk/mod/picographfixture.mod"
work_dir="$(mktemp -d)"

if [[ -e "$fixture_link" || -L "$fixture_link" ]]; then
	echo "Pico namespace fixture path already exists: $fixture_link" >&2
	exit 1
fi

ln -s "$fixture_root" "$fixture_link"
trap 'rm -f "$fixture_link"; rm -rf "$work_dir"' EXIT

"$bmk" makeapp \
	-a -r \
	-l pico -g arm -board "$board" -heap 16k \
	-o "$work_dir/namespace_module_graph" \
	"$module_root/examples/namespace_module_graph.bmx"

test -s "$work_dir/namespace_module_graph.elf"
test -s "$work_dir/namespace_module_graph.uf2"

generated_dir="$module_root/examples/.bmx/namespace_module_graph.release.pico.arm.$board/generated"
native_cmake="$generated_dir/blitzmax_pico_native.cmake"
test -s "$native_cmake"
grep -q 'picographfixture.mod/utility.mod/native.c' "$native_cmake"
grep -q -- '-DBMX_PICO_GRAPH_MODULE_VALUE=5' "$native_cmake"
grep -q -- '-I.*picographfixture.mod/utility.mod/include' "$native_cmake"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi
"$toolchain/bin/arm-none-eabi-nm" "$work_dir/namespace_module_graph.elf" | grep -q ' bmx_pico_graph_native_value$'

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/namespace_module_graph.elf" | awk 'NR == 2')
test "$text_size" -le 55000
test "$bss_size" -le 22000

echo "Pico general namespace module graph passed: board=$board text=$text_size bss=$bss_size"
