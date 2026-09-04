SuperStrict

Framework BRL.StandardIO
Import Pico.System.Power

Local capabilities:UInt = PowerCapabilities()
If Not PowerSupports(PowerCapabilityIdle) Then Throw "idle capability missing"

If capabilities & PowerCapabilitySleepTimer Then
	Local sleepResult:EPowerResult = LowPowerSleep(10)
End If

If capabilities & PowerCapabilitySleepInterrupt Then
	' Keep this compile fixture non-blocking; merely retain the callable signature.
	Local sleepFunction:EPowerResult() = LowPowerSleepUntilInterrupt
End If

If capabilities & PowerCapabilityDormantTimer Then
	Local dormantFunction:EPowerResult(milliseconds:UInt) = DormantSleep
End If

If capabilities & PowerCapabilityDormantGPIO Then
	Local gpioFunction:EPowerResult(pin:UInt, edge:Int, high:Int) = DormantSleepUntilGPIO
End If

Local idleFunction() = PowerIdle
Print capabilities
