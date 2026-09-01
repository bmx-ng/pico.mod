SuperStrict

Import BRL.StandardIO
Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.Runtime.Memory

Local checksPassed:Int = True

Local cText:Byte[] = New Byte[5]
cText[0] = 80; cText[1] = 105; cText[2] = 99; cText[3] = 111
checksPassed :& String.FromCString(cText) = "Pico"
checksPassed :& String.FromBytes(cText, 4) = "Pico"

Local wideText:Short[] = New Short[5]
wideText[0] = 80; wideText[1] = 105; wideText[2] = 99; wideText[3] = 111
checksPassed :& String.FromShorts(wideText, 4) = "Pico"
checksPassed :& String.FromWString(wideText) = "Pico"

Local utf8Text:Byte[] = New Byte[7]
utf8Text[0] = 80; utf8Text[1] = 105; utf8Text[2] = 32
utf8Text[3] = $e2; utf8Text[4] = $82; utf8Text[5] = $ac
Local unicodeText:String = "Pi €"
checksPassed :& String.FromUTF8String(utf8Text) = unicodeText
checksPassed :& String.FromUTF8Bytes(utf8Text, 6) = unicodeText

Local cOutput:Byte Ptr = "Pico".ToCString()
checksPassed :& cOutput And cOutput[0] = 80 And cOutput[3] = 111 And cOutput[4] = 0
MemFree(cOutput)

Local wideOutput:Short Ptr = unicodeText.ToWString()
checksPassed :& wideOutput And wideOutput[0] = 80 And wideOutput[3] = $20ac And wideOutput[4] = 0
MemFree(wideOutput)

Local wideBuffer:Short[4]
Local wideLength:Size_T = 4
Local returnedWideBuffer:Short Ptr = "Pico".ToWStringBuffer(wideBuffer, wideLength)
checksPassed :& returnedWideBuffer <> Null And wideLength = 3 And wideBuffer[0] = 80 And wideBuffer[2] = 99 And wideBuffer[3] = 0

Local utf8Length:Size_T
Local utf8Output:Byte Ptr = unicodeText.ToUTF8String(utf8Length)
checksPassed :& utf8Output And utf8Length = 6 And utf8Output[3] = $e2 And utf8Output[5] = $ac And utf8Output[6] = 0
MemFree(utf8Output)

Local utf8Buffer:Byte[4]
Local utf8Capacity:Size_T = 4
Local returnedUTF8Buffer:Byte Ptr = "€X".ToUTF8StringBuffer(utf8Buffer, utf8Capacity)
checksPassed :& returnedUTF8Buffer <> Null And utf8Capacity = 3 And utf8Buffer[0] = $e2 And utf8Buffer[2] = $ac And utf8Buffer[3] = 0

Local utf32Text:UInt[] = New UInt[3]
utf32Text[0] = $1f642; utf32Text[1] = $20ac
Local supplementary:String = String.FromUTF32String(utf32Text)
checksPassed :& supplementary = "🙂€" And supplementary.Length = 3
checksPassed :& String.FromUTF32Bytes(utf32Text, 2) = supplementary
Local utf32Output:UInt Ptr = supplementary.ToUTF32String()
checksPassed :& utf32Output And utf32Output[0] = $1f642 And utf32Output[1] = $20ac And utf32Output[2] = 0
MemFree(utf32Output)
Local invalidUTF32:UInt[] = New UInt[1]
invalidUTF32[0] = $110000
checksPassed :& String.FromUTF32Bytes(invalidUTF32, 1) = Chr($fffd)
Local invalidSurrogateCaught:Int
Try
	Local invalidOutput:UInt Ptr = Chr($d800).ToUTF32String()
	If invalidOutput Then MemFree(invalidOutput)
Catch message:String
	invalidSurrogateCaught = message.Contains("Invalid UTF-16")
End Try
checksPassed :& invalidSurrogateCaught

Local hexBytes:Byte[] = New Byte[3]
hexBytes[1] = $ab; hexBytes[2] = $ff
checksPassed :& String.FromBytesAsHex(hexBytes, 3) = "00ABFF"
checksPassed :& String.FromBytesAsHex(hexBytes, 3, False) = "00abff"
Local decoded:Byte[3]
checksPassed :& "00abFF".ToBytesFromHex(decoded, 3) = 3
checksPassed :& decoded[0] = 0 And decoded[1] = $ab And decoded[2] = $ff
MemClear(decoded, 3)
checksPassed :& "xx00abFFyy".ToBytesFromHex(2, 6, decoded, 3) = 3
checksPassed :& decoded[0] = 0 And decoded[1] = $ab And decoded[2] = $ff

' Ensure managed conversion results survive collection and manual buffers do not leak.
Local retained:String = String.FromUTF8String(utf8Text)
For Local index:Int = 0 Until 500
	Local transient:String = String.FromBytesAsHex(hexBytes, 3, index & 1)
Next
CollectObjects()
checksPassed :& retained = unicodeText And InvalidReferenceCount() = 0 And ArenaFailureCount() = 0

Local ledPin:UInt = DefaultLEDPin()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)

While True
	GPIOPut(ledPin, checksPassed)
	If checksPassed Then
		Print "String encoding checks passed"
	Else
		ErrPrint "String encoding check failed"
	End If
	Delay(1000)
Wend
