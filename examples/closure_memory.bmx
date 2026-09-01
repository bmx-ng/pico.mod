SuperStrict

Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.Runtime.Memory
Import Pico.System.Time

Global boxDeletes:Int

Type TBox
	Field label:String
	Field samples:Int[]

	Method New(value:Int)
		label = "box" + Chr(33)
		samples = New Int[2]
		samples[0] = value
	End Method

	Method Delete()
		boxDeletes :+ 1
	End Method

	Method Reader:Closure<Int()>()
		Return Function()
			Return Self.samples[0]
		End Function
	End Method
End Type

Type TClosureHolder
	Field callback:Closure<Int()>
	Field transform:Int(value:Int)
End Type

Function DoubleValue:Int(value:Int)
	Return value * 2
End Function

Function MakeCounter:Closure<Int(delta:Int)>(initial:Int)
	Local count:Int = initial
	Local box:TBox = New TBox(3)
	Return Function(delta:Int)
		count :+ delta
		box.samples[0] :+ 1
		Return count + box.samples[0] + box.label.length
	End Function
End Function

Function MakeFactory:Closure<Closure<Int()>()>(initial:Int)
	Local shared:Int = initial
	Return Function()
		Local inner:Int = 10
		Return Function()
			shared :+ 1
			inner :+ 2
			Return shared + inner
		End Function
	End Function
End Function

Function MakeTransient:Closure<Int()>(value:Int)
	Local box:TBox = New TBox(value)
	Return Function()
		Return box.samples[0]
	End Function
End Function

StandardIOInit()

Local transform:Int(value:Int) = DoubleValue
Local missing:Int(value:Int)
Local addOne:Closure<Int(value:Int)> = Function(value:Int)
	Return value + 1
End Function
Local counter:Closure<Int(delta:Int)> = MakeCounter(10)
Local factory:Closure<Closure<Int()>()> = MakeFactory(5)
Local nested:Closure<Int()> = factory()
Local callbacks:Closure<Int()>[] = New Closure<Int()>[2]
callbacks[0] = nested
Local holder:TClosureHolder = New TClosureHolder
holder.callback = nested
holder.transform = transform
Local owned:TBox = New TBox(7)
Local reader:Closure<Int()> = owned.Reader()
owned = Null

Local checksPassed:Int = transform(21) = 42 And Not missing And addOne(41) = 42 And ..
	counter(2) = 20 And counter(3) = 24 And callbacks[0]() = 18 And nested() = 21

Local transient:Closure<Int()>
For Local index:Int = 0 Until 700
	transient = MakeTransient(index)
Next
CollectObjects()
CollectObjects()
ReachabilityAudit()
checksPassed :& AutomaticCollectionCount() > 0 And InvalidReferenceCount() = 0 And ..
	ObjectFailureCount() = 0 And ArrayFailureCount() = 0 And boxDeletes > 0 And ..
	transient() = 699 And counter(1) = 26 And callbacks[0]() = 24 And ..
	holder.callback() = 27 And holder.transform(21) = 42 And reader() = 7

If checksPassed Then
	PutString("Closure checks passed")
Else
	PutString("Closure check failed")
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
