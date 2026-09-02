' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Device identity, reset, and BOOTSEL services for Pico targets.
End Rem
Module Pico.System.Device
?pico

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"

Extern "C"
	Rem
	bbdoc: Returns the board's 64-bit unique identifier as 16 uppercase hexadecimal characters.
	about: RP2040 boards derive this identifier from the external flash device. RP2350 boards
	derive it from OTP memory. A no-flash build may return the Pico SDK's placeholder value.
	End Rem
	Function UniqueBoardID:String() = "bmx_pico_unique_board_id"

	Rem
	bbdoc: Returns the board's 64-bit unique identifier as eight bytes in SDK order.
	End Rem
	Function UniqueBoardIDBytes:Byte[]() = "bmx_pico_unique_board_id_bytes"

	Rem
	bbdoc: Returns True when the BOOTSEL button is currently pressed.
	about: This samples the external-flash chip-select line and is meaningful only on boards
	that wire a BOOTSEL button in the usual way. The operation briefly suspends flash access,
	masks interrupts, and must be called from core 0. Multicore use is not currently supported.
	End Rem
	Function BootselButtonPressed:Int() = "bmx_pico_bootsel_button_pressed"

	Rem
	bbdoc: Requests a normal reboot and returns True if the delay is accepted.
	about: delayMilliseconds defaults to zero for an immediate reboot. It must not exceed the
	chip's watchdog maximum; an accepted immediate request does not return.
	End Rem
	Function Reboot:Int(delayMilliseconds:UInt = 0) = "bmx_pico_device_reboot"

	Rem
	bbdoc: Reboots into the ROM BOOTSEL loader; returns False only if the request is invalid.
	about: With no arguments, both the UF2 mass-storage and PICOBOOT interfaces are enabled.
	activityPin may be -1 for no activity indication, or a valid GPIO number. A successful
	request does not return. Disabling both USB interfaces is allowed but usually undesirable.
	End Rem
	Function RebootToBootsel:Int(activityPin:Int = -1, activityPinActiveLow:Int = False, disableMassStorage:Int = False, disablePicoboot:Int = False) = "bmx_pico_device_reboot_to_bootsel"
End Extern
?
