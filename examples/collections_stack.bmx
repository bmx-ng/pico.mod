SuperStrict

Framework BRL.StandardIO
Import Collections.Stack
Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.System.Time

Local stack:TStack<String> = New TStack<String>(2)
stack.Push("first")
stack.Push("second")
stack.Push("third")

Local collection:ICollection<String> = stack
Local order:String
For Local value:String = EachIn stack
	order :+ value + ","
Next

Local values:String[] = stack.ToArray()
Local top:String
Local checksPassed:Int = collection.Count() = 3 And stack.Contains("second") And ..
	stack.TryPeek(top) And top = "third" And ..
	order = "third,second,first," And ..
	values.length = 3 And values[0] = "third" And values[2] = "first" And ..
	stack.Pop() = "third" And stack.Count() = 2

If checksPassed Then
	Print "Collections.Stack generic Type checks passed"
Else
	Print "Collections.Stack generic Type check failed"
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
