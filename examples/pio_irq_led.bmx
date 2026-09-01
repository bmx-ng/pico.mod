SuperStrict

Import BRL.StandardIO
Import Pico.Board.Pico
Import Pico.Hardware.PIO
Import "pio_irq_led.pio"

Function WaitForPIOIRQ:Int(controller:Int, irqLine:UInt, timeoutMillis:Int)
	Local deadline:Int = MilliSecs() + timeoutMillis
	While PIOIRQPendingEvents(controller, irqLine) = 0
		If MilliSecs() >= deadline Then Return False
		Delay(1)
	Wend
	Return True
End Function

Local checksPassed:Int = True
Local program:TPIOProgram = PIOProgram("irq_led")
checksPassed :& program <> Null And program.IsValid()
If program Then
	checksPassed :& program.Length() = 5
	checksPassed :& program.Version() = 0
	checksPassed :& program.WrapTarget() = 0 And program.Wrap() = 4
End If

Const controller:Int = PIOController0
Const irqLine:UInt = PIOIRQLine0
Const interruptNumber:UInt = 0
Local irqSource:UInt = PIOIRQInterruptMask(interruptNumber)
checksPassed :& irqSource <> 0
checksPassed :& (PIOIRQSupportedSources() & irqSource) = irqSource

Local stateMachine:Int = PIOClaimUnusedStateMachine(controller)
checksPassed :& stateMachine >= 0

Local offset:Int = PIOErrorInvalidArgument
If stateMachine >= 0 And program Then
	offset = PIOAddProgram(controller, program)
	checksPassed :& offset >= 0
End If

If stateMachine >= 0 And offset >= 0 Then
	Local ledPin:UInt = DefaultLEDPin()
	checksPassed :& PIOGPIOInit(controller, ledPin)
	checksPassed :& PIOStateMachineSetConsecutivePinDirections(controller, stateMachine, ledPin, 1, True) >= 0
	checksPassed :& PIOStateMachineInitProgram(controller, stateMachine, program, offset) >= 0
	checksPassed :& PIOStateMachineSetOutPins(controller, stateMachine, ledPin, 1)
	checksPassed :& PIOStateMachineSetClockDivider(controller, stateMachine, 1000.0)
	checksPassed :& PIOStateMachineClearFIFOs(controller, stateMachine)
	checksPassed :& PIOInterruptClear(controller, interruptNumber)
	checksPassed :& PIOIRQSetSourcesEnabled(controller, irqLine, irqSource, True)
	checksPassed :& (PIOIRQEnabledSources(controller, irqLine) & irqSource) <> 0
	checksPassed :& (PIOIRQArmedSources(controller, irqLine) & irqSource) <> 0
	checksPassed :& PIOStateMachineRestart(controller, stateMachine)
	checksPassed :& PIOStateMachineRestartClockDivider(controller, stateMachine)
	checksPassed :& PIOStateMachineSetEnabled(controller, stateMachine, True)
End If

If Not checksPassed Then
	While True
		Print "PIO IRQ setup check failed"
		Delay(1000)
	Wend
End If

Local level:UInt = 1
Const delayCount:UInt = 60000

While True
	Local command:UInt = (delayCount Shl 1) | level
	PIOStateMachinePutBlocking(controller, stateMachine, command)

	If Not WaitForPIOIRQ(controller, irqLine, 1500) Then
		Print "PIO IRQ event timed out"
		Continue
	End If

	Local events:UInt = PIOIRQTakeEvents(controller, irqLine)
	Local eventPassed:Int = (events & irqSource) <> 0
	eventPassed :& (PIOIRQArmedSources(controller, irqLine) & irqSource) = 0
	eventPassed :& PIOInterruptIsSet(controller, interruptNumber)
	eventPassed :& PIOInterruptClear(controller, interruptNumber)
	eventPassed :& Not PIOInterruptIsSet(controller, interruptNumber)
	eventPassed :& PIOIRQRearmSources(controller, irqLine, irqSource)
	eventPassed :& (PIOIRQArmedSources(controller, irqLine) & irqSource) <> 0

	If eventPassed Then
		Print "PIO IRQ event handled: LED=" + level + " events=" + events
	Else
		Print "PIO IRQ event check failed"
	End If

	level :~ 1
Wend
