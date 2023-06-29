#pragma once

#include <iostream>
//UDP
#include <WinSock2.h>


class UDPReceiver
{
public:
	//vars
	WSADATA wsaData;
	SOCKET receivingSocket;
	SOCKADDR_IN receiverAddr;
	int port, byteReceived, bufLength, timeSec;
	char receiveBuf[1024];

	//simcraft orientation stuff
	float pitchVal, rollVal, yawVal;

	//constructy
	UDPReceiver();
	//destructy
	~UDPReceiver();

	//setup
	int setupUDP();

	//receiver for data
	int receiveUDPData();

	//closes socket
	void endUDP();

	//getters, don't need setters
	float UDPReceiver::getPitchVal() { return pitchVal; }
	float UDPReceiver::getRollVal() { return rollVal; }
	float UDPReceiver::getYawVal() { return yawVal; }
	int UDPReceiver::getTime() { return timeSec; }


};
