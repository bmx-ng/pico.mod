SuperStrict

Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.System.Time
Import Pico.Random

StandardIOInit()

Local generator:TRandom = CreateRandom("Pico")
Local names:String[] = GetRandomNames()
Local foundPico:Int
For Local name:String = EachIn names
	If name = "Pico" Then foundPico = True
Next

Local checksPassed:Int = generator And generator.GetName() = "Pico" And ..
	GetRandomName() = "Pico" And foundPico And Not generator.CanSaveState() And ..
	Not generator.SaveState() And Not RandomSaveState()

For Local index:Int = 0 Until 256
	Local floatValue:Float = generator.RndFloat()
	Local doubleValue:Double = generator.RndDouble()
	Local ranged:Double = generator.Rnd(-2.5, 7.25)
	Local intValue:Int = generator.RandomInt(-19, 23)
	Local reverseInt:Int = generator.RandomInt(23, -19)
	Local longValue:Long = generator.RandomLong(-5000000000:Long, 7000000000:Long)
	Local byteValue:Byte = generator.RandomByte(4, 17)
	Local shortValue:Short = generator.RandomShort(300, 900)
	Local uintValue:UInt = generator.RandomUInt(4000000000:UInt, 4000000100:UInt)
	Local ulongValue:ULong = generator.RandomULong(9000000000000000000:ULong, 9000000000000000100:ULong)
	Local longIntValue:LongInt = generator.RandomLongInt(-1000:LongInt, 1000:LongInt)
	Local ulongIntValue:ULongInt = generator.RandomULongInt(1000:ULongInt, 2000:ULongInt)
	Local sizeValue:Size_T = generator.RandomSizeT(10:Size_T, 20:Size_T)
	checksPassed :& floatValue >= 0.0 And floatValue < 1.0 And ..
		doubleValue >= 0.0 And doubleValue < 1.0 And ..
		ranged >= -2.5 And ranged < 7.25 And ..
		intValue >= -19 And intValue <= 23 And ..
		reverseInt >= -19 And reverseInt <= 23 And ..
		longValue >= -5000000000:Long And longValue <= 7000000000:Long And ..
		byteValue >= 4 And byteValue <= 17 And ..
		shortValue >= 300 And shortValue <= 900 And ..
		uintValue >= 4000000000:UInt And uintValue <= 4000000100:UInt And ..
		ulongValue >= 9000000000000000000:ULong And ulongValue <= 9000000000000000100:ULong And ..
		longIntValue >= -1000:LongInt And longIntValue <= 1000:LongInt And ..
		ulongIntValue >= 1000:ULongInt And ulongIntValue <= 2000:ULongInt And ..
		sizeValue >= 10:Size_T And sizeValue <= 20:Size_T
Next

If checksPassed Then
	PutString("Pico random checks passed")
Else
	PutString("Pico random check failed")
End If
PutCharacter(10)

Local ledPin:UInt = DefaultLEDPin()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)

While True
	GPIOPut(ledPin, checksPassed)
	If checksPassed Then PutCharacter(46) Else PutCharacter(33)
	SleepMilliseconds(250)
	GPIOPut(ledPin, False)
	SleepMilliseconds(250)
Wend
