SuperStrict

Framework BRL.StandardIO
Import Pub.Time
Import Pico.System.Calendar

Global passed:Int
Global failed:Int
Global failureDetails:String

Function Check(label:String, condition:Int)
	If condition Then
		passed :+ 1
	Else
		failed :+ 1
		failureDetails :+ label + "; "
		Print "FAIL: " + label
	End If
End Function

Function CheckDate(label:String, value:SDateTime, year:Int, month:Int, day:Int, hour:Int, minute:Int, second:Int)
	Check(label, value.year = year And value.month = month And value.day = day And ..
		value.hour = hour And value.minute = minute And value.second = second)
End Function

' Gregorian/Unix boundary and round-trip vectors.
Local epoch0:SDateTime = SDateTime.FromEpoch(0)
CheckDate("Unix epoch", epoch0, 1970, 1, 1, 0, 0, 0)
Check("Unix epoch weekday", epoch0.DayOfWeek() = EWeekday.Thursday)
Check("Unix epoch round trip", epoch0.ToEpochSecs() = 0)

Local beforeEpoch:SDateTime = SDateTime.FromEpoch(-1)
CheckDate("Before Unix epoch", beforeEpoch, 1969, 12, 31, 23, 59, 59)
Check("Negative epoch round trip", beforeEpoch.ToEpochSecs() = -1)

Local leap2000:SDateTime = SDateTime.FromEpoch(951782400)
CheckDate("Leap day 2000", leap2000, 2000, 2, 29, 0, 0, 0)
Check("Leap day round trip", leap2000.ToEpochSecs() = 951782400)

Local boundary2038:SDateTime = SDateTime.FromEpoch(2147483647)
CheckDate("2038 boundary", boundary2038, 2038, 1, 19, 3, 14, 7)
Check("2038 round trip", boundary2038.ToEpochSecs() = 2147483647)

Local year2100:SDateTime = SDateTime.FromEpoch(4102444800:Long)
CheckDate("Non-leap century", year2100, 2100, 1, 1, 0, 0, 0)
Check("64-bit epoch round trip", year2100.ToEpochSecs() = 4102444800:Long)

Local finalDate:SDateTime = SDateTime.FromEpoch(253402300799:Long)
CheckDate("Final supported date", finalDate, 9999, 12, 31, 23, 59, 59)
Check("Final date round trip", finalDate.ToEpochSecs() = 253402300799:Long)

Local fractional:SDateTime = SDateTime.FromEpoch(0, 123456789)
Check("Fractional epoch milliseconds", fractional.millisecond = 123)

' UTC offsets, daylight saving, ISO output and calendar validation.
Local localEast:SDateTime = New SDateTime(2026, 9, 3, 14, 34, 56, 250, False, 60, 1)
Check("East/DST epoch conversion", localEast.ToEpochSecs() = 1788438896:Long)
Check("East/DST UTC conversion", localEast.ToUtc().ToIso8601(True) = "2026-09-03T12:34:56.250Z")
Check("Local ISO offset", localEast.ToIso8601(True) = "2026-09-03T14:34:56.250+02:00")

Local localWest:SDateTime = New SDateTime(1970, 1, 1, 0, 0, 0, 0, False, -60, 0)
Check("West offset conversion", localWest.ToEpochSecs() = 3600)
Check("West offset UTC conversion", localWest.ToUtc().ToIso8601() = "1970-01-01T01:00:00Z")

Check("Leap year 2000 accepted", New SDateTime(2000, 2, 29, 0, 0, 0).ToEpochSecs() = 951782400)
Check("Century 1900 rejected", New SDateTime(1900, 2, 29, 0, 0, 0).ToEpochSecs() = -1)
Check("Invalid month rejected", New SDateTime(2026, 13, 1, 0, 0, 0).ToEpochSecs() = -1)
Check("Invalid day rejected", New SDateTime(2026, 4, 31, 0, 0, 0).ToEpochSecs() = -1)
Check("Invalid hour rejected", New SDateTime(2026, 1, 1, 24, 0, 0).ToEpochSecs() = -1)
Check("Invalid second rejected", New SDateTime(2026, 1, 1, 0, 0, 60).ToEpochSecs() = -1)
Check("DayOfWeek Monday", New SDateTime(2024, 1, 1, 0, 0, 0).DayOfWeek() = EWeekday.Monday)
Check("DayOfWeek leap day", New SDateTime(2024, 2, 29, 0, 0, 0).DayOfWeek() = EWeekday.Thursday)

' Hardware lifecycle and standard Pub.Time names.
CalendarStop()
Check("Calendar initially stopped", Not CalendarIsRunning())
Local stoppedTime:SDateTime
Check("Stopped calendar read rejected", Not CalendarGet(stoppedTime))
Local invalidStart:SDateTime = New SDateTime(2025, 2, 29, 0, 0, 0)
Check("Invalid calendar start rejected", Not CalendarStart(invalidStart))

Local clockStart:SDateTime = New SDateTime(2024, 2, 29, 23, 5, 7, 0, True)
Check("Calendar start", CalendarStart(clockStart))
Check("Calendar running", CalendarIsRunning())
Local clockNow:SDateTime
Check("Calendar get", CalendarGet(clockNow))
Check("Calendar date", clockNow.year = 2024 And clockNow.month = 2 And clockNow.day = 29)
Check("CurrentUnixTime", CurrentUnixTime() >= 1709247907000:ULong)
Check("CurrentDate default", CurrentDate() = "29 Feb 2024")
Check("CurrentTime", CurrentTime().StartsWith("23:05:"))
Check("All documented format tokens", ..
	CurrentDate("%a|%A|%b|%B|%d|%H|%I|%j|%m|%M|%p|%U|%w|%W|%x|%y|%Y|%Z|%%") = ..
	"Thu|Thursday|Feb|February|29|23|11|060|02|05|PM|08|4|09|02/29/24|24|2024|UTC|%")
Local localeText:String = CurrentDate("%c")
Check("Locale date/time token", localeText.StartsWith("Thu Feb 29 23:05:") And localeText.EndsWith(" 2024"))
Check("Unsupported format rejected", CurrentDate("%Q") = "")
Check("Calendar resolution", CalendarResolutionNanoseconds() >= 1000000:ULong And ..
	CalendarResolutionNanoseconds() <= 1000000000:ULong)

' Legacy low-level entry points used by compatible standard modules.
Local rawTime:Int
Check("time_ result", time_(Varptr rawTime) = rawTime And rawTime > 0)
Local brokenDown:Byte Ptr = localtime_(Varptr rawTime)
Check("localtime_ result", brokenDown <> Null)
Local formatted:Byte[64]
Check("strftime_ result", strftime_(formatted, formatted.length, "%Y-%m-%d", brokenDown) = 10)
Check("strftime_ value", String.FromCString(formatted) = "2024-02-29")
Local monotonicBefore:STimeSpec
Local monotonicAfter:STimeSpec
Check("clock_gettime_ before", clock_gettime_(1, monotonicBefore) = 0)
Delay 2
Check("clock_gettime_ after", clock_gettime_(1, monotonicAfter) = 0)
Check("Monotonic time progresses", monotonicAfter.tv_sec > monotonicBefore.tv_sec Or ..
	(monotonicAfter.tv_sec = monotonicBefore.tv_sec And monotonicAfter.tv_nsec > monotonicBefore.tv_nsec))

Local changed:SDateTime = New SDateTime(2038, 1, 19, 3, 14, 7, 0, True)
Check("Calendar set", CalendarSet(changed))
Check("Calendar get after set", CalendarGet(clockNow))
Check("Calendar set value", clockNow.year = 2038 And clockNow.month = 1 And clockNow.day = 19)

' One-shot and cancelled alarms.
Local alarmBase:SDateTime = New SDateTime(2026, 9, 3, 12, 34, 10, 0, True)
Check("Alarm clock set", CalendarSet(alarmBase))
Local alarmTime:SDateTime = New SDateTime(2026, 9, 3, 12, 34, 12, 0, True)
Check("Alarm set", CalendarSetAlarm(alarmTime))
Delay 2200
Check("One-shot alarm pending", PendingCalendarAlarmEvents() = 1)
Check("One-shot alarm consumed", TakeCalendarAlarmEvents() = 1)
Check("Alarm empty after consume", PendingCalendarAlarmEvents() = 0)

Local cancelBase:SDateTime = New SDateTime(2026, 9, 3, 12, 35, 20, 0, True)
Check("Cancelled alarm clock set", CalendarSet(cancelBase))
Local cancelledAlarm:SDateTime = New SDateTime(2026, 9, 3, 12, 35, 22, 0, True)
Check("Cancelled alarm set", CalendarSetAlarm(cancelledAlarm))
CalendarDisableAlarm()
Delay 2200
Check("Cancelled alarm silent", PendingCalendarAlarmEvents() = 0)

CalendarStop()
Check("Calendar stop", Not CalendarIsRunning())
Check("Stopped CurrentUnixTime", CurrentUnixTime() = 0)
Check("Stopped CurrentDate", CurrentDate() = "")

Local result:String
If failed Then
	result = "Pico calendar unit tests failed: " + failed + " failed, " + passed + " passed; " + failureDetails
Else
	result = "Pico calendar unit tests passed: " + passed + " checks"
End If

While True
	Print result
	Delay 1000
Wend
