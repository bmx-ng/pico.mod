SuperStrict

Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.Runtime.Memory
Import Pico.System.Time

Type TArrayItem
	Field value:Int

	Method New(value:Int)
		Self.value = value
	End Method
End Type

Struct SArrayEntry
	Field label:String

	Method New(label:String)
		Self.label = label
	End Method
End Struct

Function ApplyArrayPressure()
	For Local index:Int = 0 Until 400
		Local temporary:String[] = ["temporary", "array"] + ["value"]
	Next
End Function

StandardIOInit()

Local numbers:Int[] = [1, 2, 3]
numbers :+ [4, 5]
Local names:String[] = ["alpha", "beta"]
Local joined:String[] = names + ["gamma", "delta"]
Local items:TArrayItem[] = [New TArrayItem(7), New TArrayItem(11)]
Local entries:SArrayEntry[] = [New SArrayEntry("left"), New SArrayEntry("right")]

ApplyArrayPressure()
CollectObjects()

Local checksPassed:Int = numbers.Length = 5 And numbers[0] = 1 And numbers[4] = 5 And ..
	names.Length = 2 And names[1] = "beta" And ..
	joined.Length = 4 And joined[0] = "alpha" And joined[3] = "delta" And ..
	items.Length = 2 And items[0].value = 7 And items[1].value = 11 And ..
	entries.Length = 2 And entries[0].label = "left" And entries[1].label = "right" And ..
	AutomaticCollectionCount() > 0 And InvalidReferenceCount() = 0 And ArrayFailureCount() = 0

If checksPassed Then
	PutString("Array literal and concatenation checks passed")
Else
	PutString("Array literal or concatenation check failed")
End If
PutCharacter(10)

Local ledPin:UInt = DefaultLEDPin()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)

While True
	GPIOPut(ledPin, checksPassed)
	If checksPassed Then PutCharacter(46) Else PutCharacter(33)
	SleepMilliseconds(250)
	GPIOPut(ledPin, False)
	SleepMilliseconds(250)
Wend
