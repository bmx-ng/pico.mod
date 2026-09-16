' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Framework BRL.EventQueue
Import BRL.StandardIO
Import Pico.Network.WiFi

If WiFiInitialize(WiFiCountryUK) <> 0 Then Throw "Unable to initialize Wi-Fi"

' Use a local copy and supply a password to test WPA2 instead.
Local result:Int = WiFiStartAccessPoint("BlitzMax-Pico", "", WiFiAuthenticationOpen, 6)
If result <> 0 Then Throw "Unable to start SoftAP: " + result
If Not WiFiAccessPointActive() Then Throw "SoftAP did not become active"
Print "SoftAP address: " + WiFiAccessPointIPv4Address()
Print "SoftAP netmask: " + WiFiAccessPointIPv4Netmask()
Print "SoftAP gateway: " + WiFiAccessPointIPv4Gateway()

While True
	Local count:UInt
	result = WiFiAccessPointClientCount(count)
	If result <> 0 Then Throw "Unable to read SoftAP client count: " + result
	Print "Associated clients: " + count
	PollSystem()
	Delay 2000
Wend
