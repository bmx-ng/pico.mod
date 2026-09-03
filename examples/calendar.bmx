SuperStrict

Framework BRL.StandardIO
Import Pub.Time
Import Pico.System.Calendar

Local startTime:SDateTime = New SDateTime(2026, 9, 3, 12, 34, 56, 0, True)
Local started:Int = CalendarStart(startTime)
Local checksPassed:Int = started

Local current:SDateTime
Local running:Int = CalendarIsRunning()
Local gotCurrent:Int = CalendarGet(current)
checksPassed :& running And gotCurrent
checksPassed :& current.year = 2026 And current.month = 9 And current.day = 3
Local epoch:Long = current.ToEpochSecs()
Local unixTime:ULong = CurrentUnixTime()
Local dateText:String = CurrentDate("%Y-%m-%d")
Local timeText:String = CurrentTime()
checksPassed :& epoch >= 1788438896:Long
checksPassed :& unixTime >= 1788438896000:ULong
checksPassed :& dateText = "2026-09-03"
checksPassed :& timeText.StartsWith("12:34:")

Local localTime:SDateTime = New SDateTime(2026, 9, 3, 14, 34, 56, 250, False, 60, 1)
Local utcTime:SDateTime = localTime.ToUtc()
Local utcText:String = utcTime.ToIso8601(True)
checksPassed :& utcText = "2026-09-03T12:34:56.250Z"

Local alarmTime:SDateTime = New SDateTime(2026, 9, 3, 12, 34, 58, 0, True)
Local alarmSet:Int = CalendarSetAlarm(alarmTime)
checksPassed :& alarmSet
Delay 2200
Local pending:UInt = PendingCalendarAlarmEvents()
Local taken:UInt = TakeCalendarAlarmEvents()
checksPassed :& pending = 1
checksPassed :& taken = 1
checksPassed :& PendingCalendarAlarmEvents() = 0

Local result:String
If checksPassed Then
	result = "Pico calendar checks passed: " + CurrentDate() + " " + CurrentTime()
Else
	result = "Pico calendar check failed: start=" + started + " running=" + running + ..
		" get=" + gotCurrent + " epoch=" + epoch + " unix=" + unixTime + ..
		" date=" + dateText + " time=" + timeText + " utc=" + utcText + ..
		" alarm=" + alarmSet + " pending=" + pending + " taken=" + taken
End If

While True
	Print result
	Delay 1000
Wend
