SuperStrict

Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.Runtime.Memory

Local checksPassed:Int = True

Local ints:Int[] = "-2147483648, 0,2147483647".SplitInts(",")
checksPassed :& ints.Length = 3
checksPassed :& ints[0] = $80000000 And ints[1] = 0 And ints[2] = $7fffffff
checksPassed :& "|".Join(ints) = "-2147483648|0|2147483647"

Local bytes:Byte[] = "0::255".SplitBytes("::")
checksPassed :& bytes.Length = 2 And bytes[0] = 0 And bytes[1] = 255
checksPassed :& ",".Join(bytes) = "0,255"

Local shorts:Short[] = "1,,65535".SplitShorts(",")
checksPassed :& shorts.Length = 3 And shorts[0] = 1 And shorts[1] = 0 And shorts[2] = 65535
checksPassed :& ":".Join(shorts) = "1:0:65535"

Local uints:UInt[] = "0,$ffffffff".SplitUInts(",")
checksPassed :& uints.Length = 2 And uints[0] = 0 And uints[1] = $ffffffff:UInt
checksPassed :& ",".Join(uints) = "0,4294967295"

Local longs:Long[] = "-9223372036854775808/9223372036854775807".SplitLongs("/")
checksPassed :& longs.Length = 2
checksPassed :& longs[0] = $8000000000000000:Long And longs[1] = $7fffffffffffffff:Long
checksPassed :& ",".Join(longs) = "-9223372036854775808,9223372036854775807"

Local ulongs:ULong[] = "0/18446744073709551615".SplitULongs("/")
checksPassed :& ulongs.Length = 2 And ulongs[0] = 0 And ulongs[1] = $ffffffffffffffff:ULong
checksPassed :& ",".Join(ulongs) = "0,18446744073709551615"

Local sizes:Size_T[] = "1;4294967295".SplitSizeTs(";")
checksPassed :& sizes.Length = 2 And sizes[0] = 1 And sizes[1] = $ffffffff:Size_T
checksPassed :& ",".Join(sizes) = "1,4294967295"

Local longInts:LongInt[] = "-2147483648;2147483647".SplitLongInts(";")
checksPassed :& longInts.Length = 2
checksPassed :& ",".Join(longInts) = "-2147483648,2147483647"

Local ulongInts:ULongInt[] = "0;4294967295".SplitULongInts(";")
checksPassed :& ulongInts.Length = 2
checksPassed :& ",".Join(ulongInts) = "0,4294967295"

' Empty separator parses one complete value. Trailing junk, empty tokens and empty input follow desktop rules.
Local whole:Int[] = "  $2a  ".SplitInts("")
Local invalid:Int[] = "12x,7,, 9 ".SplitInts(",")
Local empty:Int[] = "".SplitInts(",")
checksPassed :& whole.Length = 1 And whole[0] = 42
checksPassed :& invalid.Length = 4 And invalid[0] = 0 And invalid[1] = 7 And invalid[2] = 0 And invalid[3] = 9
checksPassed :& empty.Length = 0 And ",".Join(empty) = ""

' Exercise allocation pressure while retaining both an Array and a joined String.
Local retainedInts:Int[] = "4,8,15,16,23,42".SplitInts(",")
Local retained:String = ":".Join(retainedInts)
For Local index:Int = 0 Until 500
	Local transient:Int[] = "-12,0,345".SplitInts(",")
	Local joined:String = "|".Join(transient)
Next
CollectObjects()
checksPassed :& retained = "4:8:15:16:23:42" And retainedInts[5] = 42
checksPassed :& AutomaticCollectionCount() > 0 And InvalidReferenceCount() = 0
checksPassed :& ArenaFailureCount() = 0 And StringFailureCount() = 0 And ArrayFailureCount() = 0

Local ledPin:UInt = DefaultLEDPin()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)
StandardIOInit()

While True
	GPIOPut(ledPin, checksPassed)
	If checksPassed Then
		PutString("Integer String Split/Join checks passed~n")
	Else
		PutString("Integer String Split/Join check failed~n")
	End If
	Delay(1000)
Wend
