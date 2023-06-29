#include "UDPReceiver.h"

//constructy
UDPReceiver::UDPReceiver()
{
	//setup variables
	port = 9090;
	bufLength = 1024;
	pitchVal, rollVal, yawVal = 0.0;
}
//destructy
UDPReceiver::~UDPReceiver()
{

}

/*
	Sets up UDP socket
*/
int UDPReceiver::setupUDP()
{
	//initialize Winsock version 2.2
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cout << "WSAStartup failed! Error code: " << WSAGetLastError() << std::endl;
		return -1;
	}
	else std::cout << "The Winsock DLL status is " << wsaData.szSystemStatus << std::endl;


	//create a new socket to receive datagrams
	receivingSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	//check for errors
	if (receivingSocket == INVALID_SOCKET)
	{
		std::cout << "socket() failed! Error code: " << WSAGetLastError() << std::endl;
		// Clean up
		WSACleanup();
		return -1;
	}

	// The IPv4 family
	receiverAddr.sin_family = AF_INET;
	// Port no. 9090
	receiverAddr.sin_port = htons(port);
	// From all interface (0.0.0.0)
	receiverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	// associate the address information with the socket using bind and error check
	if (bind(receivingSocket, (SOCKADDR*)&receiverAddr, sizeof(receiverAddr)) == SOCKET_ERROR)
	{
		std::cout << "bind() failed! Error code: " << WSAGetLastError() << std::endl;

		//close + clean
		closesocket(receivingSocket);
		WSACleanup();

		//exit with error
		return -1;
	}

	//saves some info
	getsockname(receivingSocket, (SOCKADDR*)&receiverAddr, (int*)sizeof(receiverAddr));

	return 0;
}

/*
	Gets pitch,roll,yaw from UDP datapacket and sets those to be used by the main function
	Returns 1 (true) if completed successfully.
*/
int UDPReceiver::receiveUDPData()
{
	//get datapackets
	byteReceived = recv(receivingSocket, receiveBuf, bufLength, 0);

	//set vals if good
	if (byteReceived > 0)
	{
		//assumes data being received is in correct format
		pitchVal = *(float*)&receiveBuf[0];
		rollVal = *(float*)&receiveBuf[4];
		yawVal = *(float*)&receiveBuf[8];
		timeSec = *(int*)&receiveBuf[12];

	}
	else if (byteReceived <= 0)
	{
		std::cout << "Connection failed! Error code: " << WSAGetLastError() << std::endl;
		return -1;
	}
	else
	{
		std::cout << "recvfrom() failed! Error code: " << WSAGetLastError() << std::endl;
		return -1;
	}

	return 1;
}

/*
	Closes the UDP socket and runs cleanup. Throws errors if necessary.
*/
void UDPReceiver::endUDP()
{
	//close socket
	std::cout << "Done receiving. Closing socket.\n";
	if (closesocket(receivingSocket) != 0)
		std::cout << "closesocket() failed! Error code: " << WSAGetLastError() << std::endl;

	//cleanup
	std::cout << "Cleaning up\n";
	if (WSACleanup() != 0)
		std::cout << "WSACleanup() failed! Error code: " << WSAGetLastError() << std::endl;
}
