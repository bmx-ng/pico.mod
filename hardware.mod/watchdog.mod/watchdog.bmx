' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Hardware watchdog timer and watchdog-reset services for Raspberry Pi Pico targets.
about: Once enabled, call WatchdogFeed regularly before the configured timeout.
Configuration and feeding from multiple cores must be coordinated by the application.
End Rem
Module Pico.Hardware.Watchdog
?pico

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"

Extern "C"
	Rem
	bbdoc: Returns the longest watchdog delay supported by the selected chip, in milliseconds.
	End Rem
	Function WatchdogMaximumDelayMilliseconds:UInt() = "bmx_pico_watchdog_maximum_delay_ms"

	Rem
	bbdoc: Enables the watchdog and returns True, or returns False for an invalid delay.
	about: delayMilliseconds must be non-zero and no greater than
	WatchdogMaximumDelayMilliseconds. Set pauseOnDebug to pause the countdown while
	a debugger halts either core or the JTAG connection.
	End Rem
	Function WatchdogEnable:Int(delayMilliseconds:UInt, pauseOnDebug:Int) = "bmx_pico_watchdog_enable"

	Rem
	bbdoc: Disables the watchdog.
	End Rem
	Function WatchdogDisable() = "bmx_pico_watchdog_disable"

	Rem
	bbdoc: Reloads the watchdog countdown using the delay supplied to WatchdogEnable.
	End Rem
	Function WatchdogFeed() = "bmx_pico_watchdog_feed"

	Rem
	bbdoc: Returns True if a watchdog action caused the last reboot.
	End Rem
	Function WatchdogCausedReboot:Int() = "bmx_pico_watchdog_caused_reboot"

	Rem
	bbdoc: Returns True if a timeout configured by WatchdogEnable caused the last reboot.
	about: Unlike WatchdogCausedReboot, this excludes explicit WatchdogReboot calls and
	boot-ROM watchdog resets such as entering BOOTSEL mode. Query this during startup,
	before calling WatchdogEnable again, because enabling writes its identification marker.
	End Rem
	Function WatchdogEnableCausedReboot:Int() = "bmx_pico_watchdog_enable_caused_reboot"

	Rem
	bbdoc: Returns the remaining watchdog time in microseconds.
	about: Due to an RP2040 hardware limitation, RP2040 returns the last loaded value
	instead of a live countdown. RP2350 returns the live remaining time.
	End Rem
	Function WatchdogRemainingMicroseconds:UInt() = "bmx_pico_watchdog_time_remaining_us"

	Rem
	bbdoc: Returns the remaining watchdog time in milliseconds.
	about: Due to an RP2040 hardware limitation, RP2040 returns the last loaded value
	instead of a live countdown. RP2350 returns the live remaining time.
	End Rem
	Function WatchdogRemainingMilliseconds:UInt() = "bmx_pico_watchdog_time_remaining_ms"

	Rem
	bbdoc: Requests a normal watchdog reboot after the specified delay and returns True.
	about: A zero delay reboots immediately. A delay greater than
	WatchdogMaximumDelayMilliseconds is rejected and returns False. An accepted zero-delay
	request does not return before the reset.
	End Rem
	Function WatchdogReboot:Int(delayMilliseconds:UInt) = "bmx_pico_watchdog_reboot"
End Extern
?
