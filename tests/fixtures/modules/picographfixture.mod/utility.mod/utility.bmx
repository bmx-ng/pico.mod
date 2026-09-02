' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Module PicoGraphFixture.Utility
?pico

ModuleInfo "CC_OPTS: -DBMX_PICO_GRAPH_MODULE_VALUE=5"

Import "include/*.h"
Import "native.c"

Extern
	Function PicoGraphNativeValue:Int(value:Int) = "bmx_pico_graph_native_value"
End Extern

Function PicoGraphFixtureValue:Int(value:Int)
	Return PicoGraphNativeValue(value)
End Function
?
