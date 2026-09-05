' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Framework BRL.EventQueue
Import BRL.StandardIO
Import Pico.Hardware.DMA

Const byteCount:Int = 64
Local ringStorage:TDMABuffer = TDMABuffer.Allocate(byteCount, 1, 64)
If Not ringStorage Or Not ringStorage.IsOwned() Then Throw "Unable to allocate aligned DMA storage"
ringStorage.Close()

Local source:Byte[] = New Byte[byteCount]
Local destination:Byte[] = New Byte[byteCount]
For Local index:Int = 0 Until byteCount
	source[index] = Byte((index * 29 + 7) & $ff)
Next

Local sourceBuffer:TDMABuffer = TDMABuffer.FromArray(source)
Local destinationBuffer:TDMABuffer = TDMABuffer.FromArray(destination)
Local pacer:TDMAPacer = TDMAPacer.Create(1, 65535)
If Not pacer Then Throw "Unable to claim DMA pacing timer"

Local settings:SDMAConfig = New SDMAConfig(DMADataSize8, True, True, pacer.DREQ())
settings.HighPriority = True
If settings.DataSize <> DMADataSize8 Or Not settings.ReadIncrement Or ..
		Not settings.WriteIncrement Or settings.DREQ <> pacer.DREQ() Then
	Throw "DMA configuration constructor did not preserve its arguments"
End If

Local transfer:TDMATransfer = DMAAdvancedTransfer(sourceBuffer.Address(), ..
	destinationBuffer.Address(), sourceBuffer.ElementCount(), settings, ..
	DMAIRQLine0, sourceBuffer, destinationBuffer)
If Not transfer Or Not transfer.Start() Then Throw "Unable to start advanced DMA transfer"

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
pacer.Close()

While True
	If checksPassed Then
		Print "Advanced DMA EventQueue checks passed"
	Else
		Print "Advanced DMA EventQueue check failed"
	End If
	Delay 1000
Wend
