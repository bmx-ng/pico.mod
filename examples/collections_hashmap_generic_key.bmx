SuperStrict

Framework BRL.StandardIO
Import Collections.HashMap
Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.System.Time

Global hashCalls:UInt
Global equalsCalls:UInt

Type TKey<T>
	Field value:T

	Method New(value:T)
		Self.value = value
	End Method

	Method HashCode:UInt() Override
		hashCalls :+ 1
		Return 7
	End Method

	Method Equals:Int(other:Object) Override
		equalsCalls :+ 1
		Local key:TKey<T> = TKey<T>(other)
		Return key And value = key.value
	End Method
End Type

Local values:THashMap<TKey<Int>, String> = New THashMap<TKey<Int>, String>
For Local index:Int = 0 Until 12
	values.Add(New TKey<Int>(index), "value-" + index)
Next

Local found:String
Local checksPassed:Int = values.Count() = 12 And ..
	values.TryGetValue(New TKey<Int>(7), found) And found = "value-7" And ..
	values.ContainsKey(New TKey<Int>(11)) And ..
	Not values.ContainsKey(New TKey<Int>(20)) And ..
	hashCalls > 0 And equalsCalls > 0

If checksPassed Then
	Print "Generic HashMap checks passed"
Else
	Print "Generic HashMap check failed"
End If

Local ledPin:UInt = DefaultLEDPin()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)

While True
	GPIOPut(ledPin, checksPassed)
	Delay 250
	GPIOPut(ledPin, False)
	Delay 250
Wend
