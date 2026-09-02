' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Module Pico.Tests.NativeGraph
?pico

Import "../native/native_value.h"
Import "../native/native_value.c"

'@bmk push cc_opts
'@bmk addccopt native_graph_lexical -DBMX_PICO_NATIVE_LEXICAL=19
Import "../native/native_value.cpp"
'@bmk pop cc_opts

Extern
	Function NativeCValue:Int(value:Int) = "bmx_pico_native_c_value"
	Function NativeCPPValue:Int(value:Int) = "bmx_pico_native_cpp_value"
End Extern
?
