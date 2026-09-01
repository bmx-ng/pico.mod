SuperStrict

Import BRL.ByteArrayStream
Import BRL.StandardIO
Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.System.Time

Global IntrinsicArgumentEvaluations:Int

Function CountedIntrinsicArgument:Int(value:Int)
	IntrinsicArgumentEvaluations :+ 1
	Return value
End Function

Local checksPassed:Int = True
Local stream:TByteArrayStream = New TByteArrayStream(New Byte[0], False, False)

' BRL.Blitz scalar intrinsics retain their ordinary overloads on Pico.
Local intValue:Int = Min(7, 3) + Max(2, 5)
Local longLow:Long = 3
Local longHigh:Long = 7
Local floatLow:Float = 3.0
Local floatHigh:Float = 7.0
Local doubleLow:Double = 3.0
Local doubleHigh:Double = 7.0
Local byteLow:Byte = 3
Local byteHigh:Byte = 7
Local shortLow:Short = 3
Local shortHigh:Short = 7
Local uintLow:UInt = 3
Local uintHigh:UInt = 7
Local ulongLow:ULong = 3
Local ulongHigh:ULong = 7
Local sizeLow:Size_T = 3
Local sizeHigh:Size_T = 7
Local longIntLow:LongInt = 3
Local longIntHigh:LongInt = 7
Local ulongIntLow:ULongInt = 3
Local ulongIntHigh:ULongInt = 7
checksPassed :& intValue = 8
checksPassed :& Min(longHigh, longLow) = longLow And Max(longLow, longHigh) = longHigh
checksPassed :& Min(floatHigh, floatLow) = floatLow And Max(floatLow, floatHigh) = floatHigh
checksPassed :& Min(doubleHigh, doubleLow) = doubleLow And Max(doubleLow, doubleHigh) = doubleHigh
checksPassed :& Min(byteHigh, byteLow) = byteLow And Max(byteLow, byteHigh) = byteHigh
checksPassed :& Min(shortHigh, shortLow) = shortLow And Max(shortLow, shortHigh) = shortHigh
checksPassed :& Min(uintHigh, uintLow) = uintLow And Max(uintLow, uintHigh) = uintHigh
checksPassed :& Min(ulongHigh, ulongLow) = ulongLow And Max(ulongLow, ulongHigh) = ulongHigh
checksPassed :& Min(sizeHigh, sizeLow) = sizeLow And Max(sizeLow, sizeHigh) = sizeHigh
checksPassed :& Min(longIntHigh, longIntLow) = longIntLow And Max(longIntLow, longIntHigh) = longIntHigh
checksPassed :& Min(ulongIntHigh, ulongIntLow) = ulongIntLow And Max(ulongIntLow, ulongIntHigh) = ulongIntHigh
checksPassed :& Min(CountedIntrinsicArgument(7), CountedIntrinsicArgument(3)) = 3
checksPassed :& IntrinsicArgumentEvaluations = 2
checksPassed :& Abs(-7) = 7 And Abs(-7:Long) = 7:Long
checksPassed :& Abs(-7.5:Float) = 7.5:Float And Abs(-7.5:Double) = 7.5:Double
checksPassed :& Sgn(-7) = -1 And Sgn(0) = 0 And Sgn(7) = 1
checksPassed :& Sgn(-7:Long) = -1:Long And Sgn(7:Long) = 1:Long
checksPassed :& Sgn(-7.5:Float) = -1.0:Float And Sgn(7.5:Double) = 1.0:Double

' Typed values and UTF-8 text share the ordinary TStream contract.
stream.WriteInt($12345678)
stream.WriteLong($1020304050607080:Long)
stream.WriteLine("Pico stream £ ✓", True)
checksPassed :& stream.Size() > 16

stream.Seek(0)
checksPassed :& stream.ReadInt() = $12345678
checksPassed :& stream.ReadLong() = $1020304050607080:Long
checksPassed :& stream.ReadLine(True) = "Pico stream £ ✓"
checksPassed :& stream.Eof()

' Passing a stream as a URL uses a non-owning wrapper, as on desktop BlitzMax.
stream.Seek(0)
Local loaded:Byte[] = LoadByteArray(stream)
checksPassed :& loaded.Length = stream.Size()
checksPassed :& stream.Pos() = stream.Size()

stream.SetSize(0)
stream.Seek(0)
SaveString("UTF-8 round trip £ ✓", stream, True)
stream.Seek(0)
checksPassed :& LoadString(stream, True) = "UTF-8 round trip £ ✓"

Local ledPin:UInt = DefaultLEDPin()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)

While True
	GPIOPut(ledPin, checksPassed)
	If checksPassed Then
		Print "BRL.Stream and BRL.StandardIO checks passed £ ✓"
	Else
		ErrPrint "BRL.Stream check failed"
	End If
	SleepMilliseconds(1000)
Wend
