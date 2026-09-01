SuperStrict

Rem
bbdoc: UART and USB standard I/O selection for Pico targets.
End Rem
Module Pico.IO.StandardIO

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"

Extern "C"
	Function StandardIOInit:Int() = "bmx_pico_stdio_init_all"
	Function PutCharacter:Int(character:Int) = "bmx_pico_putchar_raw"
	Function PutString:Int(text:String) = "bmx_pico_put_string"
End Extern
