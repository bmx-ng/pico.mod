SuperStrict

Import BRL.StandardIO
Import Pico.Hardware.ADC
Import Pico.Hardware.DMA
Import Pico.Runtime.Memory

Function AbandonConfiguredTransfer:Int()
	Local source:UInt[] = New UInt[2]
	source[0] = $11223344
	source[1] = $55667788
	Local destination:UInt[] = New UInt[2]
	Local transfer:TDMATransfer = DMACopy(TDMABuffer.FromArray(source), ..
		TDMABuffer.FromArray(destination))
	If Not transfer Then Return -1
	Return transfer.Channel()
End Function

Const byteCount:Int = 96
Local source:Byte[] = New Byte[byteCount + 8]
Local destination:Byte[] = New Byte[byteCount + 8]
For Local index:Int = 0 Until source.length
	source[index] = Byte((index * 29 + 7) & $ff)
	destination[index] = $cc
Next

' Copy a slice so pointer offsets, retained buffers, inferred width and bounds
' are all exercised. The transfer becomes the sole owner before collection.
Local sourceBuffer:TDMABuffer = TDMABuffer.FromArray(source, 4, byteCount)
Local destinationBuffer:TDMABuffer = TDMABuffer.FromArray(destination, 6, byteCount)
Local transfer:TDMATransfer = DMACopy(sourceBuffer, destinationBuffer)
Local checksPassed:Int = transfer <> Null
Local channel:Int = -1
If transfer Then channel = transfer.Channel()

source = Null
destination = Null
sourceBuffer = Null
destinationBuffer = Null
CollectObjects()

If transfer Then
	checksPassed :& transfer.IsOpen() And DMAChannelIsClaimed(channel)
	checksPassed :& transfer.ReadBuffer().DataSize() = DMADataSize8
	checksPassed :& transfer.ReadBuffer().ByteLength() = byteCount
	checksPassed :& transfer.Start()
	checksPassed :& transfer.Wait(1000)
	checksPassed :& transfer.IsComplete() And Not transfer.IsBusy()
	checksPassed :& transfer.Remaining() = 0
	checksPassed :& transfer.PendingCompletionEvents() = 1
	checksPassed :& transfer.TakeCompletionEvents() = 1

	Local retainedSource:Byte[] = transfer.ReadBuffer().Bytes()
	Local retainedDestination:Byte[] = transfer.WriteBuffer().Bytes()
	For Local index:Int = 0 Until byteCount
		checksPassed :& retainedDestination[index + 6] = retainedSource[index + 4]
	Next
	checksPassed :& retainedDestination[0] = $cc And retainedDestination[5] = $cc
	checksPassed :& retainedDestination[byteCount + 6] = $cc

	transfer.Close()
	checksPassed :& Not transfer.IsOpen() And Not DMAChannelIsClaimed(channel)
	transfer.Close()
End If

' Borrowed views support manually owned and library-supplied pointer storage.
' The transfer retains each view, but neither view nor transfer frees the memory.
Const borrowedWordCount:UInt = 8
Local borrowedSource:Byte Ptr = MemAlloc(borrowedWordCount * SizeOf(UInt))
Local borrowedDestination:Byte Ptr = MemAlloc(borrowedWordCount * SizeOf(UInt))
checksPassed :& borrowedSource <> Null And borrowedDestination <> Null
If borrowedSource And borrowedDestination Then
	For Local index:UInt = 0 Until borrowedWordCount * SizeOf(UInt)
		borrowedSource[index] = Byte((index * 17 + 3) & $ff)
		borrowedDestination[index] = 0
	Next
	Local borrowedSourceBuffer:TDMABuffer = TDMABuffer.Borrow(borrowedSource, ..
		borrowedWordCount, SizeOf(UInt))
	Local borrowedDestinationBuffer:TDMABuffer = TDMABuffer.Borrow(borrowedDestination, ..
		borrowedWordCount, SizeOf(UInt))
	checksPassed :& borrowedSourceBuffer <> Null And borrowedDestinationBuffer <> Null
	If borrowedSourceBuffer And borrowedDestinationBuffer Then
		checksPassed :& borrowedSourceBuffer.IsBorrowed()
		checksPassed :& borrowedSourceBuffer.ElementSize() = SizeOf(UInt)
		checksPassed :& borrowedSourceBuffer.ByteLength() = borrowedWordCount * SizeOf(UInt)
		Local borrowedTransfer:TDMATransfer = DMACopy(borrowedSourceBuffer, ..
			borrowedDestinationBuffer)
		checksPassed :& borrowedTransfer <> Null
		If borrowedTransfer Then
			checksPassed :& borrowedTransfer.Start() And borrowedTransfer.Wait(1000)
			borrowedTransfer.Close()
			For Local index:UInt = 0 Until borrowedWordCount * SizeOf(UInt)
				checksPassed :& borrowedDestination[index] = borrowedSource[index]
			Next
		End If
	End If
	MemFree(borrowedSource)
	MemFree(borrowedDestination)
End If
checksPassed :& TDMABuffer.Borrow(Null, 1, 1) = Null
checksPassed :& TDMABuffer.Borrow(Byte Ptr(1), 0, 1) = Null
checksPassed :& TDMABuffer.Borrow(Byte Ptr(1), 1, 3) = Null

' An unreachable configured transfer is finalized, releasing its channel even
' if the application forgot an explicit Close.
Local abandonedChannel:Int = AbandonConfiguredTransfer()
checksPassed :& abandonedChannel >= 0 And DMAChannelIsClaimed(abandonedChannel)
CollectObjects()
checksPassed :& Not DMAChannelIsClaimed(abandonedChannel)
CollectObjects()

' A started peripheral transfer stalled on an empty ADC FIFO can be aborted,
' after which its one-shot object refuses to restart and still closes cleanly.
ADCInit()
ADCFIFODrain()
ADCFIFOSetup(True, True, 1, False, False)
Local stalledValues:Short[] = New Short[8]
Local stalled:TDMATransfer = DMAReadInto(ADCFIFOAddress(), ..
	TDMABuffer.FromArray(stalledValues), ADCDREQ())
checksPassed :& stalled <> Null
If stalled Then
	Local stalledChannel:Int = stalled.Channel()
	checksPassed :& stalled.Start() And stalled.IsBusy()
	checksPassed :& stalled.Abort() And Not stalled.IsBusy()
	checksPassed :& Not stalled.IsComplete() And Not stalled.Start()
	checksPassed :& DMAChannelIsClaimed(stalledChannel)
	stalled.Close()
	checksPassed :& Not DMAChannelIsClaimed(stalledChannel)
End If
ADCFIFOSetup(False, False, 1, False, False)

checksPassed :& InvalidReferenceCount() = 0 And ObjectFailureCount() = 0

While True
	If checksPassed Then
		Print "Managed DMA transfer ownership checks passed"
	Else
		Print "Managed DMA transfer ownership check failed"
	End If
	Delay(1000)
Wend
