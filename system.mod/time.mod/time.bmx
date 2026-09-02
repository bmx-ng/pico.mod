' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Monotonic time, deadlines, and sleep operations for Pico targets.
End Rem
Module Pico.System.Time
?pico

ModuleInfo "Version: 0.2"
ModuleInfo "License: zlib/libpng"

Extern "C"
	Rem
	bbdoc: Returns the current system-clock frequency in hertz.
	End Rem
	Function SystemClockFrequency:UInt() = "bmx_pico_system_clock_hz"

	Function MonotonicMicroseconds:ULong() = "bmx_pico_time_microseconds"
	Function MonotonicMilliseconds:ULong() = "bmx_pico_time_milliseconds"
	Function SleepMilliseconds(milliseconds:UInt) = "bmx_pico_sleep_ms"
	Function SleepMicroseconds(microseconds:ULong) = "bmx_pico_sleep_us"

	Rem
	bbdoc: Creates a one-shot alarm and returns its generation-checked handle.
	about: A zero result means that the duration was invalid, the call was not
	made from core 0, or all eight native alarm slots are occupied.
	End Rem
	Function AlarmAfterMilliseconds:Int(milliseconds:UInt) = "bmx_pico_alarm_after_ms"
	Function AlarmAfterMicroseconds:Int(microseconds:ULong) = "bmx_pico_alarm_after_us"
	Function RepeatingAlarmMilliseconds:Int(milliseconds:UInt) = "bmx_pico_repeating_alarm_ms"
	Function RepeatingAlarmMicroseconds:Int(microseconds:ULong) = "bmx_pico_repeating_alarm_us"
	Function CancelAlarm:Int(handle:Int) = "bmx_pico_alarm_cancel"
	Function AlarmActive:Int(handle:Int) = "bmx_pico_alarm_active"
	Function PendingAlarmEvents:UInt(handle:Int) = "bmx_pico_alarm_pending_events"

	Rem
	bbdoc: Returns and clears the coalesced event count for an alarm.
	about: Consuming a fired one-shot alarm also releases its native slot.
	End Rem
	Function TakeAlarmEvents:UInt(handle:Int) = "bmx_pico_alarm_take_events"
	Function RemainingAlarmMicroseconds:Long(handle:Int) = "bmx_pico_alarm_remaining_us"
	Function RemainingAlarmMilliseconds:Int(handle:Int) = "bmx_pico_alarm_remaining_ms"
End Extern
?
