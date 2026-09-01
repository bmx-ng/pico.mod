SuperStrict

Import BRL.StandardIO
Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.System.Time

Local checksPassed:Int = True
Local beforeMicros:ULong = MonotonicMicroseconds()
Local beforeMillis:ULong = MonotonicMilliseconds()
Local standardBefore:Int = MilliSecs()
SleepMilliseconds(5)
checksPassed :& MonotonicMicroseconds() >= beforeMicros + 5000:ULong
checksPassed :& MonotonicMilliseconds() >= beforeMillis + 5:ULong
checksPassed :& MilliSecs() - standardBefore >= 5
Local beforeShortSleep:ULong = MonotonicMicroseconds()
SleepMicroseconds(1000)
checksPassed :& MonotonicMicroseconds() >= beforeShortSleep + 1000:ULong

Local oneShot:Int = AlarmAfterMilliseconds(10)
checksPassed :& oneShot <> 0 And AlarmActive(oneShot)
Delay(15)
checksPassed :& Not AlarmActive(oneShot)
checksPassed :& PendingAlarmEvents(oneShot) = 1
checksPassed :& TakeAlarmEvents(oneShot) = 1
checksPassed :& PendingAlarmEvents(oneShot) = 0

Local microOneShot:Int = AlarmAfterMicroseconds(2000)
Delay(4)
checksPassed :& TakeAlarmEvents(microOneShot) = 1

Local repeating:Int = RepeatingAlarmMilliseconds(5)
checksPassed :& repeating <> 0 And AlarmActive(repeating)
Delay(24)
Local repeatCount:UInt = TakeAlarmEvents(repeating)
checksPassed :& repeatCount >= 3
checksPassed :& AlarmActive(repeating)
checksPassed :& CancelAlarm(repeating)
checksPassed :& Not AlarmActive(repeating)

Local microRepeating:Int = RepeatingAlarmMicroseconds(2000)
Delay(7)
checksPassed :& TakeAlarmEvents(microRepeating) >= 2
checksPassed :& CancelAlarm(microRepeating)

Local cancellable:Int = AlarmAfterMilliseconds(1000)
Local remaining:Int = RemainingAlarmMilliseconds(cancellable)
Local remainingMicros:Long = RemainingAlarmMicroseconds(cancellable)
checksPassed :& cancellable <> 0 And remaining >= 0 And remaining <= 1000
checksPassed :& remainingMicros >= 0 And remainingMicros <= 1000000
checksPassed :& CancelAlarm(cancellable)

Local ledPin:UInt = DefaultLEDPin()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)
GPIOPut(ledPin, checksPassed)

While True
	If checksPassed Then
		Print "Monotonic time and native alarm checks passed"
	Else
		Print "Monotonic time or native alarm check failed"
	End If
	Delay(1000)
Wend
