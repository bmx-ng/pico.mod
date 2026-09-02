' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Direct-memory-access channels for Raspberry Pi Pico targets.
End Rem
Module Pico.Hardware.DMA
?pico

ModuleInfo "Version: 0.3"
ModuleInfo "License: zlib/libpng"

Import BRL.Blitz

Const DMADataSize8:UInt = 0
Const DMADataSize16:UInt = 1
Const DMADataSize32:UInt = 2

Const DMAIRQLine0:UInt = 0
Const DMAIRQLine1:UInt = 1

Extern "C"
	Function DMAChannelCount:UInt() = "bmx_pico_dma_channel_count"
	Function DMAIRQLineCount:UInt() = "bmx_pico_dma_irq_line_count"
	Function DMAForceDREQ:UInt() = "bmx_pico_dma_force_dreq"

	Function DMAClaimUnusedChannel:Int() = "bmx_pico_dma_claim_unused_channel"
	Function DMAChannelIsClaimed:Int(channel:UInt) = "bmx_pico_dma_channel_is_claimed"
	Function DMAUnclaimChannel:Int(channel:UInt) = "bmx_pico_dma_unclaim_channel"

	Rem
	bbdoc: Configures a finite DMA transfer and optionally starts it.
	about: transferCount counts elements of dataSize, not bytes. Any managed Array
	whose payload is used by this transfer must remain strongly reachable until
	the channel completes or is aborted. MemAlloc storage must not be freed early.
	End Rem
	Function DMAConfigure:Int(channel:UInt, readAddress:Byte Ptr, writeAddress:Byte Ptr, transferCount:UInt, dataSize:UInt, readIncrement:Int, writeIncrement:Int, dreq:UInt, start:Int) = "bmx_pico_dma_configure"
	Function DMAStart:Int(channel:UInt) = "bmx_pico_dma_start"
	Function DMAAbort:Int(channel:UInt) = "bmx_pico_dma_abort"
	Function DMABusy:Int(channel:UInt) = "bmx_pico_dma_busy"
	Function DMARemaining:UInt(channel:UInt) = "bmx_pico_dma_remaining"

	Rem
	bbdoc: Enables native completion-event counting for a channel on core 0.
	about: The IRQ handler acknowledges hardware and increments a saturating
	counter. It never calls BlitzMax code or touches managed objects.
	End Rem
	Function DMASetIRQEnabled:Int(channel:UInt, irqLine:UInt, enabled:Int) = "bmx_pico_dma_set_irq_enabled"
	Function DMAPendingCompletionEvents:UInt(channel:UInt, irqLine:UInt) = "bmx_pico_dma_pending_completion_events"
	Function DMATakeCompletionEvents:UInt(channel:UInt, irqLine:UInt) = "bmx_pico_dma_take_completion_events"
End Extern

Rem
bbdoc: A retained, type-safe view of a managed Array slice used by DMA.
about: FromArray infers the DMA transfer width from the element type. The view
keeps the Array alive and its address valid until every transfer retaining the
view is closed or finalized.
End Rem
Type TDMABuffer
	Private
	Field address:Byte Ptr
	Field elementCount:UInt
	Field dataSize:UInt
	Field bytes:Byte[]
	Field shorts:Short[]
	Field ints:Int[]
	Field uints:UInt[]
	Field floats:Float[]
	Field borrowed:Int

	Public
	Function FromArray:TDMABuffer(values:Byte[], offset:UInt = 0, count:UInt = 0)
		If offset > values.length Then Return Null
		Local available:UInt = UInt(values.length) - offset
		If count = 0 Then count = available
		If count = 0 Or count > available Then Return Null
		Local buffer:TDMABuffer = New TDMABuffer
		buffer.address = Byte Ptr(values) + offset
		buffer.elementCount = count
		buffer.dataSize = DMADataSize8
		buffer.bytes = values
		Return buffer
	End Function

	Function FromArray:TDMABuffer(values:Short[], offset:UInt = 0, count:UInt = 0)
		If offset > values.length Then Return Null
		Local available:UInt = UInt(values.length) - offset
		If count = 0 Then count = available
		If count = 0 Or count > available Then Return Null
		Local buffer:TDMABuffer = New TDMABuffer
		buffer.address = Byte Ptr(values) + offset * 2
		buffer.elementCount = count
		buffer.dataSize = DMADataSize16
		buffer.shorts = values
		Return buffer
	End Function

	Function FromArray:TDMABuffer(values:Int[], offset:UInt = 0, count:UInt = 0)
		If offset > values.length Then Return Null
		Local available:UInt = UInt(values.length) - offset
		If count = 0 Then count = available
		If count = 0 Or count > available Then Return Null
		Local buffer:TDMABuffer = New TDMABuffer
		buffer.address = Byte Ptr(values) + offset * 4
		buffer.elementCount = count
		buffer.dataSize = DMADataSize32
		buffer.ints = values
		Return buffer
	End Function

	Function FromArray:TDMABuffer(values:UInt[], offset:UInt = 0, count:UInt = 0)
		If offset > values.length Then Return Null
		Local available:UInt = UInt(values.length) - offset
		If count = 0 Then count = available
		If count = 0 Or count > available Then Return Null
		Local buffer:TDMABuffer = New TDMABuffer
		buffer.address = Byte Ptr(values) + offset * 4
		buffer.elementCount = count
		buffer.dataSize = DMADataSize32
		buffer.uints = values
		Return buffer
	End Function

	Function FromArray:TDMABuffer(values:Float[], offset:UInt = 0, count:UInt = 0)
		If offset > values.length Then Return Null
		Local available:UInt = UInt(values.length) - offset
		If count = 0 Then count = available
		If count = 0 Or count > available Then Return Null
		Local buffer:TDMABuffer = New TDMABuffer
		buffer.address = Byte Ptr(values) + offset * 4
		buffer.elementCount = count
		buffer.dataSize = DMADataSize32
		buffer.floats = values
		Return buffer
	End Function

	Rem
	bbdoc: Creates a non-owning DMA view over caller-managed storage.
	about: elementSize is the size of one transfer element in bytes and must be
	1, 2, or 4. SizeOf(T) can be used when the pointer addresses T elements.
	The caller must keep the storage valid and suitably aligned until every
	transfer retaining this view has completed or been aborted and closed.
	Borrow never frees the storage; MemAlloc storage still requires MemFree.
	End Rem
	Function Borrow:TDMABuffer(address:Byte Ptr, elementCount:UInt, elementSize:UInt)
		If address = Null Or elementCount = 0 Then Return Null
		Local dataSize:UInt
		Select elementSize
			Case 1
				dataSize = DMADataSize8
			Case 2
				dataSize = DMADataSize16
			Case 4
				dataSize = DMADataSize32
			Default
				Return Null
		End Select
		Local buffer:TDMABuffer = New TDMABuffer
		buffer.address = address
		buffer.elementCount = elementCount
		buffer.dataSize = dataSize
		buffer.borrowed = True
		Return buffer
	End Function

	Method Address:Byte Ptr()
		Return address
	End Method

	Method ElementCount:UInt()
		Return elementCount
	End Method

	Method DataSize:UInt()
		Return dataSize
	End Method

	Method ElementSize:UInt()
		Return 1 Shl dataSize
	End Method

	Method ByteLength:UInt()
		Return elementCount Shl dataSize
	End Method

	Method IsBorrowed:Int()
		Return borrowed
	End Method

	Method Bytes:Byte[]()
		Return bytes
	End Method

	Method Shorts:Short[]()
		Return shorts
	End Method

	Method Ints:Int[]()
		Return ints
	End Method

	Method UInts:UInt[]()
		Return uints
	End Method

	Method Floats:Float[]()
		Return floats
	End Method
End Type

Rem
bbdoc: Owns one finite DMA channel and retains its participating managed buffers.
about: Transfers are configured but not started by DMAReadInto, DMAWriteFrom,
or DMACopy. Call Start once, then poll IsComplete or call Wait. Close is
idempotent, aborts an active transfer, releases the channel, and releases the
managed buffers. Delete calls Close as a final safety net; deterministic Close
or a Using block remains preferable for scarce DMA channels.
End Rem
Type TDMATransfer Implements ICloseable
	Private
	Field channel:Int = -1
	Field irqLine:UInt
	Field readBuffer:TDMABuffer
	Field writeBuffer:TDMABuffer
	Field started:Int
	Field aborted:Int

	Public
	Method _Configure:Int(readAddress:Byte Ptr, writeAddress:Byte Ptr, transferCount:UInt, ..
			dataSize:UInt, readIncrement:Int, writeIncrement:Int, dreq:UInt, ..
			irqLine:UInt, readBuffer:TDMABuffer, writeBuffer:TDMABuffer)
		If channel >= 0 Or irqLine >= DMAIRQLineCount() Then Return False
		channel = DMAClaimUnusedChannel()
		If channel < 0 Then Return False
		Self.irqLine = irqLine
		Self.readBuffer = readBuffer
		Self.writeBuffer = writeBuffer
		If Not DMASetIRQEnabled(channel, irqLine, True) Or ..
				Not DMAConfigure(channel, readAddress, writeAddress, transferCount, ..
				dataSize, readIncrement, writeIncrement, dreq, False) Then
			Close()
			Return False
		End If
		Return True
	End Method

	Method Channel:Int()
		Return channel
	End Method

	Method IRQLine:UInt()
		Return irqLine
	End Method

	Method ReadBuffer:TDMABuffer()
		Return readBuffer
	End Method

	Method WriteBuffer:TDMABuffer()
		Return writeBuffer
	End Method

	Method IsOpen:Int()
		Return channel >= 0
	End Method

	Method Start:Int()
		If channel < 0 Or started Or aborted Then Return False
		started = DMAStart(channel)
		Return started
	End Method

	Method IsBusy:Int()
		Return channel >= 0 And DMABusy(channel)
	End Method

	Method Remaining:UInt()
		If channel < 0 Then Return 0
		Return DMARemaining(channel)
	End Method

	Method IsComplete:Int()
		Return channel >= 0 And started And Not aborted And ..
			Not DMABusy(channel) And DMARemaining(channel) = 0
	End Method

	Method PendingCompletionEvents:UInt()
		If channel < 0 Then Return 0
		Return DMAPendingCompletionEvents(channel, irqLine)
	End Method

	Method TakeCompletionEvents:UInt()
		If channel < 0 Then Return 0
		Return DMATakeCompletionEvents(channel, irqLine)
	End Method

	Method Wait:Int(timeoutMillis:Int = -1)
		If channel < 0 Or Not started Or aborted Then Return False
		Local deadline:Int = MilliSecs() + timeoutMillis
		While Not IsComplete()
			If timeoutMillis >= 0 And MilliSecs() >= deadline Then Return False
			Delay(1)
		Wend
		Return True
	End Method

	Method Abort:Int()
		If channel < 0 Or aborted Then Return False
		If DMABusy(channel) And Not DMAAbort(channel) Then Return False
		aborted = True
		Return True
	End Method

	Method Close()
		If channel < 0 Then Return
		If DMABusy(channel) Then DMAAbort(channel)
		If DMAUnclaimChannel(channel) Then
			channel = -1
			readBuffer = Null
			writeBuffer = Null
		End If
	End Method

	Method Delete()
		Close()
	End Method
End Type

Rem
bbdoc: Creates a one-shot peripheral-to-managed-buffer DMA transfer.
about: The source address is not owned and must remain valid until completion
or abort. The destination buffer and its Array are retained by the transfer.
End Rem
Function DMAReadInto:TDMATransfer(sourceAddress:Byte Ptr, destination:TDMABuffer, ..
		dreq:UInt, irqLine:UInt = DMAIRQLine0)
	If sourceAddress = Null Or destination = Null Then Return Null
	Local transfer:TDMATransfer = New TDMATransfer
	If Not transfer._Configure(sourceAddress, destination.Address(), ..
			destination.ElementCount(), destination.DataSize(), False, True, ..
			dreq, irqLine, Null, destination) Then Return Null
	Return transfer
End Function

Rem
bbdoc: Creates a one-shot managed-buffer-to-peripheral DMA transfer.
about: The destination address is not owned and must remain valid until
completion or abort. The source buffer and its Array are retained.
End Rem
Function DMAWriteFrom:TDMATransfer(source:TDMABuffer, destinationAddress:Byte Ptr, ..
		dreq:UInt, irqLine:UInt = DMAIRQLine0)
	If source = Null Or destinationAddress = Null Then Return Null
	Local transfer:TDMATransfer = New TDMATransfer
	If Not transfer._Configure(source.Address(), destinationAddress, ..
			source.ElementCount(), source.DataSize(), True, False, dreq, irqLine, ..
			source, Null) Then Return Null
	Return transfer
End Function

Rem
bbdoc: Creates a one-shot managed-buffer-to-managed-buffer DMA copy.
about: Source and destination must have matching element widths and the
destination must be at least as long as the source slice. Both Arrays are
retained until Close or finalization. Source and destination slices must not
overlap; use MemMove when overlap is possible.
End Rem
Function DMACopy:TDMATransfer(source:TDMABuffer, destination:TDMABuffer, ..
		irqLine:UInt = DMAIRQLine0)
	If source = Null Or destination = Null Or ..
			source.DataSize() <> destination.DataSize() Or ..
			destination.ElementCount() < source.ElementCount() Then Return Null
	Local transfer:TDMATransfer = New TDMATransfer
	If Not transfer._Configure(source.Address(), destination.Address(), ..
			source.ElementCount(), source.DataSize(), True, True, DMAForceDREQ(), ..
			irqLine, source, destination) Then Return Null
	Return transfer
End Function
?
