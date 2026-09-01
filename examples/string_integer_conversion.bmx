SuperStrict

Import BRL.StandardIO
Import Pico.System.Time

Local value:Int = -12345
Local text:String = "value=" + value
Local checksPassed:Int = text = "value=-12345" And Int("  -12345tail") = value And ..
	String.FromULong($ffffffffffffffff:ULong) = "18446744073709551615"

While True
	If checksPassed Then
		Print "Integer String conversion checks passed"
	Else
		ErrPrint "Integer String conversion check failed"
	End If
	SleepMilliseconds(1000)
Wend
