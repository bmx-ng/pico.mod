SuperStrict

Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO

Local checksPassed:Int = True

checksPassed :& "Alpha".Compare("aLPHa", False) = 0
checksPassed :& "Alpha".Equals("aLPHa", False)
checksPassed :& "Alpha".HashCode(False) = "aLPHa".HashCode(False)
checksPassed :& "Alpha".Compare("aLPHa", True) <> 0
checksPassed :& Not "Alpha".Equals("aLPHa", True)
checksPassed :& "Alpha".HashCode(True) = "Alpha".HashCode()

' Without Pico.Text.Unicode, the compact profile intentionally folds ASCII only.
checksPassed :& "ÄÖÜ".ToLower() = "ÄÖÜ"
checksPassed :& Not "ÄÖÜ".Equals("äöü", False)

Local ledPin:UInt = DefaultLEDPin()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)
StandardIOInit()

While True
	GPIOPut(ledPin, checksPassed)
	If checksPassed Then
		PutString("Compact String case checks passed~n")
	Else
		PutString("Compact String case check failed~n")
	End If
	Delay(1000)
Wend
