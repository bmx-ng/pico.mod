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
Const serverPort:Int = 8080

Delay 1500

If networkName = "<ssid>" Then
	Throw "Set networkName and networkPassword in a local copy of this example."
End If

If WiFiInitialize(WiFiCountryUK) <> 0 Then Throw "Unable to initialize WiFi"
If WiFiConnect(networkName, networkPassword, WiFiAuthenticationWPA2MixedPSK) <> 0 Then
	Throw "Unable to start WiFi connection"
End If

Local connected:Int
Local started:UInt = MilliSecs()
While Not connected And MilliSecs() - started < 30000
	If PollEvent() = EVENT_WIFILINKSTATE Then
		Local state:TWiFiLinkState = TWiFiLinkState(EventExtra())
		If state And state.status = WiFiLinkUp Then connected = True
		If state And state.status < 0 Then Exit
	End If
	Delay 10
Wend
If Not connected Then Throw "WiFi did not obtain an address"

Local server:TSocket = TSocket.CreateTCP()
If Not server Or Not server.Bind(serverPort) Or Not server.Listen(4) Then
	Throw "Unable to listen on TCP port " + serverPort
End If
If Not server.EnableEvents(SocketEventAccept | SocketEventError) Then
	Throw "Unable to enable socket events"
End If

Print "Listening at http://" + WiFiIPv4Address() + ":" + serverPort + "/"

While True
	Select PollEvent()
		Case EVENT_SOCKETACCEPT
			If EventSource() = server Then
				Local client:TSocket = server.Accept(0)
				While client
					Local stream:TSocketStream = TSocketStream.Create(client)
					stream.WriteString("HTTP/1.0 200 OK~r~nContent-Type: text/plain~r~n" + ..
						"Connection: close~r~n~r~nHello from BlitzMax on Pico!~n")
					stream.Close()
					client = server.Accept(0)
				Wend
			End If
		Case EVENT_SOCKETERROR
			If EventSource() = server Then
				Throw "TCP listener error: " + SocketLastError(server)
			End If
	End Select
	Delay 1
Wend
