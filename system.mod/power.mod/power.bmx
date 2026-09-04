' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Low-power sleep and wake services for Raspberry Pi Pico targets.
about: Full clock-gated sleep and dormant operation requires Pico SDK 2.3.0 or
newer. #PowerCapabilities reports the operations available in the selected SDK
and processor. Ordinary #Delay and Pico.System.Time sleeps do not enter these
explicit low-power states.
End Rem
Module Pico.System.Power
?pico

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"

Const PowerCapabilityIdle:UInt = $01
Const PowerCapabilitySleepInterrupt:UInt = $02
Const PowerCapabilitySleepTimer:UInt = $04
Const PowerCapabilityDormantGPIO:UInt = $08
Const PowerCapabilityDormantTimer:UInt = $10
Const PowerCapabilityLowLeakagePins:UInt = $20

Rem
bbdoc: Results returned by low-power operations.
End Rem
Enum EPowerResult:Int
	Success = 0
	Error = -1
	Timeout = -2
	NotPermitted = -4
	InvalidArgument = -5
	InsufficientResources = -9
	InvalidState = -12
	PreconditionNotMet = -14
	InvalidData = -16
	Unavailable = -17
	ResourceInUse = -21
End Enum

Extern "C"
	Rem
	bbdoc: Returns a mask of PowerCapability constants supported by this build.
	End Rem
	Function PowerCapabilities:UInt() = "bmx_pico_power_capabilities"

	Rem
	bbdoc: Waits efficiently until an interrupt occurs without changing clocks.
	about: This operation is available with every supported Pico SDK and may wake
	spuriously. Code must always recheck the condition it was waiting for.
	End Rem
	Function PowerIdle() = "bmx_pico_power_idle"

	Rem
	bbdoc: Enters clock-gated sleep until any enabled interrupt occurs.
	End Rem
	Function LowPowerSleepUntilInterrupt:EPowerResult() = "bmx_pico_power_sleep_until_interrupt"

	Rem
	bbdoc: Enters clock-gated sleep for approximately the requested duration.
	about: When @exclusive is true, unrelated interrupts are temporarily masked.
	USB and configured standard-I/O clocks may remain active so their connections
	are preserved.
	End Rem
	Function LowPowerSleep:EPowerResult(milliseconds:UInt, exclusive:Int = True) = "bmx_pico_power_sleep_for_ms"

	Rem
	bbdoc: Enters dormant mode and wakes after the requested duration.
	about: Timed dormant is directly supported on RP2350. RP2040 requires an
	external always-on clock and currently returns PreconditionNotMet. USB is
	temporarily disconnected while dormant and is restored after wake.
	End Rem
	Function DormantSleep:EPowerResult(milliseconds:UInt) = "bmx_pico_power_dormant_for_ms"

	Rem
	bbdoc: Enters dormant mode until a GPIO edge or level is detected.
	param edge: True selects an edge; False selects a level.
	param high: True selects rising/high; False selects falling/low.
	about: Configure the GPIO direction and pulls before calling. USB is
	temporarily disconnected while dormant and is restored after wake.
	End Rem
	Function DormantSleepUntilGPIO:EPowerResult(pin:UInt, edge:Int, high:Int) = "bmx_pico_power_dormant_until_gpio"

	Rem
	bbdoc: Places every GPIO except those selected by @excludeMask into a low-leakage input state.
	about: This changes pin configuration and is not automatically undone. Reinitialise
	affected peripherals after waking. A set bit preserves that GPIO's configuration.
	End Rem
	Function SetUnusedPinsLowLeakage:EPowerResult(excludeMask:ULong = 0) = "bmx_pico_power_set_unused_pins_low_leakage"
End Extern

Rem
bbdoc: Returns True when every requested capability is available.
End Rem
Function PowerSupports:Int(capabilities:UInt)
	Return (PowerCapabilities() & capabilities) = capabilities
End Function
?
