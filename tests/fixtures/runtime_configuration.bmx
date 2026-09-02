SuperStrict

Framework BRL.StandardIO
Import Pico.Runtime.Memory
Import Pico.Board

Print "Managed heap: " + ArenaCapacity()
Local defaultLED:UInt = DefaultLEDPin()

While True
	Delay 1000
Wend
