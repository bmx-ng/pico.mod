' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Framework BRL.EventQueue
Import BRL.StandardIO
Import Pico.Network.WiFi

' Copy this example before entering local credentials; do not commit them.
Const networkName:String = "<ssid>"
Const networkPassword:String = "<password>"

' Give USB serial monitors time to reconnect after a fresh upload.
Delay 1500

Local result:Int = WiFiInitialize(WiFiCountryUK)
If result <> 0 Then Throw "Unable to initialize WiFi: " + result

If networkName = "<ssid>" Then
	Print "Set networkName and networkPassword in a local copy of this example."
Else
	result = WiFiConnect(networkName, networkPassword, WiFiAuthenticationWPA2MixedPSK)
	If result <> 0 Then Throw "Unable to start WiFi connection: " + result

	Local finished:Int
	Local connected:Int
	Local started:UInt = MilliSecs()
	While Not finished And MilliSecs() - started < 30000
		If PollEvent() = EVENT_WIFILINKSTATE
			Local state:TWiFiLinkState = TWiFiLinkState(EventExtra())
			If state Then
				Print "WiFi link state=" + state.status
				If state.status = WiFiLinkUp Then
					connected = True
					finished = True
					Print "Address: " + state.address
					Print "Netmask: " + state.netmask
					Print "Gateway: " + state.gateway
				Else If state.status < 0 Then
					finished = True
				End If
			End If
		End If
		Delay 10
	Wend

	If connected Then
		Print "Live address: " + WiFiIPv4Address()
		Delay 1000
		WiFiDisconnect()
	Else
		Print "WiFi connection did not obtain an address"
	End If
End If

Delay 100
result = WiFiDeinitialize()
If result <> 0 Then Throw "Unable to deinitialize WiFi: " + result
