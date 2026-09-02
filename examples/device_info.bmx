SuperStrict

Import BRL.StandardIO
Import Pico.System.Device

Local identifier:String = UniqueBoardID()
Local identifierBytes:Byte[] = UniqueBoardIDBytes()

If identifier.length = 16 And identifierBytes.length = 8 Then
	Print "Pico device identity checks passed"
Else
	Print "Pico device identity checks failed"
End If

While True
	Print "Unique board ID: " + identifier
	Print "Unique board ID bytes: " + identifierBytes.length
	Print "BOOTSEL pressed: " + BootselButtonPressed()
	Delay 2000
Wend
