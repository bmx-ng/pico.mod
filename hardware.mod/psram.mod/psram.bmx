' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: External PSRAM information for RP2350 boards.
about: The selected Pico SDK board definition controls PSRAM initialization.
Importing this module links the SDK's PSRAM runtime support. Managed-heap
placement is selected separately with bmk's `-heap-region psram` option.
End Rem
Module Pico.Hardware.PSRAM
?pico

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"

Import Pico.Core

Extern "C"
	Rem
	bbdoc: Returns True when external PSRAM was initialized successfully.
	End Rem
	Function PSRAMAvailable:Int() = "bmx_pico_psram_available"

	Rem
	bbdoc: Returns the initialized PSRAM capacity in bytes, or zero when unavailable.
	End Rem
	Function PSRAMCapacity:UInt() = "bmx_pico_psram_capacity"

	Rem
	bbdoc: Returns True when @address lies within initialized PSRAM.
	End Rem
	Function PSRAMContains:Int(address:Byte Ptr) = "bmx_pico_psram_contains"
End Extern
?
