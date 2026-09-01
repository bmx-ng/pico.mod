#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
source_file="$module_root/tests/fixtures/runtime_configuration.bmx"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

pico_output=$("$bmk" makeapp -a -r -l pico -g arm -board pico \
	-o "$work_dir/runtime-pico" "$source_file" 2>&1)
grep -q 'Managed heap: 196608 bytes (auto)' <<<"$pico_output"

pico2_output=$("$bmk" makeapp -a -r -l pico -g arm -board pico2 \
	-o "$work_dir/runtime-pico2" "$source_file" 2>&1)
grep -q 'Managed heap: 393216 bytes (auto)' <<<"$pico2_output"

explicit_output=$("$bmk" makeapp -a -r -l pico -g arm -board pico2 -heap 64k \
	-o "$work_dir/runtime-explicit" "$source_file" 2>&1)
grep -q 'Managed heap: 65536 bytes (64k)' <<<"$explicit_output"
grep -q 'Flash:' <<<"$explicit_output"
grep -q 'App/SDK RAM:' <<<"$explicit_output"
grep -q 'RAM headroom:' <<<"$explicit_output"

if "$bmk" makeapp -r -l pico -g arm -board pico -heap invalid \
	-o "$work_dir/runtime-invalid" "$source_file" >"$work_dir/invalid.log" 2>&1; then
	echo "invalid Pico heap size was accepted" >&2
	exit 1
fi
grep -q "Invalid Pico heap size 'invalid'" "$work_dir/invalid.log"

echo "Pico runtime configuration passed (Pico auto=192 KiB, Pico 2 auto=384 KiB, explicit=64 KiB)"
