#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

examples=(
	adc_temperature array_memory block_memory closure_memory container_memory
	collections_hashmap_generic_key collections_stack exception_memory finalizer_memory generic_finalizer generic_inheritance generic_object_hooks generic_runtime_coverage generic_scalar_memory gpio_input_irq
	i2c_controller imported_type_memory inheritance_memory object_memory pwm_led
	multisource_application native_module_graph spi_controller stream_memory string_case_memory string_conversion_memory
	string_encoding_memory string_float_conversion string_float_join_fixed
	string_float_split_join_memory string_integer_conversion
	string_integer_split_join_memory string_memory string_split_join_memory
	string_unicode_case_memory struct_enum_memory timer_alarm uart_controller
	using_memory
)

for board in pico pico2; do
	for example in "${examples[@]}"; do
		"$bmk" makeapp -a -r -l pico -g arm -board "$board" \
			-o "$work_dir/${example}-${board}" \
			"$module_root/examples/${example}.bmx"
		test -s "$work_dir/${example}-${board}.elf"
		test -s "$work_dir/${example}-${board}.uf2"
	done
done

"$bmk" makeapp -a -r -l pico -g arm -board pico \
	-o "$work_dir/pico-blink" "$module_root/examples/pico_blink.bmx"
test -s "$work_dir/pico-blink.elf"
test -s "$work_dir/pico-blink.uf2"

echo "Pico and Pico 2 build matrix passed (${#examples[@]} shared examples per board plus Pico board-module coverage)"
