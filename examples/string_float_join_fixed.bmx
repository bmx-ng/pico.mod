SuperStrict

Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO

Local floats:Float[] = New Float[2]
floats[0] = 1.5:Float
floats[1] = -2.25:Float
Local doubles:Double[] = New Double[2]
doubles[0] = 3.125:Double
doubles[1] = -0.5:Double
Local checksPassed:Int = ",".Join(floats, True) = "1.500000000,-2.250000000" And ..
	"|".Join(doubles, True) = "3.12500000000000000|-0.50000000000000000"

Local ledPin:UInt = DefaultLEDPin()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)
StandardIOInit()

While True
	GPIOPut(ledPin, checksPassed)
	If checksPassed Then
		PutString("Fixed floating String Join checks passed~n")
	Else
		PutString("Fixed floating String Join check failed~n")
	End If
	Delay(1000)
Wend
