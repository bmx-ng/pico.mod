#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

for board in pico pico2; do
	"$bmk" makeapp -a -r -l pico -g arm -board "$board" -heap 16k \
		-o "$work_dir/device-info-$board" \
		"$module_root/examples/device_info.bmx"

	test -s "$work_dir/device-info-$board.elf"
	test -s "$work_dir/device-info-$board.uf2"
done

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

image_symbols="$("$toolchain/bin/arm-none-eabi-nm" "$work_dir/device-info-pico2.elf")"
rg -q ' T bmx_pico_unique_board_id$' <<<"$image_symbols"
rg -q ' T bmx_pico_unique_board_id_bytes$' <<<"$image_symbols"
rg -q ' T bmx_pico_bootsel_button_pressed$' <<<"$image_symbols"

runtime_object="$(find "$module_root/examples/.bmx/device_info.release.pico.arm.pico2" \
	-path '*/pico.mod/runtime.mod/native/pico_runtime.c.o' -print -quit)"
test -n "$runtime_object"
runtime_symbols="$("$toolchain/bin/arm-none-eabi-nm" "$runtime_object")"
rg -q ' T bmx_pico_device_reboot$' <<<"$runtime_symbols"
rg -q ' T bmx_pico_device_reboot_to_bootsel$' <<<"$runtime_symbols"

echo "Pico device identity/reset images passed"
