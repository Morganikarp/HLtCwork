// Copyright Epic Games, Inc. All Rights Reserved.

#include "HLtC_CombatSystemCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AHLtC_CombatSystemCharacter

AHLtC_CombatSystemCharacter::AHLtC_CombatSystemCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 200.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AHLtC_CombatSystemCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	PlayerControlState = "Slow";
	PlayerAction = "Idle";
	CameraState = "Free";

	isSprinting = false;
}

void AHLtC_CombatSystemCharacter::Tick(float DeltaTime)
{
	SetControlStateDefaults(DeltaTime); // Checks values every frame
	Super::Tick(DeltaTime);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AHLtC_CombatSystemCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AHLtC_CombatSystemCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AHLtC_CombatSystemCharacter::Look);

		// Sprinting Flag
		EnhancedInputComponent->BindAction(SprintingAction, ETriggerEvent::Triggered, this, &AHLtC_CombatSystemCharacter::SprintingFlag);

		// Light Attack
		EnhancedInputComponent->BindAction(LightAttackAction, ETriggerEvent::Started, this, &AHLtC_CombatSystemCharacter::LightAttack);
		
		// Heavy Attack
		EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Started, this, &AHLtC_CombatSystemCharacter::HeavyAttack);

		// Blocking
		EnhancedInputComponent->BindAction(BlockAction, ETriggerEvent::Started, this, &AHLtC_CombatSystemCharacter::Block);
		EnhancedInputComponent->BindAction(BlockAction, ETriggerEvent::Completed, this, &AHLtC_CombatSystemCharacter::Block);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AHLtC_CombatSystemCharacter::SetControlStateDefaults(float DeltaTime) // Sets the default values associated with each control, action and camera state every frame
{
	if (!StaticAction) // If the player isn't performing a static action (e.g. an attack)...
	{
		if (GetCharacterMovement()->Velocity == FVector(0.0f, 0.0f, 0.0f)) // If the player is standing still...
		{
			PlayerAction = "Idle";
		}
	
		else // If the player isn't standing still...
		{
			if (isSprinting) // If the player is sprinting...
			{
				PlayerAction = "Sprinting";
			}

			else // If the player isn't sprinting...
			{
				PlayerAction = "Moving";
			}
		}
	
		if (PlayerControlState == "Slow") // If the control state is "Slow"...
		{
			GetCharacterMovement()->MaxWalkSpeed = MoveSpeed_Slow; // Set the players move speed to the associated variable
			DesiredBoomSocketOffset = BoomSocketOffset_Slow; // Set the target boom offset to the associated variable
			CamShakeTiming = 0; // Reset CamShakeTiming
			
			if (PlayerAction == "Idle") { // If the player is standing still...
				DesiredArmLength = ArmLengths_Slow[0]; // Set the target boom offset to the associated variable
			}
		
			else if (PlayerAction == "Moving") { // If the player is moving...
				DesiredArmLength = ArmLengths_Slow[1]; // Set the target boom offset to the associated variable
			}
		
			else if (PlayerAction == "Sprinting") { // If the player is sprinting...
				DesiredArmLength = ArmLengths_Slow[2]; // Set the target boom offset to the associated variable
			}
		}
	
		else if (PlayerControlState == "Action") // If the control state is "Action"...
		{
			GetCharacterMovement()->MaxWalkSpeed = MoveSpeed_Action; // Set the players move speed to the associated variable

			if (CameraState == "Focus") // If the player is locked onto an enemy...
			{
				DesiredArmLength = ArmLengths_Action[1]; // Set the target boom length and offset to the associated variables
				DesiredBoomSocketOffset = BoomSocketOffset_Action[1];
			}

			else // If the player isn't locked onto an enemy...
			{
				DesiredArmLength = ArmLengths_Action[0]; // Set the target boom length and offset to the associated variables
				DesiredBoomSocketOffset = BoomSocketOffset_Action[0];
			}
		}

		if (isSprinting) { GetCharacterMovement()->MaxWalkSpeed += SprintSpeedAddition; } // If the player is sprinting, add the additional speed mod to the move speed

		CameraBoom->TargetArmLength = FMath::Lerp(CameraBoom->TargetArmLength, DesiredArmLength, DeltaTime * 2.5f); // Set the boom length to a value interpolated between its current and desired length

		if (PlayerControlState == "Action" && CameraState == "Free") // If the control state is "Action" and the camera is open to player input...
		{
			float newCamShakeTiming = 0; // Define new value for later addition
			if (PlayerAction == "Moving") { newCamShakeTiming = DeltaTime / CamShakeDeltaTimeDivision[0]; } // If player is moving, set newCamShakeTiming to the DeltaTime divided by the associated index in CamShakeDeltaTimeDivision
			if (PlayerAction == "Sprinting") { newCamShakeTiming = DeltaTime / CamShakeDeltaTimeDivision[1]; } // If player is sprinting, set newCamShakeTiming to the DeltaTime divided by the associated index in CamShakeDeltaTimeDivision

			if (!CamShakeRising) { newCamShakeTiming *= -1;} // If the camera should be dropping in the shake, invert newCamShakeTiming

			CamShakeTiming += newCamShakeTiming;

			if (CamShakeTiming >= CamShakeTimingConstraint) { CamShakeTiming = CamShakeTimingConstraint; CamShakeRising = false; } // If CamShakeTiming is greater or equal to CamShakeTimingConstraint, set CamShakeTiming to CamShakeTimingConstraint and signal that the camera should drop
			if (CamShakeTiming <= -CamShakeTimingConstraint) { CamShakeTiming = -CamShakeTimingConstraint; CamShakeRising = true; } // If CamShakeTiming is less or equal to negative CamShakeTimingConstraint, set CamShakeTiming to negative CamShakeTimingConstraint and signal that the camera should rise

			FVector ShakenBoomSocketOffset = DesiredBoomSocketOffset;
			
			if (PlayerAction == "Moving" || PlayerAction == "Sprinting") { ShakenBoomSocketOffset += FVector(0,0,1); } // If the player is moving or sprinting, offset the vector

			if (ShakenBoomSocketOffset != DesiredBoomSocketOffset) // If the shaken offset is different from the desired offset...
			{
				DesiredBoomSocketOffset = FMath::Lerp(ShakenBoomSocketOffset, -ShakenBoomSocketOffset, CamShakeTiming); // Set the desired offset to an interpolated vector between the shaken offset and the negative shaken offset, based on CamShakeTiming
			}

			else // If the player isn't moving or sprinting...
			{
				CamShakeTiming = 0;
			}
		}
		
		CameraBoom->SocketOffset = FMath::Lerp(CameraBoom->SocketOffset, DesiredBoomSocketOffset, DeltaTime * 10.0f); // Set the camera offset to an interpolated vector between its current and desired location
	}

	else // If the player is performing a static action (e.g. an attack)
	{
		StaticActionDurationTimer -= DeltaTime; // Countdown of the action duration

		if (StaticActionDurationTimer <= 0.0f) // If the timer is less than or equal to 0...
		{
			// Set variables ready for the player to move freely again
			StaticAction = false;
			StaticActionDurationTimer = 0.0f;
			LightAttackIndex = 0;
			HeavyAttackIndex = 0;

			AdditionalAttackBufferTiming = 0.0f;
			AdditionalAttackBuffer = false;
		}

		else if (StaticActionDurationTimer <= AdditionalAttackBufferTiming && AdditionalAttackBuffer) // If the remaining action duration is less than the current AdditionalAttackBufferTiming, and an additional attack has been buffered...
		{
			if (CurrentAttackType == "Light") // If the prior attack was "Light"...
			{
				LightAttack(false); // Trigger an additional light attack
			}

			else if (CurrentAttackType == "Heavy") // If the prior attack was "Heavy"...
			{
				HeavyAttack(false); // Trigger an additional heavy attack
			}
		}
	}
}

void AHLtC_CombatSystemCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr && !StaticAction)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AHLtC_CombatSystemCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AHLtC_CombatSystemCharacter::SprintingFlag(const FInputActionValue& Value)
{
	isSprinting = Value.Get<bool>(); // Set the value to if the input it being pressed or released
}

void AHLtC_CombatSystemCharacter::LightAttack(const FInputActionValue& Value)
{
	if (HeavyAttackIndex == 0 && LightAttackIndex < LightAttackChainLength) // If not using the heavy attack and the current light attack is not the last in the chain...
	{
		if (StaticActionDurationTimer <= AdditionalAttackBufferTiming) // If the remaining duration on the current attack is less or equal to the AdditionalAttackBufferTiming...
		{
			// Set variables to trigger an attack
			StaticAction = true;
			Blocking = false;
			CurrentAttackType = "Light";
			AttackMechanicsTrigger = true;

			PlayerAction = "LightAttack_" + FString::FromInt(LightAttackIndex);
			StaticActionDurationTimer = LightAttackTimings[LightAttackIndex]; // Set the attack duration based on what attack it is
			AdditionalAttackBufferTiming = StaticActionDurationTimer * AttackBufferTimingMulti; // Set the attack buffer based on StaticActionDurationTimer
			AdditionalAttackBuffer = false;
			LightAttackIndex++;	// Update the index so it reflects the current attack being used
		}

		else // If the user attempts to attack too soon after a prior attack...
		{
			CurrentAttackType = "Light";
			AdditionalAttackBuffer = true; // Buffer an attack to use as soon as it can be
		}
	}
}

void AHLtC_CombatSystemCharacter::HeavyAttack(const FInputActionValue& Value)
{
	if (LightAttackIndex == 0 && HeavyAttackIndex < HeavyAttackChainLength) // If not using the light attack and the current heavy attack is not the last in the chain...
	{
		if (StaticActionDurationTimer <= AdditionalAttackBufferTiming) // If the remaining duration on the current attack is less or equal to the AdditionalAttackBufferTiming...
		{
			// Set variables to trigger an attack
			StaticAction = true;
			Blocking = false;
			CurrentAttackType = "Heavy";
			AttackMechanicsTrigger = true;

			PlayerAction = "HeavyAttack_" + FString::FromInt(HeavyAttackIndex);
			StaticActionDurationTimer = HeavyAttackTimings[HeavyAttackIndex]; // Set the attack duration based on what attack it is
			AdditionalAttackBufferTiming = StaticActionDurationTimer * AttackBufferTimingMulti; // Set the attack buffer based on StaticActionDurationTimer
			AdditionalAttackBuffer = false;
			HeavyAttackIndex++;	// Update the index so it reflects the current attack being used
		}

		else // If the user attempts to attack too soon after a prior attack...
		{
			CurrentAttackType = "Heavy";
			AdditionalAttackBuffer = true; // Buffer an attack to use as soon as it can be
		}
	}
}

void AHLtC_CombatSystemCharacter::Block(const FInputActionValue& Value)
{
	if (!StaticAction) // If the player isn't doing an action...
	{
		if (Value.Get<bool>()) // If the blocking input is being pressed or held...
		{
			Blocking = true;
		}

		else // If the blocking input is being released...
		{
			Blocking = false;
		}
	}
}
