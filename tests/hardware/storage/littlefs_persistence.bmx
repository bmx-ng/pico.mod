SuperStrict

Framework BRL.StandardIO
Import BRL.FileSystem
Import BRL.Glob
Import Pico.Storage.LittleFS

' Give USB serial time to enumerate before producing the one-shot details.
Delay 2000

If Not LittleFSIsMounted() Then
	Print "FAIL mount: " + LittleFSLastError()
Else
	If Not CreateDir("storage-test", True) Then
		Print "FAIL mkdir: " + LittleFSLastError()
	Else
		Local output:TStream = OpenStream("storage-test/boots.txt", False, WRITE_MODE_APPEND)
		If Not output Then
			Print "FAIL open-write: " + LittleFSLastError()
		Else
			output.WriteLine("Persistent boot record")
			output.Close()

			Local input:TStream = ReadFile("file::storage-test/boots.txt")
			If Not input Then
				Print "FAIL open-read: " + LittleFSLastError()
			Else
				Local records:Int
				While Not input.Eof()
					If input.ReadLine() = "Persistent boot record" Then records :+ 1
				Wend
				input.Close()

				SetFileTime("storage-test/boots.txt", 1234567890, FILETIME_MODIFIED)
				SetFileTime("storage-test/boots.txt", 1234567800, FILETIME_CREATED)
				SetFileTime("storage-test/boots.txt", 1234567000, FILETIME_ACCESSED)
				Local info:SFileStat
				Local metadataOk:Int = FileStat("storage-test/boots.txt", info) And ..
					info.modifiedTime = 1234567890 And info.creationTime = 1234567800 And ..
					info.accessTime = 1234567000
				Local matches:String[] = Glob("storage-test/**/*.txt", EGlobOptions.GlobStar)
				Print "PASS records=" + records + ", matches=" + matches.length + ..
					", metadata=" + metadataOk + ", used=" + LittleFSUsed() + ..
					", capacity=" + LittleFSCapacity()
			End If
		End If
	End If
End If

While True
	Delay 2000
	Local heartbeatInfo:SFileStat
	FileStat("storage-test/boots.txt", heartbeatInfo)
	Print "LittleFS alive; size=" + heartbeatInfo.size + ..
		", modified=" + heartbeatInfo.modifiedTime + ", created=" + heartbeatInfo.creationTime + ..
		", accessed=" + heartbeatInfo.accessTime
Wend
