' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: General-purpose digital input and output for Raspberry Pi Pico targets.
End Rem
Module Pico.Hardware.GPIO
?pico

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"

Const GPIOInput:Int = 0
Const GPIOOutput:Int = 1

Const GPIOFunctionSPI:Int = 1
Const GPIOFunctionUART:Int = 2
Const GPIOFunctionUARTAux:Int = 11
Const GPIOFunctionI2C:Int = 3
Const GPIOFunctionPWM:Int = 4
Const GPIOFunctionSIO:Int = 5
Const GPIOFunctionPIO0:Int = 6
Const GPIOFunctionPIO1:Int = 7
Const GPIOFunctionPIO2:Int = 8
Const GPIOFunctionNull:Int = $1f

Const GPIOIRQLevelLow:UInt = $1
Const GPIOIRQLevelHigh:UInt = $2
Const GPIOIRQEdgeFall:UInt = $4
Const GPIOIRQEdgeRise:UInt = $8

Const GPIOSlewRateSlow:Int = 0
Const GPIOSlewRateFast:Int = 1

Const GPIODriveStrength2mA:Int = 0
Const GPIODriveStrength4mA:Int = 1
Const GPIODriveStrength8mA:Int = 2
Const GPIODriveStrength12mA:Int = 3

Extern "C"
	Function GPIOInit(pin:UInt) = "bmx_pico_gpio_init"
	Function GPIOSetFunction(pin:UInt, gpioFunction:Int) = "bmx_pico_gpio_set_function"
	Function GPIOGetFunction:Int(pin:UInt) = "bmx_pico_gpio_get_function"
	Function GPIOSetDirection(pin:UInt, direction:Int) = "bmx_pico_gpio_set_direction"
	Function GPIOGetDirection:Int(pin:UInt) = "bmx_pico_gpio_get_direction"
	Function GPIOSetInput(pin:UInt) = "bmx_pico_gpio_set_input"
	Function GPIOSetOutput(pin:UInt) = "bmx_pico_gpio_set_output"
	Function GPIOGet:Int(pin:UInt) = "bmx_pico_gpio_get"
	Function GPIOPut(pin:UInt, value:Int) = "bmx_pico_gpio_put"
	Function GPIOGetOutput:Int(pin:UInt) = "bmx_pico_gpio_get_output"
	Function GPIOSetPulls(pin:UInt, pullUp:Int, pullDown:Int) = "bmx_pico_gpio_set_pulls"
	Function GPIOPullUp(pin:UInt) = "bmx_pico_gpio_pull_up"
	Function GPIOPullDown(pin:UInt) = "bmx_pico_gpio_pull_down"
	Function GPIODisablePulls(pin:UInt) = "bmx_pico_gpio_disable_pulls"
	Function GPIOIsPulledUp:Int(pin:UInt) = "bmx_pico_gpio_is_pulled_up"
	Function GPIOIsPulledDown:Int(pin:UInt) = "bmx_pico_gpio_is_pulled_down"
	Function GPIOSetInputEnabled(pin:UInt, enabled:Int) = "bmx_pico_gpio_set_input_enabled"
	Function GPIOSetInputHysteresisEnabled(pin:UInt, enabled:Int) = "bmx_pico_gpio_set_input_hysteresis_enabled"
	Function GPIOSetSlewRate(pin:UInt, slewRate:Int) = "bmx_pico_gpio_set_slew_rate"
	Function GPIOGetSlewRate:Int(pin:UInt) = "bmx_pico_gpio_get_slew_rate"
	Function GPIOSetDriveStrength(pin:UInt, driveStrength:Int) = "bmx_pico_gpio_set_drive_strength"
	Function GPIOGetDriveStrength:Int(pin:UInt) = "bmx_pico_gpio_get_drive_strength"

	Rem
	bbdoc: Enables or disables native interrupt event latching on core 0.
	about: The interrupt handler does not call BlitzMax code. Read and clear the
	coalesced event bits later with #GPIOTakeIRQEvents from ordinary thread mode.
	End Rem
	Function GPIOSetIRQEnabled:Int(pin:UInt, eventMask:UInt, enabled:Int) = "bmx_pico_gpio_set_irq_enabled"
	Function GPIOPendingIRQEvents:UInt(pin:UInt) = "bmx_pico_gpio_pending_irq_events"
	Function GPIOTakeIRQEvents:UInt(pin:UInt) = "bmx_pico_gpio_take_irq_events"
End Extern
?
