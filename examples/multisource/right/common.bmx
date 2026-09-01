SuperStrict

Incbin "value.dat"

Function RightValue:Int()
	Print "right source ready"
	If IncbinLen("value.dat") > 0 Then Return 2
	Return 0
End Function
