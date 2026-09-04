SuperStrict

Framework BRL.StandardIO
Import Pico.System.Power
Import Pico.System.Time

' Give USB serial time to enumerate before the first message.
Delay 2000

Print "Power capabilities: " + PowerCapabilities()
Print "Entering clock-gated sleep for two seconds"
Local before:ULong = MonotonicMilliseconds()
Local sleepResult:EPowerResult = LowPowerSleep(2000)
Local elapsed:ULong = MonotonicMilliseconds() - before
Print "Sleep result=" + Int(sleepResult) + ", elapsed=" + elapsed + " ms"

If PowerSupports(PowerCapabilityDormantTimer) Then
	Print "Entering dormant mode for three seconds; USB serial will reconnect"
	Delay 100
	Local dormantResult:EPowerResult = DormantSleep(3000)
	' Allow the host to rediscover and reopen USB serial before printing.
	Delay 3000
	Print "Dormant result=" + Int(dormantResult)
Else
	Print "Timed dormant is not supported by this processor/SDK combination"
End If

While True
	PowerIdle()
Wend
