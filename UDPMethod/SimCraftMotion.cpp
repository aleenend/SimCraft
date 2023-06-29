

#include "craftware.h"
#include "UDPReceiver.h"
#include <iostream>

// include for WINAPI
#include <atlbase.h>
#include <windows.h>
#include <string>

#include <Sysinfoapi.h>




// This module name (scGM99) is set aside for all custom interfaces
#define MODULE_NAME "scGM99"


// main - main in this example is substituted as the main thread in the game process implementation
int main(int argc, char* argv[])
{
	bool bInterfaceActive = true;
	bool bCraftwareInitialized = false;
	int timeSec = 0;

	//freeze/unfreeze vars
	float freeze = -666.0;
	float thaw = 666.0;
	bool frozen = false;
	SYSTEMTIME currTime;

	std::cout << "Starting UDP" << "\n";
	UDPReceiver UDPer;
	UDPer.setupUDP();

	// Initialize Craftware: pass in MODULE_NAME, and false to prevent motion system init.
	// The final implementation will require that this second parameter be true.  A value
	// of true will not cause any major problems, just an undesirable warning message box
	// that indicates that the SimCraft device could not be found.  This message box may not
	// be desireable during implementation and testing which is the reason for this option.
	if (Craftware::initialize(MODULE_NAME, true))
	{
		bCraftwareInitialized = true;
		std::cout << "Craftware Initialized!\n";
	}
	else std::cout << "Uh oh... Craftware didn't initialize...\n";

	// bInterfaceActive used as an example - the game/simulation is active
	while (bInterfaceActive)
	{
		//receiveUDPData returns true if there are no errors
		if (UDPer.receiveUDPData())
		{
			if (GetKeyState('Q') < 0) {
				// The Q key is down.
				bInterfaceActive = false;
				std::cout << "Quitting\n";
			}

			//assumes user is not messing up their system time otherwise oh well
			GetLocalTime(&currTime);
			timeSec = currTime.wSecond;

			//ignores data that is older than 3 seconds
			if ((UDPer.getTime() - timeSec) < 3 ||
				//edge case, 59, 0, 1, . 58 59 0 ,
				UDPer.getTime() == 58 && timeSec < 1 ||
				UDPer.getTime() == 59 && timeSec < 2)
			{
				//print statement if you want to view the received data
				//std::cout << "Got " << UDPer.getPitchVal() << ", " << UDPer.getRollVal() << ", " << UDPer.getYawVal() << "\n";
				Craftware::accelerationsAndAngles6DOF(UDPer.getRollVal(), UDPer.getPitchVal(), 1, 1, 1, UDPer.getYawVal(), 0, 0, 0, false, true);
			}


		}
		if (GetKeyState('Q') < 0) {
			// The Q key is down.
			bInterfaceActive = false;
			std::cout << "Quitting\n";
		}
	}
	//closes socket when done
	UDPer.endUDP();

	//shutdown can occur anytime but *must* be called at some point before ending the sim/game process
	std::cout << "Homing and Shutting Down\n";
	Craftware::shutdown(true);

	return 1;
}
