SuperStrict

Framework BRL.EventQueue
Import BRL.StandardIO
Import Pico.Hardware.DMA
Import Pico.Runtime.Events

Const byteCount:Int = 64
Local source:Byte[] = New Byte[byteCount]
Local destination:Byte[] = New Byte[byteCount]
For Local index:Int = 0 Until byteCount
	source[index] = Byte((index * 37 + 11) & $ff)
Next

Local transfer:TDMATransfer = DMACopy(TDMABuffer.FromArray(source), ..
	TDMABuffer.FromArray(destination))
If Not transfer Or Not transfer.Start() Then Throw "Unable to start DMA transfer"

Local checksPassed:Int
Repeat
	If WaitEvent() = EVENT_DMACOMPLETE And EventSource() = transfer
		checksPassed = EventData() = transfer.Channel()
		For Local index:Int = 0 Until byteCount
			checksPassed :& destination[index] = source[index]
		Next
		Exit
	End If
Forever

transfer.Close()
checksPassed :& PicoDeferredEventDropped() = 0

While True
	If checksPassed Then
		Print "DMA EventQueue checks passed"
	Else
		Print "DMA EventQueue check failed"
	End If
	Delay 1000
Wend
