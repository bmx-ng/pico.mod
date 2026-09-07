' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Framework BRL.EventQueue
Import BRL.StandardIO
Import Pico.Board
Import Pico.Hardware.PIO
Import "pio_irq_led.pio"

Local checksPassed:Int = True
Local program:TPIOProgram = PIOProgram("irq_led")
checksPassed :& program <> Null And program.IsValid()

Const controller:Int = PIOController0
Const irqLine:UInt = PIOIRQLine0
Const interruptNumber:UInt = 0
Local irqMask:UInt = PIOIRQInterruptMask(interruptNumber)

Local ledPin:UInt = DefaultLEDPin()
Local settings:SPIOStateMachineConfig
settings.SetOutPins(ledPin, 1)
settings.SetClockDivider(1000.0)
Local machine:TPIOStateMachine
If program Then machine = TPIOStateMachine.Create(controller, program, settings)
checksPassed :& machine <> Null And irqMask <> 0

If checksPassed Then
	Local stateMachine:Int = machine.Index()
	checksPassed :& PIOGPIOInit(controller, ledPin)
	checksPassed :& PIOStateMachineSetConsecutivePinDirections(controller, stateMachine, ledPin, 1, True) >= 0
	checksPassed :& settings.OverrideMask() = (PIOConfigOutPins | PIOConfigClockDivider)
	checksPassed :& PIOStateMachineClearFIFOs(controller, stateMachine)
	checksPassed :& PIOInterruptClear(controller, interruptNumber)
	checksPassed :& machine.SetEnabled(True)
End If

Local irqSource:TPIOIRQSource
If checksPassed Then irqSource = TPIOIRQSource.Create(controller, irqLine, irqMask)
If Not irqSource Then Throw "Unable to create PIO IRQ event source"

Local level:UInt = 1
Const delayCount:UInt = 60000

While True
	Local command:UInt = (delayCount Shl 1) | level
	machine.PutBlocking(command)

	Local eventId:Int = WaitEvent()
	If eventId <> EVENT_PIOIRQ Or EventSource() <> irqSource Then Continue

	Local eventMask:UInt = UInt(EventData())
	Local latchedMask:UInt = irqSource.TakeEvents()
	Local capturedAt:ULong = PIOIRQTimeUS(CurrentEvent)
	Local eventPassed:Int = (eventMask & irqMask) <> 0 And eventMask = latchedMask
	eventPassed :& EventMods() = controller And capturedAt <> 0
	eventPassed :& PIOInterruptIsSet(controller, interruptNumber)
	eventPassed :& PIOInterruptClear(controller, interruptNumber)
	eventPassed :& irqSource.RearmSources(irqMask)

	If eventPassed Then
		Print "PIO event handled: LED=" + level + " captured_us=" + capturedAt
	Else
		Print "PIO event check failed"
	End If
	level :~ 1
Wend
