SuperStrict

Import BRL.RamStream
Import BRL.StandardIO
Import Pico.Board.Pico
Import Pico.Hardware.GPIO
Import Pico.System.Time

Incbin "incbin_payload.txt"

Local checksPassed:Int = True
Local failure:Int
Local payload:Byte Ptr = IncbinPtr("incbin_payload.txt")
Local payloadLength:Int = IncbinLen("incbin_payload.txt")
If payload = Null Then failure = 1
If Not failure And payloadLength <> 37 Then failure = 2
If Not failure And payload[0] <> Asc("B") Then failure = 3
If Not failure And payload[payloadLength - 1] <> 10 Then failure = 4

Local stream:TStream = ReadStream("incbin::incbin_payload.txt")
If Not failure And stream = Null Then failure = 5
If Not failure Then
	If stream.Size() <> payloadLength Then failure = 6
	If Not failure And stream.ReadLine() <> "BlitzMax Pico Incbin stream payload!" Then failure = 7
	If Not failure And Not stream.Eof() Then failure = 8
	stream.Seek(9)
	Local bytes:Byte[] = New Byte[4]
	If Not failure And stream.Read(bytes, bytes.Length) <> bytes.Length Then failure = 9
	If Not failure And (bytes[0] <> Asc("P") Or bytes[1] <> Asc("i") Or bytes[2] <> Asc("c") Or bytes[3] <> Asc("o")) Then failure = 10
	stream.Close()
End If
checksPassed = failure = 0

Local ledPin:UInt = DefaultLEDPin()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)

While True
	GPIOPut(ledPin, checksPassed)
	If checksPassed Then
		Print "IncbinPtr, IncbinLen and BRL.RamStream checks passed"
	Else
		ErrPrint "Incbin or RamStream check failed: " + failure
	End If
	SleepMilliseconds(1000)
Wend
