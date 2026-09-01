SuperStrict

Import BRL.StandardIO
Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.Hardware.PWM

Local checksPassed:Int = True
Local ledPin:UInt = DefaultLEDPin()
Local slice:UInt = PWMInitGPIO(ledPin)
Local channel:UInt = PWMChannelForGPIO(ledPin)
checksPassed :& slice = PWMSliceForGPIO(ledPin)
checksPassed :& channel = PWMChannelA Or channel = PWMChannelB
checksPassed :& GPIOGetFunction(ledPin) = GPIOFunctionPWM

PWMSetEnabled(slice, False)
PWMSetClockDivider(slice, 1.0)
PWMSetClockDividerIntFrac(slice, 1, 0)
PWMSetWrap(slice, 999)
PWMSetBothLevels(slice, 250, 750)
checksPassed :& PWMGetChannelLevel(slice, PWMChannelA) = 250
checksPassed :& PWMGetChannelLevel(slice, PWMChannelB) = 750
PWMSetCounter(slice, 123)
checksPassed :& PWMGetCounter(slice) = 123

PWMSetPhaseCorrect(slice, False)
PWMSetOutputPolarity(slice, False, False)
PWMSetDividerMode(slice, PWMDividerFreeRunning)
Local achievedFrequency:UInt = PWMSetFrequency(slice, 1000)
Local wrap:UInt = PWMGetWrap(slice)
checksPassed :& achievedFrequency >= 995 And achievedFrequency <= 1005
checksPassed :& wrap > 0 And wrap <= $ffff

Local halfLevel:UInt = (wrap + 1) / 2
PWMSetChannelLevel(slice, channel, halfLevel)
checksPassed :& PWMGetChannelLevel(slice, channel) = halfLevel
PWMSetCounter(slice, 0)
checksPassed :& PWMSetIRQEnabled(slice, True)
PWMSetEnabled(slice, True)
Delay(12)
Local wrapEvents:UInt = PWMTakeWrapEvents(slice)
checksPassed :& wrapEvents >= 5
PWMSetIRQEnabled(slice, False)

' Leave a visible 5 Hz, 50% duty waveform on the onboard LED. The helper
' configures a 10 Hz trailing-edge rate; phase-correct counting halves it.
PWMSetEnabled(slice, False)
PWMSetPhaseCorrect(slice, True)
achievedFrequency = PWMSetFrequency(slice, 10)
wrap = PWMGetWrap(slice)
PWMSetGPIOLevel(ledPin, (wrap + 1) / 2)
PWMSetCounter(slice, 0)
PWMSetEnabled(slice, True)
checksPassed :& achievedFrequency >= 9 And achievedFrequency <= 10

While True
	If checksPassed Then
		Print "PWM configuration, frequency and wrap IRQ checks passed"
	Else
		Print "PWM configuration, frequency or wrap IRQ check failed"
	End If
	Delay(1000)
Wend
