SuperStrict

Framework BRL.StandardIO
Import BRL.FileSystem
Import BRL.Path
Import BRL.Glob
Import Pico.Storage.LittleFS

If Not LittleFSIsMounted() Then
	Print "LittleFS mount failed: " + LittleFSLastError()
Else
	CreateDir("data/sub", True)
	Local output:TStream = OpenStream("data/sub/hello.txt", False, WRITE_MODE_APPEND)
	If output Then
		output.WriteLine("Persistent boot record")
		output.Close()
	End If

	Local input:TStream = ReadFile("file::data/sub/hello.txt")
	If input Then
		Print input.ReadLine()
		input.Close()
	End If

	For Local path:String = EachIn Glob("data/**/*.txt", EGlobOptions.GlobStar)
		Print path
	Next
	Print "LittleFS: " + LittleFSUsed() + " / " + LittleFSCapacity()
End If
