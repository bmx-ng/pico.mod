SuperStrict

Import BRL.StandardIO
Import Pico.Hardware.Watchdog

Local checksPassed:Int = True
Local watchdogCausedReboot:Int = WatchdogCausedReboot()
Local watchdogEnableCausedReboot:Int = WatchdogEnableCausedReboot()
Local maximumDelay:UInt = WatchdogMaximumDelayMilliseconds()
checksPassed :& maximumDelay = 8388 Or maximumDelay = 16777
checksPassed :& Not WatchdogEnable(0, True)
checksPassed :& Not WatchdogEnable(maximumDelay + 1, True)
checksPassed :& Not WatchdogReboot(maximumDelay + 1)

WatchdogDisable()
checksPassed :& WatchdogEnable(250)
Local remainingBeforeFeed:UInt = WatchdogRemainingMilliseconds()
Delay 10
WatchdogFeed()
Local remainingAfterFeed:UInt = WatchdogRemainingMilliseconds()
Local remainingMicroseconds:UInt = WatchdogRemainingMicroseconds()
checksPassed :& remainingBeforeFeed > 0 And remainingBeforeFeed <= 250
checksPassed :& remainingAfterFeed > 0 And remainingAfterFeed <= 250
checksPassed :& remainingMicroseconds > 0 And remainingMicroseconds <= 250000
WatchdogDisable()

Print "Watchdog reboot: " + watchdogCausedReboot
Print "Watchdog-enable reboot: " + watchdogEnableCausedReboot

While True
	If checksPassed Then
		Print "Watchdog configuration, validation and feed checks passed"
	Else
		Print "Watchdog configuration, validation or feed check failed"
	End If
	Delay 1000
Wend
