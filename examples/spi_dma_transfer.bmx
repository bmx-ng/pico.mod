SuperStrict

Import BRL.StandardIO
Import Pico.Hardware.DMA
Import Pico.Hardware.GPIO
Import Pico.Hardware.SPI

Function WaitForSPIIdle:Int(controller:Int, timeoutMillis:Int)
	Local deadline:Int = MilliSecs() + timeoutMillis
	While SPIIsBusy(controller)
		If MilliSecs() >= deadline Then Return False
	Wend
	Return True
End Function

Function TransferAndCheck:Int(controller:Int, source:Byte[], destination:Byte[], expected:Byte)
	Local receive:TDMATransfer = DMAReadInto(SPIDataRegisterAddress(controller), ..
		TDMABuffer.FromArray(destination), SPIRXDREQ(controller))
	Local transmit:TDMATransfer = DMAWriteFrom(TDMABuffer.FromArray(source), ..
		SPIDataRegisterAddress(controller), SPITXDREQ(controller))
	If Not receive Or Not transmit Then
		If receive Then receive.Close()
		If transmit Then transmit.Close()
		Return False
	End If

	Local passed:Int = receive.Start()
	If passed Then passed = transmit.Start()
	If passed Then passed = transmit.Wait(1000)
	If passed Then passed = receive.Wait(1000)
	If passed Then passed = WaitForSPIIdle(controller, 1000)
	If passed Then passed = transmit.TakeCompletionEvents() = 1
	If passed Then passed = receive.TakeCompletionEvents() = 1
	If passed Then passed = transmit.IsComplete() And receive.IsComplete()
	If passed Then passed = Not transmit.IsBusy() And Not receive.IsBusy()
	transmit.Close()
	receive.Close()

	For Local index:Int = 0 Until destination.length
		passed :& destination[index] = expected
	Next
	Return passed
End Function

Const byteCount:Int = 64

Local controller:Int = SPIDefaultController()
Local rxPin:UInt = SPIDefaultRXPin()
Local txPin:UInt = SPIDefaultTXPin()
Local clockPin:UInt = SPIDefaultClockPin()
Local checksPassed:Int = SPIConfigurePins(controller, rxPin, txPin, clockPin)
checksPassed :& SPIInit(controller, 1000000) > 0
checksPassed :& SPISetFormat(controller, 8, SPIClockPolarity0, SPIClockPhase0, SPIBitOrderMSBFirst)
checksPassed :& SPIDataRegisterAddress(controller) <> Null
checksPassed :& SPITXDREQ(controller) <> SPIRXDREQ(controller)
checksPassed :& SPITXDREQ(controller) < DMAForceDREQ() And SPIRXDREQ(controller) < DMAForceDREQ()
checksPassed :& SPIDataRegisterAddress(2) = Null

Local source:Byte[] = New Byte[byteCount]
Local destination:Byte[] = New Byte[byteCount]
For Local index:Int = 0 Until byteCount
	source[index] = Byte((index * 37 + 11) & $ff)
Next

While True
	Local transferPassed:Int = checksPassed
	If transferPassed Then
		GPIOPullDown(rxPin)
		transferPassed :& TransferAndCheck(controller, source, destination, $00)
		GPIOPullUp(rxPin)
		transferPassed :& TransferAndCheck(controller, source, destination, $ff)
	End If

	If transferPassed Then
		Print "SPI DMA passed: two " + byteCount + "-byte full-duplex transfers"
	Else
		Print "SPI DMA transfer failed"
	End If
	Delay(1000)
Wend
