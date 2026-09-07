' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Framework BRL.StandardIO
Import Pico.Hardware.PSRAM
Import Pico.Runtime.Memory

Function Check:Int(label:String, condition:Int)
	If condition Then
		Print "PASS: " + label
		Return True
	End If
	Print "FAIL: " + label
	Return False
End Function

Local passed:Int = True
passed :& Check("PSRAM available", PSRAMAvailable())
passed :& Check("PSRAM capacity", PSRAMCapacity() = 8 * 1024 * 1024)
passed :& Check("managed heap capacity", ArenaCapacity() = 8 * 1024 * 1024 - 64 * 1024)

' This allocation cannot fit in RP2350 internal SRAM and therefore proves that
' ordinary managed Array storage is backed by the selected PSRAM heap.
Local bytes:Byte[] = New Byte[2 * 1024 * 1024]
passed :& Check("large managed allocation", bytes.Length = 2 * 1024 * 1024)
passed :& Check("array payload in PSRAM", PSRAMContains(bytes))

For Local offset:Int = 0 Until bytes.Length Step 4096
	bytes[offset] = Byte((offset / 4096) & $ff)
Next
For Local offset:Int = 0 Until bytes.Length Step 4096
	passed :& bytes[offset] = Byte((offset / 4096) & $ff)
Next
passed :& Check("sampled PSRAM read/write", passed)

Local usedBeforeCollection:UInt = ArenaUsed()
CollectObjects()
passed :& Check("root survives collection", bytes[4096] = 1 And ArenaUsed() = usedBeforeCollection)
passed :& Check("collector reference audit", InvalidReferenceCount() = 0)

If passed Then
	Print "PSRAM managed-heap checks passed"
Else
	Print "PSRAM managed-heap checks failed"
End If

While True
	Delay 1000
	If passed Then
		Print "PSRAM managed-heap checks passed"
	Else
		Print "PSRAM managed-heap checks failed"
	End If
Wend
