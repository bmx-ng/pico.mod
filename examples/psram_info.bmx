' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Framework BRL.StandardIO
Import Pico.Hardware.PSRAM
Import Pico.Runtime.Memory

Print "PSRAM available: " + PSRAMAvailable()
Print "PSRAM capacity: " + PSRAMCapacity()
Print "Managed heap capacity: " + ArenaCapacity()

While True
	Delay 1000
Wend
