SuperStrict

Rem
bbdoc: Board definitions for Raspberry Pi Pico 2.
End Rem
Module Pico.Board.Pico2

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"

Extern "C"
	Function DefaultLEDPin:UInt() = "bmx_pico_default_led_pin"
End Extern
