SuperStrict

Module Pico.Tests.NativeGraph

ModuleInfo "CC_OPTS: -DBMX_PICO_NATIVE_COMMON=11"
ModuleInfo "C_OPTS: -DBMX_PICO_NATIVE_C_ONLY=13"
ModuleInfo "CPP_OPTS: -DBMX_PICO_NATIVE_CPP_ONLY=17"
ModuleInfo "LD_OPTS: -Wl,--defsym,bmx_pico_native_link_marker=29"

Import "support/native_support.bmx"

Incbin "payload.txt"

Public

Function NativeGraphValue:Int(value:Int)
	Local payload:Byte Ptr = IncbinPtr("payload.txt")
	If payload = Null Or IncbinLen("payload.txt") <> 21 Then Return -1
	Return NativeCValue(value) + NativeCPPValue(value) + payload[0] + IncbinLen("payload.txt")
End Function
