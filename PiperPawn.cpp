// Copyright Epic Games, Inc. All Rights Reserved.

#include "PiperPawn.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Kismet/KismetSystemLibrary.h" //quit game

#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>
#include <winbase.h>
#include <cstring>


APiperPawn::APiperPawn()
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{

		ConstructorHelpers::FObjectFinderOptional<UStaticMesh> PlaneMesh;
		FConstructorStatics()
			: PlaneMesh(TEXT("/Game/Flying/Meshes/UFO.UFO"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	// Create static mesh component
	PlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaneMesh0"));
	PlaneMesh->SetStaticMesh(ConstructorStatics.PlaneMesh.Get());	// Set static mesh
	RootComponent = PlaneMesh;

	// Create a spring arm component
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm0"));
	SpringArm->SetupAttachment(RootComponent);	// Attach SpringArm to RootComponent
	SpringArm->TargetArmLength = 160.0; // The camera follows at this distance behind the character
	SpringArm->SocketOffset = FVector(0.0,0.0,60.0);
	SpringArm->bEnableCameraLag = false;	// Do not allow camera to lag
	SpringArm->CameraLagSpeed = 15.0;

	// Create camera component
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera0"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);	// Attach the camera
	Camera->bUsePawnControlRotation = false; // Don't rotate camera with controller

	//make pipe
	hPipe = CreateFile(TEXT("\\\\.\\pipe\\Pipe"),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	// Set handling parameters
	Acceleration = 500.0;
	TurnSpeed = 50.0;
	MaxSpeed = 1000.0;
	MinSpeed = -1000.0;
	MaxYaw = 100.0;
	CurrentForwardSpeed = 500.0;
	slowDown = 0.9;
	Drag = 0.0;
	turnFriction = 0.0;
	pitchAdjust = 0.0;

	ShipShouldMove = true;

}

float APiperPawn::mapIt(float l1, float h1, float l2, float h2, float val)
{
	float slope = (h2 - l2) / (h1 / l1);
	return l2 + slope * (val - l1);
}

//function to easily print debug messages on screen
void APiperPawn::printIt(std::string message, FColor color)
{
	GEngine->AddOnScreenDebugMessage(-1, 15.0, color, message.c_str());
}

//sends a string message through the pipe
bool APiperPawn::pipeIt(std::string message)
{
	FString outmessage;

	if (hPipe != INVALID_HANDLE_VALUE)
	{
		const int len = message.length();
		char* msg = new char[len + 1];
		strcpy(msg, message.c_str());

		outmessage = ("Piping " + message).c_str();


		GEngine->AddOnScreenDebugMessage(-1, 15.0, FColor::Green, outmessage);
		WriteFile(hPipe,
			msg,
			len,   // = length of string + terminating '\0' !!!
			&dwWritten,
			NULL);

		return true;

	}

	return false;
}

void APiperPawn::Tick(float DeltaSeconds)
{
	if (ShipShouldMove)
	{
		FRotator rotation = GetActorRotation();
		float pitch = rotation.GetComponentForAxis(EAxis::Y);
		float roll = rotation.GetComponentForAxis(EAxis::X);
		float yaw = rotation.GetComponentForAxis(EAxis::Z);

		//pitchAdjust is set in Unreal blueprint
		pitch += pitchAdjust;

		//limit pitch values to prevent loop-de-loops
		if (pitch > 13.0) SetActorRotation(FRotator(13.0, yaw, roll));
		if (pitch < -24.0) SetActorRotation(FRotator(-24.0, yaw, roll));

		std::string messageToPipe = std::to_string(CurrentAcc * 2.0 / 90.0 + pitch) + "," + std::to_string(roll) + "," + std::to_string(yawAccel);
		//pipe pitch roll and yaw. If pipe returns false, still print out what we would be piping
		if (!pipeIt(messageToPipe)) GEngine->AddOnScreenDebugMessage(-1, 15.0, FColor::Red, ("not piping: " + messageToPipe).c_str());
	}



	//below this line is default unreal tick actions
	const FVector LocalMove = FVector(CurrentForwardSpeed * DeltaSeconds, 0.0, 0.0);

	// Move plan forwards (with sweep so we stop when we collide with things)
	AddActorLocalOffset(LocalMove, true);

	// Calculate change in rotation this frame
	FRotator DeltaRotation(0,0,0);
	DeltaRotation.Pitch = CurrentPitchSpeed * DeltaSeconds;
	DeltaRotation.Yaw = CurrentYawSpeed * DeltaSeconds;
	DeltaRotation.Roll = CurrentRollSpeed * DeltaSeconds;

	// Rotate plane
	AddActorLocalRotation(DeltaRotation);

	// Call any parent class Tick implementation
	Super::Tick(DeltaSeconds);

}

void APiperPawn::NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	// Deflect along the surface when we collide.
	FRotator CurrentRotation = GetActorRotation();
	SetActorRotation(FQuat::Slerp(CurrentRotation.Quaternion(), HitNormal.ToOrientationQuat(), 0.025));
}


void APiperPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
    // Check if PlayerInputComponent is valid (not NULL)
	check(PlayerInputComponent);

	// Bind our control axis' to callback functions
	PlayerInputComponent->BindAxis("Thrust", this, &APiperPawn::ThrustInput);
	PlayerInputComponent->BindAxis("MoveUp", this, &APiperPawn::MoveUpInput);
	PlayerInputComponent->BindAxis("MoveRight", this, &APiperPawn::MoveRightInput);
	PlayerInputComponent->BindAction("freeze", IE_Pressed, this, &APiperPawn::Freeze);
	PlayerInputComponent->BindAction("unfreeze", IE_Pressed, this, &APiperPawn::UnFreeze);
	//PlayerInputComponent->BindAction("endgame", IE_Pressed, this, &APiperPawn::endgame);
}

void APiperPawn::ThrustInput(float Val)
{
	if(ShipShouldMove)
	{
		/* Using XBox right stick gives weird results. If you have input,
		   make it such that pushing stick up increases val to 1
		   pushing it down decreases to -1
		   for some reason, if pushed all the way up or down the val is 0
		*/
		if (Val != 0) Val = (Val < 0) ? -1.0 - Val : 1.0 - Val;
		bool bHasInput = !FMath::IsNearlyEqual(Val, 0.0);

		//if we are moving less than 1 speed, just say we're not moving
		if (abs(CurrentForwardSpeed) < 1.0 && !bHasInput)
		{
			CurrentForwardSpeed = 0.0;
			CurrentAcc = 0.0;
			return;
		}

		CurrentAcc = Val * Acceleration;


		// If input is not held down, reduce speed
		if (!bHasInput) CurrentAcc = CurrentAcc - slowDown * CurrentForwardSpeed;

		//set drag such that  drag:currentAcc = currentSpeed:MaxSpeed
		float deltaspeed = MaxSpeed - abs(CurrentForwardSpeed);
		Drag = (1.0 - deltaspeed / MaxSpeed) * CurrentAcc;

		//if you are accelerating in the same direction as your speed, add drag to slow you down as you get to max speed
		if ((CurrentForwardSpeed > 0 && CurrentAcc > 0) || (CurrentForwardSpeed < 0 && CurrentAcc < 0)) CurrentAcc -= Drag;

		// Calculate new speed
		float NewForwardSpeed = CurrentForwardSpeed + (GetWorld()->GetDeltaSeconds() * CurrentAcc);
		// Clamp between MinSpeed and MaxSpeed
		CurrentForwardSpeed = FMath::Clamp(NewForwardSpeed, MinSpeed, MaxSpeed);
	}
}

void APiperPawn::MoveUpInput(float Val)
{
	if(ShipShouldMove)
	{
		// Target pitch speed is based in input
		float TargetPitchSpeed = (Val * TurnSpeed * -1.0);

		// When steering, we decrease pitch slightly
		TargetPitchSpeed += (FMath::Abs(CurrentYawSpeed) * -0.2);

		// Smoothly interpolate to target pitch speed
		CurrentPitchSpeed = FMath::FInterpTo(CurrentPitchSpeed, TargetPitchSpeed, GetWorld()->GetDeltaSeconds(), 2.0);
	}
}

void APiperPawn::MoveRightInput(float Val)
{
	if(ShipShouldMove)
	{
		yawAccel = Val;

		//set turn friction to slow acceleration as you reach max speed
		turnFriction = (1.0 - (MaxYaw - abs(CurrentYawSpeed)) / MaxYaw) * yawAccel;
		if ((CurrentYawSpeed > 0 && yawAccel > 0) || (CurrentYawSpeed < 0 && yawAccel < 0)) yawAccel -= turnFriction;

		// Target yaw speed is based on input
		float TargetYawSpeed = (Val * TurnSpeed);

		// Smoothly interpolate to target yaw speed
		CurrentYawSpeed = FMath::FInterpTo(CurrentYawSpeed, TargetYawSpeed, GetWorld()->GetDeltaSeconds(), 2.0);

		// Is there any left/right input?
		const bool bIsTurning = FMath::Abs(Val) > 0.2;

		// If turning, yaw value is used to influence roll
		// If not turning, roll to reverse current roll value.
		float TargetRollSpeed = bIsTurning ? (CurrentYawSpeed * 0.5) : (GetActorRotation().Roll * -2.0);
		//(Val * TurnSpeed);// CurrentYawSpeed;//

		// Smoothly interpolate roll speed
		CurrentRollSpeed = FMath::FInterpTo(CurrentRollSpeed, TargetRollSpeed, GetWorld()->GetDeltaSeconds(), 2.0);
	}
}

//debugging function which disables ship's motion
void APiperPawn::Freeze()
{
	CurrentRollSpeed = CurrentYawSpeed = CurrentPitchSpeed = CurrentForwardSpeed = 0.0;
	yawAccel = CurrentAcc = 0.0;
	ShipShouldMove = false;
	printIt("\n FREEZE!!!!!\n", FColor::FromHex("#A3E1E6"));
}

//debugging function to enable ship motion
void APiperPawn::UnFreeze()
{
	printIt("\n THAW!!! \n", FColor::FromHex("#E65020"));
	ShipShouldMove = true;
}

//function to call at end of game to close pipe
void APiperPawn::endgame()
{
	pipeIt("-333");
	pipeIt("-333");
	CloseHandle(hPipe);
}
