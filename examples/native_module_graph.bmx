SuperStrict

Import BRL.StandardIO
Import Pico.Tests.NativeGraph
Import "native_application_value.c"

Extern
	Function NativeApplicationValue:Int(value:Int) = "bmx_pico_native_application_value"
End Extern

Local result:Int = NativeGraphValue(5) + NativeApplicationValue(3)
If result <> 203 Then Throw "Pico native module graph check failed: " + result
Print "Pico native C/C++ and module Incbin checks passed"
