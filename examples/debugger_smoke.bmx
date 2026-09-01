SuperStrict

Framework BRL.StandardIO

Enum EDebugMode
	Waiting
	Running
End Enum

Struct SDebugPoint
	Field x:Int
	Field y:Float

	Method New(x:Int, y:Float)
		Self.x = x
		Self.y = y
	End Method
End Struct

Type TDebugBase
	Field baseValue:Int
End Type

Type TDebugSample Extends TDebugBase
	Field name:String
	Field values:Int[]
End Type

Function InspectWithDebugger(sample:TDebugSample, point:SDebugPoint)
	Local counter:Int = 42
	Local message:String = "BlitzMax Pico debugger"
	Local numbers:Int[] = New Int[3]
	numbers[0] = 3
	numbers[1] = 5
	numbers[2] = 8
	Local mode:EDebugMode = EDebugMode.Running
	Local debugConditional:Int
	?debug
	debugConditional = 1234
	?

	Print "Stopping at DebugStop when a debugger is attached..."
	DebugStop

	' Keep every fixture value live after DebugStop so GDB can inspect it.
	Print message + ": " + counter + ", " + numbers[2] + ", " + sample.name + ", " + point.x + ", " + mode.Ordinal() + ", " + debugConditional
End Function

Local sample:TDebugSample = New TDebugSample
sample.baseValue = 17
sample.name = "visible object"
sample.values = New Int[3]
sample.values[0] = 10
sample.values[1] = 20
sample.values[2] = 30
Local point:SDebugPoint = New SDebugPoint(7, 2.5)

While True
	InspectWithDebugger(sample, point)
	Delay 1000
Wend
