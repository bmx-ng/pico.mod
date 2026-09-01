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
		-l pico -g arm -board "$board" -heap 32k \
		-o "$work_dir/collections_hashmap_generic_key-$board" \
		"$module_root/examples/collections_hashmap_generic_key.bmx"

	test -s "$work_dir/collections_hashmap_generic_key-$board.elf"
	test -s "$work_dir/collections_hashmap_generic_key-$board.uf2"
done

build_dir="$module_root/examples/.bmx/collections_hashmap_generic_key.release.pico.arm.pico2"
manifest="$build_dir/bcc/application/bcc-build.manifest"
test -s "$manifest"
test "$(grep -c '^file generic-specialization-c ' "$manifest")" -ge 8

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi

read -r text_size _ bss_size _ < <("$toolchain/bin/arm-none-eabi-size" "$work_dir/collections_hashmap_generic_key-pico2.elf" | awk 'NR == 2')
test "$text_size" -le 75000
test "$bss_size" -le 42000

echo "Pico generic HashMap image: text=$text_size bss=$bss_size"
