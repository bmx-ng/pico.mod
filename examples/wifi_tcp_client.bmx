' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Framework BRL.EventQueue
Import BRL.SocketStream
Import BRL.StandardIO
Import Pico.Network.WiFi

' Copy this example before entering local credentials; do not commit them.
Const networkName:String = "<ssid>"
Const networkPassword:String = "<password>"

Delay 1500

If networkName = "<ssid>" Then
	Throw "Set networkName and networkPassword in a local copy of this example."
End If

Local result:Int = WiFiInitialize(WiFiCountryUK)
If result <> 0 Then Throw "Unable to initialize WiFi: " + result

result = WiFiConnectWait(networkName, networkPassword, WiFiAuthenticationWPA2MixedPSK)
If result <> 0 Then Throw "Unable to connect WiFi: " + result

Print "Connected as " + WiFiIPv4Address()
Print "Resolving and connecting to example.com..."

Local stream:TSocketStream = TSocketStream.CreateClient("example.com", 80)
If Not stream Then Throw "Unable to connect to example.com"

stream.WriteString("GET / HTTP/1.0~r~nHost: example.com~r~nConnection: close~r~n~r~n")
Print stream.ReadLine()
stream.Close()

WiFiDisconnect()
result = WiFiDeinitialize()
If result <> 0 Then Throw "Unable to deinitialize WiFi: " + result
