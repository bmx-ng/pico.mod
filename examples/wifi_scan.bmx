' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Framework BRL.EventQueue
Import BRL.StandardIO
Import Pico.Network.WiFi

Local result:Int = WiFiInitialize(WiFiCountryUK)
If result <> 0 Then Throw "Unable to initialize WiFi: " + result

WiFiSetLED(True)
Delay 150
WiFiSetLED(False)

result = WiFiStartScan()
If result <> 0 Then Throw "Unable to start WiFi scan: " + result

Print "Scanning for wireless networks..."
Local complete:Int
While Not complete
	Select WaitEvent()
		Case EVENT_WIFISCANRESULT
			Local network:TWiFiNetwork = TWiFiNetwork(EventExtra())
			If network Then
				Print network.ssid + "  channel=" + network.channel + ..
					"  rssi=" + network.rssi + "  security=" + network.security
			End If
		Case EVENT_WIFISCANCOMPLETE
			complete = True
	End Select
Wend

Print "Scan complete; dropped records=" + WiFiDroppedScanResults()
WiFiSetLED(True)
Delay 1000
WiFiSetLED(False)
result = WiFiDeinitialize()
If result <> 0 Then Throw "Unable to deinitialize WiFi: " + result
Print "WiFi deinitialized"
