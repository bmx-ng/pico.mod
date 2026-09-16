' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Framework BRL.EventQueue
Import BRL.StandardIO
Import Pico.Network.WiFi

' Set these in a local copy to exercise concurrent station and SoftAP mode.
Const stationName:String = "<ssid>"
Const stationPassword:String = "<password>"

If WiFiInitialize(WiFiCountryUK) <> 0 Then Throw "Wi-Fi initialization failed"
If WiFiStartAccessPoint("BlitzMax-Pico-test", "short", ..
		WiFiAuthenticationWPA2AESPSK, 6) = 0 Then Throw "Short WPA2 key was accepted"
If WiFiAccessPointActive() Then Throw "Invalid AP became active"

Local result:Int = WiFiStartAccessPoint("BlitzMax-Pico-test", "test-password", ..
	WiFiAuthenticationWPA2AESPSK, 6)
If result <> 0 Then Throw "WPA2 SoftAP start failed: " + result
If Not WiFiAccessPointActive() Then Throw "SoftAP inactive after start"
If Not WiFiAccessPointIPv4Address() Then Throw "SoftAP has no IPv4 address"
Print "SoftAP at " + WiFiAccessPointIPv4Address()

If stationName <> "<ssid>"
	result = WiFiConnectWait(stationName, stationPassword, ..
		WiFiAuthenticationWPA2MixedPSK, 3, 20000, 1000)
	If result <> 0 Then Throw "Station join failed: " + result
	If WiFiLinkStatus() <> WiFiLinkUp Then Throw "Station link is not up"
	Print "Station at " + WiFiIPv4Address()
End If

Local deadline:UInt = MilliSecs() + 15000
While MilliSecs() < deadline
	Local count:UInt
	If WiFiAccessPointClientCount(count) <> 0 Then Throw "Client count failed"
	If stationName <> "<ssid>" And WiFiLinkStatus() <> WiFiLinkUp Then ..
		Throw "SoftAP dropped station link"
	PollSystem()
	Delay 20
Wend

If WiFiStopAccessPoint() <> 0 Then Throw "SoftAP stop failed"
If WiFiAccessPointActive() Then Throw "SoftAP remained active"
If WiFiAccessPointIPv4Address() Then Throw "Stopped SoftAP retained address"
If stationName <> "<ssid>" Then WiFiDisconnect()
If WiFiDeinitialize() <> 0 Then Throw "Wi-Fi teardown failed"
Print "SoftAP lifecycle passed"
