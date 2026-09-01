SuperStrict

Import BRL.StandardIO
Import Pico.Board.Pico
Import Pico.Hardware.PIO
Import "pio_square_wave.pio"

Local checksPassed:Int = PIOCount() >= 2
checksPassed :& PIOStateMachineCount() = 4
checksPassed :& PIOInstructionCapacity() = 32
checksPassed :& PIOVersion() <= 1

Local program:TPIOProgram = PIOProgram("square_wave")
checksPassed :& program <> Null And program.IsValid()
If program Then
	checksPassed :& program.Length() = 8
	checksPassed :& program.Origin() = PIOProgramAutomaticOrigin
	checksPassed :& program.Version() = 0
	checksPassed :& program.WrapTarget() = 0 And program.Wrap() = 7
End If

Const controller:Int = PIOController0
Local stateMachine:Int = PIOClaimUnusedStateMachine(controller)
checksPassed :& stateMachine >= 0

Local offset:Int = PIOErrorInvalidArgument
If stateMachine >= 0 And program Then
	checksPassed :& PIOStateMachineIsClaimed(controller, stateMachine)
	checksPassed :& PIOCanAddProgram(controller, program)
	offset = PIOAddProgram(controller, program)
	checksPassed :& offset >= 0
End If

If stateMachine >= 0 And offset >= 0 Then
	Local ledPin:UInt = DefaultLEDPin()
	checksPassed :& PIOGPIOInit(controller, ledPin)
	checksPassed :& PIOStateMachineSetConsecutivePinDirections(controller, stateMachine, ledPin, 1, True) >= 0
	checksPassed :& PIOStateMachineInitProgram(controller, stateMachine, program, offset) >= 0
	checksPassed :& PIOStateMachineSetSetPins(controller, stateMachine, ledPin, 1)
	checksPassed :& PIOStateMachineSetClockDivider(controller, stateMachine, 30000.0)
	checksPassed :& PIOStateMachineClearFIFOs(controller, stateMachine)
	checksPassed :& PIOStateMachineTXEmpty(controller, stateMachine)
	checksPassed :& PIOStateMachineRXEmpty(controller, stateMachine)
	checksPassed :& PIOStateMachineTXLevel(controller, stateMachine) = 0
	checksPassed :& PIOStateMachineRXLevel(controller, stateMachine) = 0
	checksPassed :& PIOStateMachineRestart(controller, stateMachine)
	checksPassed :& PIOStateMachineRestartClockDivider(controller, stateMachine)
	checksPassed :& PIOStateMachineSetEnabled(controller, stateMachine, True)
End If

While True
	If checksPassed Then
		Print "PIO program is running; the onboard LED should flash"
	Else
		Print "PIO setup check failed"
	End If
	Delay(1000)
Wend
