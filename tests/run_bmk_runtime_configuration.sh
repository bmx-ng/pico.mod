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
grep -q 'Pico memory (pico, rp2040):' <<<"$pico_output"
grep -q ' / 2097152 bytes ' <<<"$pico_output"

pico2_output=$("$bmk" makeapp -a -r -l pico -g arm -board pico2 \
	-o "$work_dir/runtime-pico2" "$source_file" 2>&1)
grep -q 'Managed heap: 393216 bytes (auto)' <<<"$pico2_output"
grep -q 'Pico memory (pico2, rp2350-arm-s):' <<<"$pico2_output"
grep -q ' / 4194304 bytes ' <<<"$pico2_output"

rp2040_board_output=$("$bmk" makeapp -a -r -l pico -g arm -board adafruit_qtpy_rp2040 \
	-o "$work_dir/runtime-rp2040-board" "$source_file" 2>&1)
grep -q 'Managed heap: 196608 bytes (auto)' <<<"$rp2040_board_output"
grep -q 'Pico memory (adafruit_qtpy_rp2040, rp2040):' <<<"$rp2040_board_output"
grep -q ' / 8388608 bytes ' <<<"$rp2040_board_output"

rp2350_board_output=$("$bmk" makeapp -a -r -l pico -g arm -board adafruit_feather_rp2350 \
	-o "$work_dir/runtime-rp2350-board" "$source_file" 2>&1)
grep -q 'Managed heap: 393216 bytes (auto)' <<<"$rp2350_board_output"
grep -q 'Pico memory (adafruit_feather_rp2350, rp2350-arm-s):' <<<"$rp2350_board_output"
grep -q ' / 8388608 bytes ' <<<"$rp2350_board_output"

mixed_case_board_output=$("$bmk" makeapp -a -r -l pico -g arm -board hellbender_2350A_devboard \
	-o "$work_dir/runtime-mixed-case-board" "$source_file" 2>&1)
grep -q 'Pico memory (hellbender_2350A_devboard, rp2350-arm-s):' <<<"$mixed_case_board_output"

custom_board_output=$(PICO_BOARD_HEADER_DIRS="$module_root/tests/fixtures/boards" \
	"$bmk" makeapp -a -r -l pico -g arm -board blitzmax_test_rp2040 \
	-o "$work_dir/runtime-custom-board" "$source_file" 2>&1)
grep -q 'Pico memory (blitzmax_test_rp2040, rp2040):' <<<"$custom_board_output"
grep -q ' / 4194304 bytes ' <<<"$custom_board_output"

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

if "$bmk" makeapp -r -l pico -g arm -board pico -heap 300k \
	-o "$work_dir/runtime-too-large" "$source_file" >"$work_dir/too-large.log" 2>&1; then
	echo "oversized RP2040 heap was accepted" >&2
	exit 1
fi
grep -q 'managed heap must leave room for application and SDK RAM' "$work_dir/too-large.log"

if "$bmk" makeapp -r -l pico -g arm -board ../pico \
	-o "$work_dir/runtime-unsafe-board" "$source_file" >"$work_dir/unsafe-board.log" 2>&1; then
	echo "unsafe Pico SDK board name was accepted" >&2
	exit 1
fi
grep -q "Invalid Pico SDK board name '../pico'" "$work_dir/unsafe-board.log"

echo "Pico runtime configuration passed (SDK boards, platform-aware heaps, board flash, explicit heap validation)"
