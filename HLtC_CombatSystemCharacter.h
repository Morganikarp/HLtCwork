// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "HLtC_CombatSystemCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class AHLtC_CombatSystemCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Sprinting Toggle Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SprintingAction;

	/** LockOn Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LockOnCheckAction;

	/** Block Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* BlockAction;

	/** Light Attack Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LightAttackAction;

	/** Heavy Attack Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* HeavyAttackAction;

	/** Dodge Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* DodgeAction;

public:
	AHLtC_CombatSystemCharacter();

	// States
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FString PlayerControlState; // The players current control/movement state

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FString PlayerAction; // The players current action

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool StaticAction = false; // If the player can currently move (static actions freeze player movement)
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	double StaticActionDurationTimer = 0.0f; // Countdown till a static action concludes

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FString CameraState; // The cameras current control/movement state

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	int ControlSchemeIndex = 1; // The currently selected control scheme as an index

	// Movement

	float MoveSpeed_Slow = 200.0f; // Move speed of player during "Slow" control state
	
	float MoveSpeed_Action = 400.0f; // Move speed of player during "Active" control state

	bool isSprinting; // Signals if the user is holding the sprint input
	float SprintSpeedAddition = 200.0f; // Added to player speed when sprinting
	
	// Attack

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FString CurrentAttackType; // Type of attack currently being used (Light/Heavy)

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool AttackMechanicsTrigger = false; // Pulse to activate associated attack mechanics (currently hitscan) for a single instance
	
	int LightAttackIndex; // Index of the light attack in the chain currently being used
	int HeavyAttackIndex; // Index of the heavy attack in the chain currently being used
	
	int LightAttackChainLength = 5; // Length of the light attack chain
	int HeavyAttackChainLength = 3; // Length of the heavy attack chain
	
	double LightAttackTimings[5] = { 1.0f, 0.7f, 0.7f, 0.7f, 0.7f }; // Duration of each attack in the light attack chain
	double HeavyAttackTimings[3] = { 1.5f, 1.0f, 1.0f }; // Duration of each attack in the heavy attack chain

	bool AdditionalAttackBuffer; // Signals if the next attack in the chain should trigger as soon as possible. Set to true if the user tries attacking too soon after a prior attack
	double AdditionalAttackBufferTiming; // The duration of time needed to pass after an attack is triggered for a followup attack to be triggered. Based on the complete duration of the prior attack
	double AttackBufferTimingMulti = 0.5f; // The multiplier that determines the initial length of AdditionalAttackBufferTiming

	// Blocking

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool Blocking; // Signals if the user is holding the block input
	
	// Camera
	
	float ArmLengths_Slow[3] = { 175.0f, 225.0f, 275.0f }; // Array of boom arm lengths, defining said lengths for when the control state is "Slow", and PlayerAction is "Idle", "Moving", or "Sprinting" respectively
	FVector BoomSocketOffset_Slow = { 0.0f, 50.0f, 75.0f }; // The cameras offset from the player on the end of the camera boom when the control state is "Slow"
	
	float ArmLengths_Action[2] = { 300.0f, 350.0f }; // Array of boom arm lengths, defining said lengths for when the control state is "Action", and CameraState isn't "Free", and is "Free" respectively
	FVector BoomSocketOffset_Action[2] = { { 0, 0, 50.0f }, { 0, 0, 100.0f } }; // The cameras offset from the player on the end of the camera boom when the control state is "Action"
	
	float DesiredArmLength; // The target boom arm length at any instance, needed for when the actual boom arm length is between values
	FVector DesiredBoomSocketOffset; // The target camera offset at any instance, needed for when the actual camera offset is between values

	bool CamShakeRising = true; // Signals if the camera is currently positively rising during the camera shake
	double CamShakeTiming = 0; // The current timing of the camera shake used in the interpolation to determine the cameras current offset
	double CamShakeTimingConstraint = 0.1f; // Constraints CamShakeTiming. If CamShakeTiming is greater than the constraint, of less than the constraints inverse, then flip CamShakeRising so that camera shake moves in the opposite direction
	float CamShakeDeltaTimeDivision[2] = { 2.0f, 1.2f }; // An array containing the values that divide the amount added to CamShakeTiming every frame, to vary the timing of the camera shake. They are for "Moving" and "Sprinting" respectively

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	
protected:
	// To add mapping context
	virtual void BeginPlay();

	virtual void Tick(float DeltaTime) override;

	void SetControlStateDefaults(float DeltaTime); // Sets the default values associated with each control, action and camera state every frame

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for sprinting input */
	void SprintingFlag(const FInputActionValue& Value); // Executes when sprint input action is triggered. Sets isSprinting

	/** Called for light attack input */
	void LightAttack(const FInputActionValue& Value); // Executes when light attack input action is triggered. Determines what light attack should be used and when

	/** Called for heavy attack input */
	void HeavyAttack(const FInputActionValue& Value); // Executes when heavy attack input action is triggered. Determines what heavy attack should be used and when

	/** Called for block input */
	void Block(const FInputActionValue& Value); // Executes when sprint input action is triggered. Sets Blocking
	
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};

