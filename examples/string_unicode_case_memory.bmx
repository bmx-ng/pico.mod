SuperStrict

Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.Runtime.Memory
Import Pico.Text.Unicode

Local checksPassed:Int = True

Const UMLAUT_UPPER:String = "123ÄÖÜABC"
Const UMLAUT_LOWER:String = "123äöüabc"
Const CYRILLIC_UPPER:String = "123БУДИНОК"
Const CYRILLIC_LOWER:String = "123будинок"
Const ARABIC:String = "123كلمة"

checksPassed :& UMLAUT_UPPER.ToLower() = UMLAUT_LOWER
checksPassed :& UMLAUT_LOWER.ToUpper() = UMLAUT_UPPER
checksPassed :& CYRILLIC_UPPER.ToLower() = CYRILLIC_LOWER
checksPassed :& CYRILLIC_LOWER.ToUpper() = CYRILLIC_UPPER
checksPassed :& ARABIC.ToLower() = ARABIC And ARABIC.ToUpper() = ARABIC

checksPassed :& UMLAUT_UPPER.Compare(UMLAUT_LOWER, False) = 0
checksPassed :& UMLAUT_UPPER.Equals(UMLAUT_LOWER, False)
checksPassed :& UMLAUT_UPPER.HashCode(False) = UMLAUT_LOWER.HashCode(False)
checksPassed :& Not UMLAUT_UPPER.Equals(UMLAUT_LOWER, True)
checksPassed :& CYRILLIC_UPPER.Equals(CYRILLIC_LOWER, False)
checksPassed :& CYRILLIC_UPPER.HashCode(False) = CYRILLIC_LOWER.HashCode(False)

' Exercise Unicode-result allocation and collection while retaining one converted String.
Local retained:String = CYRILLIC_UPPER.ToLower()
For Local index:Int = 0 Until 500
	Local lower:String = UMLAUT_UPPER.ToLower()
	Local upper:String = CYRILLIC_LOWER.ToUpper()
Next
CollectObjects()
checksPassed :& retained = CYRILLIC_LOWER
checksPassed :& AutomaticCollectionCount() > 0 And InvalidReferenceCount() = 0
checksPassed :& ArenaFailureCount() = 0 And StringFailureCount() = 0

Local ledPin:UInt = DefaultLEDPin()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)
StandardIOInit()

While True
	GPIOPut(ledPin, checksPassed)
	If checksPassed Then
		PutString("Optional Unicode String case checks passed~n")
	Else
		PutString("Optional Unicode String case check failed~n")
	End If
	Delay(1000)
Wend
