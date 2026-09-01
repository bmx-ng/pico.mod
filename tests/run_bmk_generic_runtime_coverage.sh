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
		-o "$work_dir/generic_runtime_coverage-$board" \
		"$module_root/examples/generic_runtime_coverage.bmx"

	test -s "$work_dir/generic_runtime_coverage-$board.elf"
	test -s "$work_dir/generic_runtime_coverage-$board.uf2"
done

build_dir="$module_root/examples/.bmx/generic_runtime_coverage.release.pico.arm.pico2"
manifest="$build_dir/bcc/application/bcc-build.manifest"
test -s "$manifest"
test "$(grep -c '^file generic-specialization-c ' "$manifest")" -ge 1

generic_unit="$(find "$build_dir/bcc/application/.generics/units" -name '*.c' -type f | head -1)"
test -n "$generic_unit"
grep -q 'bmx_pico_array_concat' "$generic_unit"
grep -q 'bmx_pico_exception_enter' "$generic_unit"
grep -q 'BMXPicoRootFrame' "$generic_unit"

echo "Pico generic runtime coverage build passed for Pico and Pico 2"
