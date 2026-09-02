' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Board definitions for the original Raspberry Pi Pico.
End Rem
Module Pico.Board.Pico
?pico

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"

Extern "C"
	Function DefaultLEDPin:UInt() = "bmx_pico_default_led_pin"
End Extern
?
