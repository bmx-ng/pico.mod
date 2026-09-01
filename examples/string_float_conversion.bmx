SuperStrict

Import BRL.StandardIO
Import Pico.System.Time

' Ordinary shortest-round-trip conversion must not retain fixed formatting.
Local floatText:String = String.FromFloat(1.5:Float)
Local doubleText:String = String.FromDouble(3.141592653589793:Double)
Local checksPassed:Int = floatText = "1.5" And doubleText = "3.141592653589793" And ..
	Float("  +1.5tail") = 1.5:Float And Double(doubleText) = 3.141592653589793:Double

While True
	If checksPassed Then
		Print "Default floating String conversion checks passed"
	Else
		ErrPrint "Default floating String conversion check failed"
	End If
	SleepMilliseconds(1000)
Wend
