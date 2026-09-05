' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Direct-memory-access channels for Raspberry Pi Pico targets.
End Rem
Module Pico.Hardware.DMA
?pico

ModuleInfo "Version: 0.4"
ModuleInfo "License: zlib/libpng"

Import BRL.Blitz
Import BRL.Event
Import Pico.Runtime.Events

Const DMADataSize8:UInt = 0
Const DMADataSize16:UInt = 1
Const DMADataSize32:UInt = 2

Const DMAIRQLine0:UInt = 0
Const DMAIRQLine1:UInt = 1
Const DMANoChain:Int = -1

Rem
bbdoc: Advanced configuration for one DMA channel.
about: RingSizeBits is zero to disable address wrapping, or 1 through 15 to
wrap the selected address at a power-of-two boundary. The selected read or
write address must be aligned to the same boundary. ChainTo is #DMANoChain for
no chain, otherwise it must name another claimed DMA channel. QuietIRQ disables
ordinary end-of-transfer completion interrupts and therefore also suppresses
#EVENT_DMACOMPLETE for a managed transfer.
End Rem
Struct SDMAConfig
	Field DataSize:UInt
	Field ReadIncrement:Int
	Field WriteIncrement:Int
	Field DREQ:UInt
	Field HighPriority:Int
	Field ByteSwap:Int
	Field QuietIRQ:Int
	Field RingSizeBits:UInt
	Field RingOnWrite:Int
	Field ChainTo:Int = DMANoChain

	Method New(dataSize:UInt, readIncrement:Int, writeIncrement:Int, dreq:UInt)
		Self.DataSize = dataSize
		Self.ReadIncrement = readIncrement
		Self.WriteIncrement = writeIncrement
		Self.DREQ = dreq
	End Method
End Struct

Rem
bbdoc: Emitted when a #TDMATransfer completes.
about: The event source is the completed transfer. EventData contains its DMA
channel and EventX contains the final native DMA control/status value.
End Rem
Global EVENT_DMACOMPLETE:Int = AllocUserEventId("DMAComplete")

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
	Function _DMAConfigureAdvanced:Int(channel:UInt, readAddress:Byte Ptr, writeAddress:Byte Ptr, transferCount:UInt, dataSize:UInt, readIncrement:Int, writeIncrement:Int, dreq:UInt, highPriority:Int, byteSwap:Int, quietIRQ:Int, ringSizeBits:UInt, ringOnWrite:Int, chainTo:Int, start:Int) = "bmx_pico_dma_configure_advanced"
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
	Function _DMASetEventToken:Int(channel:UInt, irqLine:UInt, token:UInt) = "bmx_pico_dma_set_event_token"
	Function DMAPendingCompletionEvents:UInt(channel:UInt, irqLine:UInt) = "bmx_pico_dma_pending_completion_events"
	Function DMATakeCompletionEvents:UInt(channel:UInt, irqLine:UInt) = "bmx_pico_dma_take_completion_events"
	Function _DMABufferAllocate:Byte Ptr(size:UInt, alignment:UInt) = "bmx_pico_dma_buffer_allocate"
	Function _DMABufferFree(buffer:Byte Ptr) = "bmx_pico_dma_buffer_free"

	Function DMATimerCount:UInt() = "bmx_pico_dma_timer_count"
	Function _DMAClaimUnusedTimer:Int() = "bmx_pico_dma_claim_unused_timer"
	Function DMATimerIsClaimed:Int(timer:UInt) = "bmx_pico_dma_timer_is_claimed"
	Function _DMATimerConfigure:Int(timer:UInt, numerator:UInt, denominator:UInt) = "bmx_pico_dma_timer_configure"
	Function _DMATimerUnclaim:Int(timer:UInt) = "bmx_pico_dma_timer_unclaim"
	Function DMATimerDREQ:UInt(timer:UInt) = "bmx_pico_dma_timer_dreq"
End Extern

Rem
bbdoc: Configures a DMA channel using the advanced channel controls.
about: This is the non-owning, channel-level API. Addresses and any claimed
chain target must remain valid until the transfer has stopped. Prefer
#DMAAdvancedTransfer when managed Arrays participate in a transfer.
End Rem
Function DMAConfigureAdvanced:Int(channel:UInt, readAddress:Byte Ptr, ..
		writeAddress:Byte Ptr, transferCount:UInt, settings:SDMAConfig, ..
		start:Int = False)
	Return _DMAConfigureAdvanced(channel, readAddress, writeAddress, transferCount, ..
		settings.DataSize, settings.ReadIncrement, settings.WriteIncrement, ..
		settings.DREQ, settings.HighPriority, settings.ByteSwap, ..
		settings.QuietIRQ, settings.RingSizeBits, settings.RingOnWrite, ..
		settings.ChainTo, start)
End Function

Rem
bbdoc: Owns one of the hardware DMA pacing timers.
about: Configure sets the request rate to system-clock * numerator / denominator.
Both values are in the range 1 through 65535 and numerator may not exceed
denominator. Pass DREQ() to a DMA configuration. Close releases the scarce
timer; a Using block is recommended.
End Rem
Type TDMAPacer Implements ICloseable
	Private
	Field timer:Int = -1
	Field numerator:UInt
	Field denominator:UInt

	Public
	Function Create:TDMAPacer(numerator:UInt, denominator:UInt)
		Local timer:Int = _DMAClaimUnusedTimer()
		If timer < 0 Then Return Null
		Local pacer:TDMAPacer = New TDMAPacer
		pacer.timer = timer
		If Not pacer.Configure(numerator, denominator) Then
			pacer.Close()
			Return Null
		End If
		Return pacer
	End Function

	Method Configure:Int(numerator:UInt, denominator:UInt)
		If timer < 0 Or Not _DMATimerConfigure(timer, numerator, denominator) Then Return False
		Self.numerator = numerator
		Self.denominator = denominator
		Return True
	End Method

	Method Timer:Int()
		Return timer
	End Method

	Method DREQ:UInt()
		If timer < 0 Then Return DMAForceDREQ()
		Return DMATimerDREQ(timer)
	End Method

	Method Numerator:UInt()
		Return numerator
	End Method

	Method Denominator:UInt()
		Return denominator
	End Method

	Method IsOpen:Int()
		Return timer >= 0
	End Method

	Method Close()
		If timer < 0 Then Return
		If _DMATimerUnclaim(timer) Then timer = -1
	End Method

	Method Delete()
		Close()
	End Method
End Type

Rem
bbdoc: A retained, type-safe view of a managed Array slice used by DMA.
about: FromArray infers the DMA transfer width from the element type. The view
keeps the Array alive and its address valid until every transfer retaining the
view is closed or finalized.
End Rem
Type TDMABuffer Implements ICloseable
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
	Field owned:Int
	Field activeTransfers:Int
	Field closeRequested:Int

	Public
	Rem
	bbdoc: Allocates an owned DMA buffer with explicit power-of-two alignment.
	about: elementSize must be 1, 2, or 4. alignment must be a power of two
	between 16 and 32768 bytes. Use 1 Shl RingSizeBits for an address selected by
	DMA ring mode. Close releases the allocation. If Close is called while a
	transfer retains the buffer, release is deferred until that transfer closes.
	End Rem
	Function Allocate:TDMABuffer(elementCount:UInt, elementSize:UInt, alignment:UInt = 16)
		If elementCount = 0 Or elementSize = 0 Then Return Null
		If elementCount > UInt(-1) / elementSize Then Return Null
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
		Local address:Byte Ptr = _DMABufferAllocate(elementCount * elementSize, alignment)
		If address = Null Then Return Null
		Local buffer:TDMABuffer = New TDMABuffer
		buffer.address = address
		buffer.elementCount = elementCount
		buffer.dataSize = dataSize
		buffer.owned = True
		Return buffer
	End Function

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

	Method IsOwned:Int()
		Return owned
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

	Method _Retain:Int()
		If address = Null Or closeRequested Then Return False
		activeTransfers :+ 1
		Return True
	End Method

	Method _Release()
		If activeTransfers > 0 Then activeTransfers :- 1
		If activeTransfers = 0 And closeRequested Then _ReleaseStorage()
	End Method

	Method _ReleaseStorage()
		If owned And address Then _DMABufferFree(address)
		address = Null
		elementCount = 0
		bytes = Null
		shorts = Null
		ints = Null
		uints = Null
		floats = Null
		owned = False
	End Method

	Method Close()
		closeRequested = True
		If activeTransfers = 0 Then _ReleaseStorage()
	End Method

	Method Delete()
		Close()
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
	Field eventToken:UInt
	Field completionEvents:Int

	Public
	Method _Configure:Int(readAddress:Byte Ptr, writeAddress:Byte Ptr, transferCount:UInt, ..
			dataSize:UInt, readIncrement:Int, writeIncrement:Int, dreq:UInt, ..
			irqLine:UInt, readBuffer:TDMABuffer, writeBuffer:TDMABuffer)
		Local settings:SDMAConfig = New SDMAConfig(dataSize, readIncrement, ..
			writeIncrement, dreq)
		Return _ConfigureAdvanced(readAddress, writeAddress, transferCount, settings, ..
			irqLine, readBuffer, writeBuffer)
	End Method

	Method _ConfigureAdvanced:Int(readAddress:Byte Ptr, writeAddress:Byte Ptr, ..
			transferCount:UInt, settings:SDMAConfig, irqLine:UInt, ..
			readBuffer:TDMABuffer, writeBuffer:TDMABuffer)
		If channel >= 0 Or irqLine >= DMAIRQLineCount() Then Return False
		channel = DMAClaimUnusedChannel()
		If channel < 0 Then Return False
		If readBuffer And Not readBuffer._Retain() Then
			DMAUnclaimChannel(channel)
			channel = -1
			Return False
		End If
		If writeBuffer And Not writeBuffer._Retain() Then
			If readBuffer Then readBuffer._Release()
			DMAUnclaimChannel(channel)
			channel = -1
			Return False
		End If
		Self.irqLine = irqLine
		Self.readBuffer = readBuffer
		Self.writeBuffer = writeBuffer
		completionEvents = Not settings.QuietIRQ
		If (completionEvents And Not DMASetIRQEnabled(channel, irqLine, True)) Or ..
				Not DMAConfigureAdvanced(channel, readAddress, writeAddress, ..
				transferCount, settings, False) Then
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
		If Not completionEvents Then
			started = DMAStart(channel)
			Return started
		End If
		eventToken = RegisterPicoEventSource(Self, EVENT_DMACOMPLETE)
		If eventToken = 0 Or Not _DMASetEventToken(channel, irqLine, eventToken) Then
			If eventToken Then ReleasePicoEventSource(eventToken)
			eventToken = 0
			Return False
		End If
		started = DMAStart(channel)
		If Not started
			_DMASetEventToken(channel, irqLine, 0)
			ReleasePicoEventSource(eventToken)
			eventToken = 0
		End If
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

	Method EmitsCompletionEvents:Int()
		Return completionEvents
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
		_DMASetEventToken(channel, irqLine, 0)
		If eventToken Then
			ReleasePicoEventSource(eventToken)
			eventToken = 0
		End If
		If DMAUnclaimChannel(channel) Then
			channel = -1
			If readBuffer Then readBuffer._Release()
			If writeBuffer Then writeBuffer._Release()
			readBuffer = Null
			writeBuffer = Null
		End If
	End Method

	Method Delete()
		Close()
	End Method
End Type

Rem
bbdoc: Creates an advanced finite DMA transfer while retaining participating buffers.
about: A supplied TDMABuffer keeps its managed Array reachable. A raw address
remains the caller's responsibility. Ring mode requires the selected address to
be aligned to 1 Shl RingSizeBits bytes. The transfer is configured but not
started.
End Rem
Function DMAAdvancedTransfer:TDMATransfer(readAddress:Byte Ptr, ..
		writeAddress:Byte Ptr, transferCount:UInt, settings:SDMAConfig, ..
		irqLine:UInt = DMAIRQLine0, readBuffer:TDMABuffer = Null, ..
		writeBuffer:TDMABuffer = Null)
	If readAddress = Null Or writeAddress = Null Or transferCount = 0 Then Return Null
	Local transfer:TDMATransfer = New TDMATransfer
	If Not transfer._ConfigureAdvanced(readAddress, writeAddress, transferCount, ..
			settings, irqLine, readBuffer, writeBuffer) Then Return Null
	Return transfer
End Function

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
