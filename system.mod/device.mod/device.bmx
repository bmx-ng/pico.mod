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

Const DeviceResetReasonUnknown:Int = 0
Const DeviceResetReasonPowerOn:Int = 1
Const DeviceResetReasonExternal:Int = 2
Const DeviceResetReasonSoftware:Int = 3
Const DeviceResetReasonWatchdog:Int = 4
Const DeviceResetReasonPanic:Int = 5
Const DeviceResetReasonDeepSleep:Int = 6
Const DeviceResetReasonBrownout:Int = 7
Const DeviceResetReasonPowerGlitch:Int = 8
Const DeviceResetReasonCPULockup:Int = 9

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
	bbdoc: Portable alias for UniqueBoardID.
	End Rem
	Function UniqueDeviceID:String() = "bmx_embedded_unique_device_id"

	Rem
	bbdoc: Portable alias for UniqueBoardIDBytes.
	End Rem
	Function UniqueDeviceIDBytes:Byte[]() = "bmx_embedded_unique_device_id_bytes"

	Rem
	bbdoc: Returns the normalized reason for the last reset.
	about: Pico currently distinguishes watchdog resets from an otherwise unknown cause.
	End Rem
	Function DeviceResetReason:Int() = "bmx_embedded_device_reset_reason"

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

Function DeviceResetReasonName:String(reason:Int)
	Select reason
		Case DeviceResetReasonPowerOn Return "Power on"
		Case DeviceResetReasonExternal Return "External"
		Case DeviceResetReasonSoftware Return "Software"
		Case DeviceResetReasonWatchdog Return "Watchdog"
		Case DeviceResetReasonPanic Return "Panic"
		Case DeviceResetReasonDeepSleep Return "Deep sleep"
		Case DeviceResetReasonBrownout Return "Brownout"
		Case DeviceResetReasonPowerGlitch Return "Power glitch"
		Case DeviceResetReasonCPULockup Return "CPU lockup"
	End Select
	Return "Unknown"
End Function
?
