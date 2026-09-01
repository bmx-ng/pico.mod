SuperStrict

Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.Runtime.Memory

Local checksPassed:Int = True

Local floats:Float[] = "1.5,-2.25,0".SplitFloats(",")
checksPassed :& floats.Length = 3
checksPassed :& floats[0] = 1.5:Float And floats[1] = -2.25:Float And floats[2] = 0.0:Float
Local floatText:String = "|".Join(floats)
Local floatRoundTrip:Float[] = floatText.SplitFloats("|")
checksPassed :& floatRoundTrip.Length = 3
checksPassed :& floatRoundTrip[0] = floats[0] And floatRoundTrip[1] = floats[1] And floatRoundTrip[2] = floats[2]

Local doubles:Double[] = "3.141592653589793::1e-12::-42.5".SplitDoubles("::")
checksPassed :& doubles.Length = 3
checksPassed :& doubles[0] = 3.141592653589793:Double And doubles[1] = 1e-12:Double And doubles[2] = -42.5:Double
Local doubleText:String = ";".Join(doubles)
Local doubleRoundTrip:Double[] = doubleText.SplitDoubles(";")
checksPassed :& doubleRoundTrip.Length = 3
checksPassed :& doubleRoundTrip[0] = doubles[0] And doubleRoundTrip[1] = doubles[1] And doubleRoundTrip[2] = doubles[2]

' Match desktop numeric token rules.
Local whole:Double[] = "  +6.25  ".SplitDoubles("")
Local invalid:Float[] = "1.25x,, 2.5 ".SplitFloats(",")
Local empty:Double[] = "".SplitDoubles(",")
checksPassed :& whole.Length = 1 And whole[0] = 6.25:Double
checksPassed :& invalid.Length = 3 And invalid[0] = 0.0:Float And invalid[1] = 0.0:Float And invalid[2] = 2.5:Float
checksPassed :& empty.Length = 0 And ",".Join(empty) = ""

Local retained:Double[] = "0.125,2.5,1000000000000".SplitDoubles(",")
Local retainedText:String = ":".Join(retained)
For Local index:Int = 0 Until 350
	Local transient:Float[] = "-0.5,1.25,64".SplitFloats(",")
	Local joined:String = "/".Join(transient)
Next
CollectObjects()
Local retainedAgain:Double[] = retainedText.SplitDoubles(":")
checksPassed :& retainedAgain.Length = 3 And retainedAgain[0] = retained[0] And retainedAgain[2] = retained[2]
checksPassed :& AutomaticCollectionCount() > 0 And InvalidReferenceCount() = 0
checksPassed :& ArenaFailureCount() = 0 And StringFailureCount() = 0 And ArrayFailureCount() = 0

Local ledPin:UInt = DefaultLEDPin()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)
StandardIOInit()

While True
	GPIOPut(ledPin, checksPassed)
	If checksPassed Then
		PutString("Floating String Split/Join checks passed~n")
	Else
		PutString("Floating String Split/Join check failed~n")
	End If
	Delay(1000)
Wend
