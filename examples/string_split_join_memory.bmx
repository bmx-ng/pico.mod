SuperStrict

Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.Runtime.Memory

Local checksPassed:Int = True

Local separated:String[] = "alpha--beta----".Split("--")
checksPassed :& separated.Length = 4
checksPassed :& separated[0] = "alpha" And separated[1] = "beta"
checksPassed :& separated[2] = "" And separated[3] = ""
checksPassed :& "--".Join(separated) = "alpha--beta----"

Local whole:String[] = "unchanged".Split("/")
checksPassed :& whole.Length = 1 And whole[0] = "unchanged"

Local words:String[] = "  alpha~tbeta~n gamma  ".Split("")
checksPassed :& words.Length = 3
checksPassed :& words[0] = "alpha" And words[1] = "beta" And words[2] = "gamma"
checksPassed :& ",".Join(words) = "alpha,beta,gamma"

Local noWords:String[] = " ~t~n ".Split("")
checksPassed :& noWords.Length = 0 And ",".Join(noWords) = ""

Local manual:String[] = New String[4]
manual[0] = "Pico"
manual[1] = ""
manual[2] = "String"
manual[3] = "Join"
Local retained:String = ":".Join(manual)
checksPassed :& retained = "Pico::String:Join"

' Exercise repeated Array and String allocation, precise Array tracing, and reuse.
For Local index:Int = 0 Until 300
	Local dynamicSource:String = "left" + ",right"
	Local transient:String[] = dynamicSource.Split(",")
	Local joined:String = "|".Join(transient)
Next
CollectObjects()
checksPassed :& retained = "Pico::String:Join"
checksPassed :& separated[1] = "beta" And words[2] = "gamma"
checksPassed :& AutomaticCollectionCount() > 0 And InvalidReferenceCount() = 0
checksPassed :& ArenaFailureCount() = 0 And StringFailureCount() = 0 And ArrayFailureCount() = 0

Local ledPin:UInt = DefaultLEDPin()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)
StandardIOInit()

While True
	GPIOPut(ledPin, checksPassed)
	If checksPassed Then
		PutString("String Split/Join checks passed~n")
	Else
		PutString("String Split/Join check failed~n")
	End If
	Delay(1000)
Wend
