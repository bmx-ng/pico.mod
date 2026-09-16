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
Local result:Int = WiFiConnectWait(networkName, networkPassword, ..
	WiFiAuthenticationWPA2MixedPSK)
If result <> 0 Then Throw "Unable to connect WiFi: " + result

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
					' Consume the request before closing so TCP can finish with a clean FIN.
					Local request:Byte[1024]
					Local requestDeadline:UInt = MilliSecs() + 1000
					While client.ReadAvail() = 0 And MilliSecs() < requestDeadline
						Delay 1
					Wend
					While client.ReadAvail() > 0
						Local amount:Int = client.ReadAvail()
						If amount > request.length Then amount = request.length
						If client.Recv(Varptr request[0], Size_T(amount)) <= 0 Then Exit
					Wend
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
