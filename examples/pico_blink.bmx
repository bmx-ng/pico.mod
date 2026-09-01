SuperStrict

Import Pico.Board.Pico
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.System.Time

Global Tick:UInt

Function EmitTick()
	PutCharacter(48 + Int(Tick Mod 10))
	PutCharacter(10)
End Function

Local ledPin:UInt = DefaultLEDPin()
StandardIOInit()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)

While True
	GPIOPut(ledPin, Int(Tick & 1))
	EmitTick()
	Tick :+ 1
	SleepMilliseconds(500)
Wend
