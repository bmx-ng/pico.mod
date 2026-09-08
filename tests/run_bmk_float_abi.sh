#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
board="${PICO_TEST_RP2350_BOARD:-pimoroni_pico_plus2_rp2350}"
benchmark="$module_root/benchmarks/float_abi/float_abi_benchmark.bmx"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

soft_output=$(
	"$bmk" makeapp -a -r -l pico -g arm -board "$board" \
		-o "$work_dir/float-softfp" "$benchmark"
)
hard_output=$(
	"$bmk" makeapp -a -r -l pico -g arm -board "$board" -float-abi hard \
		-o "$work_dir/float-hard" "$benchmark"
)

grep -q 'Pico floating-point ABI: softfp' <<<"$soft_output"
grep -q 'Pico floating-point ABI: hard' <<<"$hard_output"
test -s "$work_dir/float-softfp.elf"
test -s "$work_dir/float-hard.elf"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

soft_attributes=$("$toolchain/bin/arm-none-eabi-readelf" -A "$work_dir/float-softfp.elf")
hard_attributes=$("$toolchain/bin/arm-none-eabi-readelf" -A "$work_dir/float-hard.elf")
if grep -q 'Tag_ABI_VFP_args: VFP registers' <<<"$soft_attributes"; then
	echo "Default RP2350 firmware unexpectedly uses the hard-float ABI" >&2
	exit 1
fi
grep -q 'Tag_ABI_VFP_args: VFP registers' <<<"$hard_attributes"

if "$bmk" makeapp -a -r -l pico -g arm -board pico -float-abi hard \
	-o "$work_dir/rp2040-hard" "$benchmark" >"$work_dir/rp2040-hard.log" 2>&1; then
	echo "RP2040 unexpectedly accepted the hard-float ABI" >&2
	exit 1
fi
grep -q 'hard-float ABI is supported only by ARM RP2350 boards' "$work_dir/rp2040-hard.log"

echo "Pico floating-point ABI build checks passed (RP2350 softfp/hard, RP2040 rejection)"
