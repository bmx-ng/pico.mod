SuperStrict

Rem
bbdoc: Pulse-width modulation for Raspberry Pi Pico targets.
End Rem
Module Pico.Hardware.PWM

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"

' RP2350 exposes twelve PWM slices. GPIO-to-slice/channel mapping should be
' queried through PWMSliceForGPIO and PWMChannelForGPIO rather than assumed.

Const PWMChannelA:UInt = 0
Const PWMChannelB:UInt = 1

Const PWMDividerFreeRunning:UInt = 0
Const PWMDividerBHigh:UInt = 1
Const PWMDividerBRising:UInt = 2
Const PWMDividerBFalling:UInt = 3

Extern "C"
	Function PWMInitGPIO:UInt(pin:UInt) = "bmx_pico_pwm_init_gpio"
	Function PWMSliceForGPIO:UInt(pin:UInt) = "bmx_pico_pwm_slice_for_gpio"
	Function PWMChannelForGPIO:UInt(pin:UInt) = "bmx_pico_pwm_channel_for_gpio"
	Function PWMSetWrap(slice:UInt, wrap:UInt) = "bmx_pico_pwm_set_wrap"
	Function PWMGetWrap:UInt(slice:UInt) = "bmx_pico_pwm_get_wrap"
	Function PWMSetChannelLevel(slice:UInt, channel:UInt, level:UInt) = "bmx_pico_pwm_set_channel_level"
	Function PWMGetChannelLevel:UInt(slice:UInt, channel:UInt) = "bmx_pico_pwm_get_channel_level"
	Function PWMSetBothLevels(slice:UInt, levelA:UInt, levelB:UInt) = "bmx_pico_pwm_set_both_levels"
	Function PWMSetGPIOLevel(pin:UInt, level:UInt) = "bmx_pico_pwm_set_gpio_level"
	Function PWMGetCounter:UInt(slice:UInt) = "bmx_pico_pwm_get_counter"
	Function PWMSetCounter(slice:UInt, counter:UInt) = "bmx_pico_pwm_set_counter"
	Function PWMSetClockDivider(slice:UInt, divider:Float) = "bmx_pico_pwm_set_clock_divider"
	Function PWMSetClockDividerIntFrac(slice:UInt, integer:UInt, fraction:UInt) = "bmx_pico_pwm_set_clock_divider_int_frac"
	Function PWMSetDividerMode(slice:UInt, mode:UInt) = "bmx_pico_pwm_set_divider_mode"
	Function PWMSetOutputPolarity(slice:UInt, invertA:Int, invertB:Int) = "bmx_pico_pwm_set_output_polarity"
	Function PWMSetPhaseCorrect(slice:UInt, enabled:Int) = "bmx_pico_pwm_set_phase_correct"
	Function PWMSetEnabled(slice:UInt, enabled:Int) = "bmx_pico_pwm_set_enabled"

	Rem
	bbdoc: Configures a trailing-edge PWM frequency and returns the achieved frequency.
	about: The helper selects a 4-bit fractional clock divider and the largest
	practical 16-bit wrap value. It returns zero for a zero requested frequency.
	End Rem
	Function PWMSetFrequency:UInt(slice:UInt, frequency:UInt) = "bmx_pico_pwm_set_frequency"

	Rem
	bbdoc: Enables or disables native wrap-event counting for a slice on core 0.
	about: The IRQ handler does not call BlitzMax code.
	End Rem
	Function PWMSetIRQEnabled:Int(slice:UInt, enabled:Int) = "bmx_pico_pwm_set_irq_enabled"
	Function PWMPendingWrapEvents:UInt(slice:UInt) = "bmx_pico_pwm_pending_wrap_events"
	Function PWMTakeWrapEvents:UInt(slice:UInt) = "bmx_pico_pwm_take_wrap_events"
End Extern
