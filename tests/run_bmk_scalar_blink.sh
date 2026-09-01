#!/usr/bin/env bash
set -euo pipefail

module_root="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(cd "$module_root/../.." && pwd)"
bmk="${PICO_TEST_BMK:-$sdk/bin/bmk}"
work_dir="$(mktemp -d)"
trap 'rm -rf "$work_dir"' EXIT

"$bmk" makeapp \
	-a -r \
	-l pico -g arm -board pico2 -heap 16k \
	-o "$work_dir/scalar_blink" \
	"$module_root/examples/scalar_blink.bmx"

test -s "$work_dir/scalar_blink.elf"
test -s "$work_dir/scalar_blink.uf2"

toolchain="${PICO_TOOLCHAIN_PATH:-}"
if [[ -z "$toolchain" ]]; then
	toolchain="$(find "$HOME/.pico-sdk/toolchain" -mindepth 1 -maxdepth 1 -type d | sort | tail -1)"
fi
size_tool="$toolchain/bin/arm-none-eabi-size"
size_text="$($size_tool "$work_dir/scalar_blink.elf" | awk 'NR == 2 { print $1 }')"
test "$size_text" -le 32768

if "$toolchain/bin/arm-none-eabi-nm" "$work_dir/scalar_blink.elf" | rg -q 'bmx_pico_gpio_(irq_callback|set_irq_enabled|pending_irq_events|take_irq_events)|bmx_pico_gpio_irq_events'; then
	echo "GPIO interrupt support leaked into the scalar blink image" >&2
	exit 1
fi

if "$toolchain/bin/arm-none-eabi-nm" "$work_dir/scalar_blink.elf" | rg -q 'bmx_pico_alarm_|bmx_pico_repeating_alarm'; then
	echo "Native alarm support leaked into the scalar blink image" >&2
	exit 1
fi

if "$toolchain/bin/arm-none-eabi-nm" "$work_dir/scalar_blink.elf" | rg -q 'bmx_pico_pwm_|bmx_pico_pwm_wrap_events'; then
	echo "PWM support leaked into the scalar blink image" >&2
	exit 1
fi

if "$toolchain/bin/arm-none-eabi-nm" "$work_dir/scalar_blink.elf" | rg -q 'bmx_pico_adc_'; then
	echo "ADC support leaked into the scalar blink image" >&2
	exit 1
fi

if "$toolchain/bin/arm-none-eabi-nm" "$work_dir/scalar_blink.elf" | rg -q 'bmx_pico_i2c_'; then
	echo "I2C support leaked into the scalar blink image" >&2
	exit 1
fi

if "$toolchain/bin/arm-none-eabi-nm" "$work_dir/scalar_blink.elf" | rg -q 'bmx_pico_spi_'; then
	echo "SPI support leaked into the scalar blink image" >&2
	exit 1
fi

if "$toolchain/bin/arm-none-eabi-nm" "$work_dir/scalar_blink.elf" | rg -q 'bmx_pico_uart_'; then
	echo "UART support leaked into the scalar blink image" >&2
	exit 1
fi
