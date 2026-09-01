SuperStrict

Import Pico.Runtime.Memory
Import "../shared/base.bmx"

Incbin "value.dat"

Function LeftValue:Int()
	If ArenaCapacity() > 0 And IncbinLen("value.dat") > 0 Then Return LocalIdentity<Int>(BaseValue())
	Return 0
End Function
