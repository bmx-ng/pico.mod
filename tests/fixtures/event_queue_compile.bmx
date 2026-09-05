SuperStrict

Framework BRL.EventQueue
Import Pico.Hardware.GPIO

Function GPIOEventTimestamp:ULong(source:TGPIOIRQSource, event:TEvent)
	If source And event And event.source = source Then Return GPIOIRQTimeUS(event)
	Return 0
End Function

Local event:TEvent = CreateEvent(EVENT_STREAMAVAIL, Null, 7)
PostEvent(event)

If PollEvent() <> EVENT_STREAMAVAIL Then Throw "event id mismatch"
If EventData() <> 7 Then Throw "event data mismatch"
