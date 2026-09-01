SuperStrict

Import BRL.StandardIO
Import Pico.Board.Pico
Import Pico.Hardware.DMA
Import Pico.Hardware.PIO
Import "pio_dma_loopback.pio"

Const controller:Int = PIOController0
Const wordCount:Int = 8
Const delayCount:UInt = 12000

Local checksPassed:Int = DMAChannelCount() >= 12 And DMAIRQLineCount() >= 2
Local program:TPIOProgram = PIOProgram("dma_loopback")
checksPassed :& program <> Null And program.IsValid()

Local stateMachine:Int = PIOClaimUnusedStateMachine(controller)
checksPassed :& stateMachine >= 0

Local offset:Int = PIOErrorInvalidArgument
If stateMachine >= 0 And program Then offset = PIOAddProgram(controller, program)
checksPassed :& offset >= 0

If checksPassed Then
	Local ledPin:UInt = DefaultLEDPin()
	checksPassed :& PIOGPIOInit(controller, ledPin)
	checksPassed :& PIOStateMachineSetConsecutivePinDirections(controller, stateMachine, ledPin, 1, True) >= 0
	checksPassed :& PIOStateMachineInitProgram(controller, stateMachine, program, offset) >= 0
	checksPassed :& PIOStateMachineSetOutPins(controller, stateMachine, ledPin, 1)
	checksPassed :& PIOStateMachineSetClockDivider(controller, stateMachine, 1000.0)
	checksPassed :& PIOStateMachineClearFIFOs(controller, stateMachine)
	checksPassed :& PIOStateMachineSetEnabled(controller, stateMachine, True)
End If

If Not checksPassed Then
	While True
		Print "PIO DMA setup check failed"
		Delay(1000)
	Wend
End If

Local source:UInt[] = New UInt[wordCount]
Local destination:UInt[] = New UInt[wordCount]
For Local index:Int = 0 Until wordCount
	source[index] = (delayCount Shl 1) | UInt(index & 1)
Next

While True
	For Local index:Int = 0 Until wordCount
		destination[index] = 0
	Next

	Local receive:TDMATransfer = DMAReadInto(..
		PIOStateMachineRXFIFOAddress(controller, stateMachine), ..
		TDMABuffer.FromArray(destination), ..
		PIOStateMachineRXDREQ(controller, stateMachine))
	Local transmit:TDMATransfer = DMAWriteFrom(TDMABuffer.FromArray(source), ..
		PIOStateMachineTXFIFOAddress(controller, stateMachine), ..
		PIOStateMachineTXDREQ(controller, stateMachine))
	Local transferPassed:Int = receive <> Null And transmit <> Null
	If receive And transmit Then
		If transferPassed Then transferPassed = receive.Start()
		If transferPassed Then transferPassed = transmit.Start()
		If transferPassed Then transferPassed = transmit.Wait(2000)
		If transferPassed Then transferPassed = receive.Wait(2000)
		If transferPassed Then transferPassed = transmit.TakeCompletionEvents() = 1
		If transferPassed Then transferPassed = receive.TakeCompletionEvents() = 1
		If transferPassed Then transferPassed = transmit.IsComplete() And receive.IsComplete()
		If transferPassed Then transferPassed = Not transmit.IsBusy() And Not receive.IsBusy()
	End If

	For Local index:Int = 0 Until wordCount
		transferPassed :& destination[index] = source[index]
	Next
	If transmit Then transmit.Close()
	If receive Then receive.Close()

	If transferPassed Then
		Print "PIO DMA loopback passed: " + wordCount + " words"
	Else
		Print "PIO DMA loopback failed"
	End If
Wend
