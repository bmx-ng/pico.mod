SuperStrict

Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.Runtime.Memory
Import Pico.System.Time
Import Pico.Tests.Generics

StandardIOInit()

Local checksPassed:Int = Identity<Int>(42) = 42
checksPassed :& SelectValue<Int>(True, 42, 0) = 42
checksPassed :& Identity<UInt>(42) = 42

Local numbers:SPair<Int> = New SPair<Int>(19, 23)
Local words:SPair<String> = New SPair<String>("managed" + " first", "managed" + " second")
Local pairs:SPair<String>[] = New SPair<String>[2]
pairs[1] = words
checksPassed :& numbers.First() + numbers.Second() = 42
checksPassed :& numbers.AppendFirst(23) = 42
checksPassed :& words.First() = "managed first" And words.Second() = "managed second"
checksPassed :& pairs[0].First() = "" And pairs[0].Second() = ""

Local transient:String
Local automaticCollectionsBefore:UInt = AutomaticCollectionCount()
For Local index:Int = 0 Until 600
	transient = pairs[1].AppendFirst(" rooted suffix")
Next
CollectObjects()
checksPassed :& pairs[1].First() = "managed first" And pairs[1].Second() = "managed second"
checksPassed :& transient = "managed first rooted suffix"
checksPassed :& AutomaticCollectionCount() > automaticCollectionsBefore
checksPassed :& InvalidReferenceCount() = 0

If checksPassed Then
	PutString("Generic scalar and Struct module checks passed")
Else
	PutString("Generic scalar or Struct module check failed")
End If
PutCharacter(10)

Local ledPin:UInt = DefaultLEDPin()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)

While True
	GPIOPut(ledPin, checksPassed)
	If checksPassed Then PutCharacter(46) Else PutCharacter(33)
	SleepMilliseconds(250)
Wend
