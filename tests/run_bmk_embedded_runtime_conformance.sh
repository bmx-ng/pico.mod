#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
fixture_root="$module_root/../embedded.mod/tests"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

for board in pico pico2; do
	for fixture in embedded_runtime_conformance embedded_language_conformance embedded_gpio_conformance embedded_gpio_events_conformance embedded_time_conformance embedded_uart_conformance embedded_buffered_uart_conformance embedded_i2c_conformance embedded_spi_conformance embedded_adc_conformance embedded_pwm_conformance embedded_watchdog_conformance embedded_device_conformance embedded_random_conformance embedded_unicode_conformance embedded_events_conformance embedded_uncaught_string embedded_runtime_error; do
		output="$work_dir/$fixture-$board"
		"$bmk" makeapp \
			-a -r \
			-l pico -g arm -board "$board" -heap 16k \
			-o "$output" \
			"$fixture_root/$fixture.bmx"

		test -s "$output.elf"
		test -s "$output.uf2"
	done

	for build_mode in debug release; do
		fixture="embedded_assert_$build_mode"
		output="$work_dir/$fixture-$board"
		if [ "$build_mode" = debug ]; then
			mode=-d
		else
			mode=-r
		fi
		"$bmk" makeapp \
			-a "$mode" \
			-l pico -g arm -board "$board" -heap 16k \
			-o "$output" \
			"$fixture_root/$fixture.bmx"

		test -s "$output.elf"
		test -s "$output.uf2"
	done
done

echo "Shared embedded runtime and fatal-path conformance images passed for Pico and Pico 2"
