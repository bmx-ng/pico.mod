SuperStrict

Import BRL.StandardIO
Import Pico.Board.Pico
Import Pico.Hardware.PIO
Import "pio_fifo_led.pio"

Function WaitForEcho:Int(controller:Int, stateMachine:UInt, expected:UInt, timeoutMillis:Int)
	Local deadline:Int = MilliSecs() + timeoutMillis
	While PIOStateMachineRXEmpty(controller, stateMachine)
		If MilliSecs() >= deadline Then Return False
		Delay(1)
	Wend
	Return PIOStateMachineGet(controller, stateMachine) = expected
End Function

Local checksPassed:Int = True
Local program:TPIOProgram = PIOProgram("fifo_led")
checksPassed :& program <> Null And program.IsValid()
If program Then
	checksPassed :& program.Length() = 6
	checksPassed :& program.Version() = 0
	checksPassed :& program.WrapTarget() = 0 And program.Wrap() = 5
End If

Const controller:Int = PIOController0
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

	' A normal, unjoined PIO TX FIFO holds four words. Since the state machine is
	' still disabled, this deterministically exercises non-blocking writes.
	For Local index:Int = 0 Until 4
		checksPassed :& PIOStateMachinePut(controller, stateMachine, UInt(index))
	Next
	checksPassed :& PIOStateMachineTXLevel(controller, stateMachine) = 4
	checksPassed :& PIOStateMachineTXFull(controller, stateMachine)
	checksPassed :& Not PIOStateMachinePut(controller, stateMachine, $ffffffff:UInt)
	checksPassed :& PIOStateMachineClearFIFOs(controller, stateMachine)
	checksPassed :& PIOStateMachineTXEmpty(controller, stateMachine)
	checksPassed :& PIOStateMachineRXEmpty(controller, stateMachine)
	checksPassed :& PIOStateMachineRestart(controller, stateMachine)
	checksPassed :& PIOStateMachineRestartClockDivider(controller, stateMachine)
	checksPassed :& PIOStateMachineSetEnabled(controller, stateMachine, True)
End If

If Not checksPassed Then
	Print "PIO FIFO setup check failed"
	While True
		Delay(1000)
	Wend
End If

Local level:UInt = 1
Const delayCount:UInt = 60000

While True
	Local command:UInt = (delayCount Shl 1) | level
	PIOStateMachinePutBlocking(controller, stateMachine, command)

	If WaitForEcho(controller, stateMachine, command, 1000) Then
		Print "PIO FIFO command acknowledged: LED=" + level + ..
			" TX=" + PIOStateMachineTXLevel(controller, stateMachine) + ..
			" RX=" + PIOStateMachineRXLevel(controller, stateMachine)
	Else
		Print "PIO FIFO acknowledgement timed out"
	End If

	level :~ 1
Wend
