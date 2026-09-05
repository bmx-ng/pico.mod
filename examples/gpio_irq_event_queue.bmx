' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Framework BRL.EventQueue
Import BRL.StandardIO
Import Pico.Hardware.GPIO

Const inputPin:UInt = 2

GPIOInit(inputPin)
GPIOSetInput(inputPin)
GPIOPullUp(inputPin)

Local irqSource:TGPIOIRQSource = TGPIOIRQSource.Create(inputPin, ..
	GPIOIRQEdgeRise | GPIOIRQEdgeFall)
If Not irqSource Then Throw "Unable to create GPIO IRQ source"

While True
	If WaitEvent() = EVENT_GPIOIRQ And EventSource() = irqSource
		Local capturedAt:ULong = GPIOIRQTimeUS(CurrentEvent)
		Print "GPIO " + EventMods() + " flags=" + EventData() + ..
			" captured_us=" + capturedAt
	End If
Wend
