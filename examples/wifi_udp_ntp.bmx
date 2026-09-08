' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Framework BRL.EventQueue
Import BRL.Socket
Import BRL.StandardIO
Import Pico.Network.WiFi

' Copy this example before entering local credentials; do not commit them.
Const networkName:String = "<ssid>"
Const networkPassword:String = "<password>"

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

Local addresses:TAddrInfo[] = AddrInfo("pool.ntp.org", "123", AF_INET_)
If Not addresses.length Then Throw "Unable to resolve pool.ntp.org"

Local socket:TSocket = TSocket.CreateUDP()
If Not socket Then Throw "Unable to create UDP socket"
If Not socket.EnableEvents(SocketEventReadable | SocketEventError) Then
	Throw "Unable to enable socket events"
End If

Local packet:Byte[48]
packet[0] = $1b
Local serverAddress:Byte Ptr = addresses[0].HostIp().ToUTF8String()
Local sent:Int = sendto_(socket.Socket(), packet, packet.length, 0, serverAddress, 123)
MemFree(serverAddress)
If sent <> packet.length Then Throw "Unable to send NTP request"

Local deadline:UInt = MilliSecs() + 10000
Local received:Int
While MilliSecs() < deadline
	Select PollEvent()
		Case EVENT_SOCKETREADABLE
			If EventSource() = socket Then
				Local senderAddress:Int
				Local senderPort:Int
				Local count:Int = recvfrom_(socket.Socket(), packet, packet.length, 0, ..
					senderAddress, senderPort)
				If count < 48 Then Throw "Incomplete NTP response"
				Local seconds:ULong = (ULong(packet[40]) Shl 24) | ..
					(ULong(packet[41]) Shl 16) | (ULong(packet[42]) Shl 8) | packet[43]
				Print "NTP Unix time: " + (seconds - 2208988800:ULong)
				received = True
				Exit
			End If
		Case EVENT_SOCKETERROR
			If EventSource() = socket Then Throw "UDP error: " + SocketLastError(socket)
	End Select
	Delay 1
Wend

socket.Close()
WiFiDisconnect()
WiFiDeinitialize()
If Not received Then Throw "Timed out waiting for the NTP response"
