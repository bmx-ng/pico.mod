' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Definitions supplied by the selected Raspberry Pi Pico SDK board.
End Rem
Module Pico.Board
?pico

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"

Import Pico.Core

Extern "C"
	Rem
	bbdoc: Returns the selected board's default LED pin, or PicoUnavailablePin if it has none.
	End Rem
	Function DefaultLEDPin:UInt() = "bmx_pico_default_led_pin"
End Extern
?
