SuperStrict

Framework BRL.EventQueue
Import Pico.Hardware.UART
Import Pico.IO.BufferedUART

UARTInit(UARTController0, 115200)
Local stream:TBufferedUARTStream = OpenBufferedUARTStream(UARTController0, 64, 64)
If stream
	Local available:UInt = stream.Available()
	Local writable:UInt = stream.WriteAvailable()
	Local dropped:UInt = stream.DroppedBytes()
	stream.Close()
End If
