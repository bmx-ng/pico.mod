' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: General-purpose digital input and output for Raspberry Pi Pico targets.
End Rem
Module Pico.Hardware.GPIO
?pico

ModuleInfo "Version: 0.2"
ModuleInfo "License: zlib/libpng"

Import BRL.Event
Import Pico.Runtime.Events

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

Rem
bbdoc: Emitted after a registered GPIO interrupt.
about: EventData contains the edge/level flags and EventMods contains the GPIO
number. Use #GPIOIRQTimeUS to recover the 64-bit timestamp captured inside the
native interrupt callback. Event delivery itself is deferred to PollSystem or
WaitSystem and can occur later than that timestamp.
End Rem
Global EVENT_GPIOIRQ:Int = AllocUserEventId("GPIOIRQ")

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
	Function _GPIOSetEventToken:Int(pin:UInt, token:UInt) = "bmx_pico_gpio_set_event_token"
	Function GPIOPendingIRQEvents:UInt(pin:UInt) = "bmx_pico_gpio_pending_irq_events"
	Function GPIOTakeIRQEvents:UInt(pin:UInt) = "bmx_pico_gpio_take_irq_events"
End Extern

Rem
bbdoc: A managed, timestamped GPIO interrupt event source.
about: Create enables the selected native GPIO interrupts on core 0. The native
callback captures time_us_64 immediately, then queues a numeric record without
calling managed code. Close disables the interrupt and unregisters the source;
it is required because an open source is deliberately retained by the event
bridge.
End Rem
Type TGPIOIRQSource Implements ICloseable
	Private
	Field pin:Int = -1
	Field eventMask:UInt
	Field eventToken:UInt
	Field tokenInstalled:Int
	Field irqEnabled:Int

	Public
	Function Create:TGPIOIRQSource(pin:UInt, eventMask:UInt)
		If eventMask = 0 Then Return Null
		Local source:TGPIOIRQSource = New TGPIOIRQSource
		source.pin = pin
		source.eventMask = eventMask
		source.eventToken = RegisterPicoEventSource(source, EVENT_GPIOIRQ, True)
		If source.eventToken = 0 Then
			source.Close()
			Return Null
		End If
		If Not _GPIOSetEventToken(pin, source.eventToken) Then
			source.Close()
			Return Null
		End If
		source.tokenInstalled = True
		If Not GPIOSetIRQEnabled(pin, eventMask, True) Then
			source.Close()
			Return Null
		End If
		source.irqEnabled = True
		Return source
	End Function

	Method Pin:Int()
		Return pin
	End Method

	Method EventMask:UInt()
		Return eventMask
	End Method

	Method IsOpen:Int()
		Return pin >= 0
	End Method

	Method Close()
		If pin < 0 Then Return
		If irqEnabled Then GPIOSetIRQEnabled(pin, eventMask, False)
		If tokenInstalled Then _GPIOSetEventToken(pin, 0)
		If eventToken Then ReleasePicoEventSource(eventToken)
		irqEnabled = False
		tokenInstalled = False
		eventToken = 0
		pin = -1
	End Method

	Method Delete()
		Close()
	End Method
End Type

Rem
bbdoc: Returns the native microsecond timestamp carried by a GPIO IRQ event.
about: This is the instant at which the interrupt callback ran, not the later
time at which the BlitzMax event queue delivered the event.
End Rem
Function GPIOIRQTimeUS:ULong(event:TEvent)
	If Not event Or event.id <> EVENT_GPIOIRQ Then Return 0
	Return ULong(UInt(event.x)) | (ULong(UInt(event.y)) Shl 32)
End Function
?
