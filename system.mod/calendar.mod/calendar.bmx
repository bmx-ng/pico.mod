' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: UTC calendar clock and alarm services for Raspberry Pi Pico targets.
about: RP2040 uses its RTC and RP2350 uses its always-on Powman timer. The clock
does not retain time across a complete loss of power and Pico supplies no timezone database.
End Rem
Module Pico.System.Calendar
?pico

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"

Import Pub.Time

Extern "C"
	Rem
	bbdoc: Starts the calendar clock at the supplied date and time.
	returns: True on success.
	End Rem
	Function CalendarStart:Int(dateTime:SDateTime Var) = "bmx_pico_calendar_start"

	Rem
	bbdoc: Stops the calendar clock.
	End Rem
	Function CalendarStop() = "bmx_pico_calendar_stop"

	Rem
	bbdoc: Changes the running calendar clock to the supplied date and time.
	returns: True on success.
	End Rem
	Function CalendarSet:Int(dateTime:SDateTime Var) = "bmx_pico_calendar_set"

	Rem
	bbdoc: Reads the calendar clock as UTC.
	returns: True when the clock is running and was read successfully.
	End Rem
	Function CalendarGet:Int(dateTime:SDateTime Var) = "bmx_pico_calendar_get"

	Function CalendarIsRunning:Int() = "bmx_pico_calendar_is_running"
	Function CalendarResolutionNanoseconds:ULong() = "bmx_pico_calendar_resolution_nanoseconds"

	Rem
	bbdoc: Configures the single hardware calendar alarm.
	about: The interrupt is deferred: poll #PendingCalendarAlarmEvents or consume it
	with #TakeCalendarAlarmEvents from normal program code.
	End Rem
	Function CalendarSetAlarm:Int(dateTime:SDateTime Var, wakeFromLowPower:Int = False) = "bmx_pico_calendar_set_alarm"
	Function CalendarDisableAlarm() = "bmx_pico_calendar_disable_alarm"
	Function PendingCalendarAlarmEvents:UInt() = "bmx_pico_calendar_pending_alarm_events"
	Function TakeCalendarAlarmEvents:UInt() = "bmx_pico_calendar_take_alarm_events"
End Extern
?
