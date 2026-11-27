// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Horror/HorrorCharacter.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SpotLightComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "SprayTool.h"
#include "WiperTool.h"
#include "WindowCleaner.h"

AHorrorCharacter::AHorrorCharacter()
{
	SprayToolClass = ASprayTool::StaticClass();
	WiperToolClass = AWiperTool::StaticClass();
	SelectSprayAction = CreateDefaultSubobject<UInputAction>(TEXT("SelectSprayAction"));
	SelectWiperAction = CreateDefaultSubobject<UInputAction>(TEXT("SelectWiperAction"));
	UseToolAction = CreateDefaultSubobject<UInputAction>(TEXT("UseToolAction"));

	// create the spotlight
	SpotLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("SpotLight"));
	SpotLight->SetupAttachment(GetFirstPersonCameraComponent());

	SpotLight->SetRelativeLocationAndRotation(FVector(30.0f, 17.5f, -5.0f), FRotator(-18.6f, -1.3f, 5.26f));
	SpotLight->Intensity = 0.5;
	SpotLight->SetIntensityUnits(ELightUnits::Lumens);
	SpotLight->AttenuationRadius = 1050.0f;
	SpotLight->InnerConeAngle = 18.7f;
	SpotLight->OuterConeAngle = 45.24f;
}

void AHorrorCharacter::BeginPlay()
{
	Super::BeginPlay();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (SprayToolClass)
	{
		SprayTool = GetWorld()->SpawnActor<ASprayTool>(SprayToolClass, GetActorTransform(), SpawnParameters);
	}
	if (WiperToolClass)
	{
		WiperTool = GetWorld()->SpawnActor<AWiperTool>(WiperToolClass, GetActorTransform(), SpawnParameters);
	}
	if (!SprayTool || !WiperTool)
	{
		UE_LOG(LogWindowCleaner, Error, TEXT("%s could not create both player tools; check their classes"), *GetName());
	}
	if (SprayTool)
	{
		SprayTool->bIsInteractable = false;
		SprayTool->OnUnequipped();
	}
	if (WiperTool)
	{
		WiperTool->bIsInteractable = false;
		WiperTool->OnUnequipped();
	}
	SelectSprayTool();

	// initialize sprint meter to max
	SprintMeter = SprintTime;

	// Initialize the walk speed
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// start the sprint tick timer
	GetWorld()->GetTimerManager().SetTimer(SprintTimer, this, &AHorrorCharacter::SprintFixedTick, SprintFixedTickTime, true);
}

void AHorrorCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	RemoveToolMappingContext();
	if (IsValid(SprayTool))
	{
		SprayTool->Destroy();
	}
	if (IsValid(WiperTool))
	{
		WiperTool->Destroy();
	}

	Super::EndPlay(EndPlayReason);

	// clear the sprint timer
	GetWorld()->GetTimerManager().ClearTimer(SprintTimer);
}

void AHorrorCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	{
		// Set up action bindings
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
		{
			// Sprinting
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AHorrorCharacter::DoStartSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AHorrorCharacter::DoEndSprint);
			EnhancedInputComponent->BindAction(SelectSprayAction, ETriggerEvent::Started, this, &AHorrorCharacter::SelectSprayTool);
			EnhancedInputComponent->BindAction(SelectWiperAction, ETriggerEvent::Started, this, &AHorrorCharacter::SelectWiperTool);
			EnhancedInputComponent->BindAction(UseToolAction, ETriggerEvent::Started, this, &AHorrorCharacter::UseSelectedTool);

		}
	}
}

void AHorrorCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	RemoveToolMappingContext();
	const APlayerController* PC = Cast<APlayerController>(GetController());
	UEnhancedInputLocalPlayerSubsystem* Subsystem = PC && PC->GetLocalPlayer()
		? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()) : nullptr;
	if (!Subsystem)
	{
		if (PC && PC->IsLocalController() && PC->GetLocalPlayer())
		{
			UE_LOG(LogWindowCleaner, Error, TEXT("%s cannot register tool inputs: Enhanced Input subsystem is unavailable"), *GetName());
		}
		return;
	}

	CreateToolMappingContext();
	Subsystem->AddMappingContext(ToolMappingContext, 0);
	ToolInputSubsystem = Subsystem;
}

void AHorrorCharacter::CreateToolMappingContext()
{
	if (!ToolMappingContext)
	{
		ToolMappingContext = NewObject<UInputMappingContext>(this);
		ToolMappingContext->MapKey(SelectSprayAction, SelectSprayKey);
		ToolMappingContext->MapKey(SelectWiperAction, SelectWiperKey);
		ToolMappingContext->MapKey(UseToolAction, UseToolKey);
	}
}

void AHorrorCharacter::UnPossessed()
{
	RemoveToolMappingContext();
	Super::UnPossessed();
}

void AHorrorCharacter::RemoveToolMappingContext()
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ToolInputSubsystem.Get())
	{
		Subsystem->RemoveMappingContext(ToolMappingContext);
	}
	ToolInputSubsystem.Reset();
}

void AHorrorCharacter::SelectTool(AATool* Tool)
{
	if (!IsValid(Tool) || SelectedTool == Tool)
	{
		return;
	}

	Tool->OnEquipped(this);
	if (!Tool->bIsEquipped)
	{
		return;
	}
	if (IsValid(SelectedTool))
	{
		SelectedTool->OnUnequipped();
	}
	SelectedTool = Tool;
}

void AHorrorCharacter::SelectSprayTool()
{
	SelectTool(SprayTool);
}

void AHorrorCharacter::SelectWiperTool()
{
	SelectTool(WiperTool);
}

void AHorrorCharacter::UseSelectedTool()
{
	if (IsValid(SelectedTool) && SelectedTool->bIsEquipped)
	{
		SelectedTool->OnUsed();
	}
}

void AHorrorCharacter::DoStartSprint()
{
	// set the sprinting flag
	bSprinting = true;

	// are we out of recovery mode?
	if (!bRecovering)
	{
		// set the sprint walk speed
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;

		// call the sprint state changed delegate
		OnSprintStateChanged.Broadcast(true);
	}

}

void AHorrorCharacter::DoEndSprint()
{
	// set the sprinting flag
	bSprinting = false;

	// are we out of recovery mode?
	if (!bRecovering)
	{
		// set the default walk speed
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

		// call the sprint state changed delegate
		OnSprintStateChanged.Broadcast(false);
	}
}

void AHorrorCharacter::SprintFixedTick()
{
	// are we out of recovery, still have stamina and are moving faster than our walk speed?
	if (bSprinting && !bRecovering && GetVelocity().Length() > WalkSpeed)
	{

		// do we still have meter to burn?
		if (SprintMeter > 0.0f)
		{
			// update the sprint meter
			SprintMeter = FMath::Max(SprintMeter - SprintFixedTickTime, 0.0f);

			// have we run out of stamina?
			if (SprintMeter <= 0.0f)
			{
				// raise the recovering flag
				bRecovering = true;

				// set the recovering walk speed
				GetCharacterMovement()->MaxWalkSpeed = RecoveringWalkSpeed;
			}
		}
		
	} else {

		// recover stamina
		SprintMeter = FMath::Min(SprintMeter + SprintFixedTickTime, SprintTime);

		if (SprintMeter >= SprintTime)
		{
			// lower the recovering flag
			bRecovering = false;

			// set the walk or sprint speed depending on whether the sprint button is down
			GetCharacterMovement()->MaxWalkSpeed = bSprinting ? SprintSpeed : WalkSpeed;

			// update the sprint state depending on whether the button is down or not
			OnSprintStateChanged.Broadcast(bSprinting);
		}

	}

	// broadcast the sprint meter updated delegate
	OnSprintMeterUpdated.Broadcast(SprintMeter / SprintTime);

}
