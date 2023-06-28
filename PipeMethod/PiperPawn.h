// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

//#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include <string>
#include <windows.h> //for HANDLE and DWORD
#include "PiperPawn.generated.h"

UCLASS(Blueprintable, Config=Game)
class APiperPawn : public APawn
{
	GENERATED_BODY()

	/** StaticMesh component that will be the visuals for our flying pawn */
	UPROPERTY(Category = Mesh, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* PlaneMesh;

	/** Spring arm that will offset the camera */
	UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* SpringArm;

	/** Camera component that will be our viewpoint */
	UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* Camera;
public:
	APiperPawn();

	// Begin AActor overrides
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;
	// End AActor overrides

	/*Give it a string to send over the pipe
	prints green message on screen and returns true if successfully piped
	prints red   message on screen and returns false if not successfully piped*/

		bool pipeIt(std::string message);

	/*give it a stringand an FColor::Color or FColor::toHex("#xxxxxx") to print onscreen*/
	void printIt(std::string message, FColor color);
	float mapIt(float l1, float h1, float l2, float h2, float value);

		UPROPERTY(BlueprintReadWrite)
	float CurrentAcc;

		UPROPERTY(BlueprintReadWrite)
			float pitchAdjust;
		UPROPERTY(BlueprintReadWrite)
			float yawAccel;

		UPROPERTY(BlueprintReadWrite)
			bool ShipShouldMove;


protected:

	// Begin APawn overrides
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override; // Allows binding actions/axes to functions
	// End APawn overrides

	/** Bound to the thrust axis */
	void ThrustInput(float Val);

	/** Bound to the vertical axis */
	void MoveUpInput(float Val);

	/** Bound to the horizontal axis */
	void MoveRightInput(float Val);

	//Bind freeze action
	void Freeze();

	//Bind unfreeze action
	void UnFreeze();

	//Bind endgame action
	UFUNCTION(BlueprintCallable)
	void endgame();

private:

	/** How quickly forward speed changes */
	UPROPERTY(Category=Plane, EditAnywhere)
	float Acceleration;

	/** How quickly pawn can steer */
	UPROPERTY(Category=Plane, EditAnywhere)
	float TurnSpeed;

	/** Max forward speed */
	UPROPERTY(Category = Pitch, EditAnywhere)
	float MaxSpeed;

	/** Min forward speed */
	UPROPERTY(Category=Yaw, EditAnywhere)
	float MinSpeed;

	UPROPERTY(Category = Yaw, EditAnywhere)
		float MaxYaw;

	/** Current forward speed */
	float CurrentForwardSpeed;

	/** Current yaw speed */
	float CurrentYawSpeed;

	/** Current pitch speed */
	float CurrentPitchSpeed;

	/** Current roll speed */
	float CurrentRollSpeed;


	float Drag;
	float slowDown;
	float turnFriction;

	//pipe thread stuff
	HANDLE hPipe;
	DWORD dwWritten;




public:
	/** Returns PlaneMesh subobject **/
	FORCEINLINE class UStaticMeshComponent* GetPlaneMesh() const { return PlaneMesh; }
	/** Returns SpringArm subobject **/
	FORCEINLINE class USpringArmComponent* GetSpringArm() const { return SpringArm; }
	/** Returns Camera subobject **/
	FORCEINLINE class UCameraComponent* GetCamera() const { return Camera; }
};
