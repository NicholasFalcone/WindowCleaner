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
#include "Blueprint/UserWidget.h"
#include "SprayTool.h"
#include "WiperTool.h"
#include "Window.h"
#include "WindowCleaningHUD.h"
#include "WindowCleaner.h"

AHorrorCharacter::AHorrorCharacter()
{
	SprayToolClass = ASprayTool::StaticClass();
	WiperToolClass = AWiperTool::StaticClass();
	SelectSprayAction = CreateDefaultSubobject<UInputAction>(TEXT("SelectSprayAction"));
	SelectWiperAction = CreateDefaultSubobject<UInputAction>(TEXT("SelectWiperAction"));
	UseToolAction = CreateDefaultSubobject<UInputAction>(TEXT("UseToolAction"));
	InteractAction = CreateDefaultSubobject<UInputAction>(TEXT("InteractAction"));

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
	ExitWindowCleaning();
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
			EnhancedInputComponent->BindAction(UseToolAction, ETriggerEvent::Completed, this, &AHorrorCharacter::ReleaseSelectedTool);
			EnhancedInputComponent->BindAction(UseToolAction, ETriggerEvent::Canceled, this, &AHorrorCharacter::ReleaseSelectedTool);
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AHorrorCharacter::InteractWithWindow);

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
		ToolMappingContext->MapKey(InteractAction, InteractKey);
	}
}

void AHorrorCharacter::UnPossessed()
{
	ExitWindowCleaning();
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
	bHasPreviousBrushPoint = false;
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
		if (IsValid(ActiveWindow))
		{
			bToolHeld = true;
			bHasPreviousBrushPoint = false;
			UpdateWindowBrush();
		}
	}
}

void AHorrorCharacter::ReleaseSelectedTool()
{
	bToolHeld = false;
	bHasPreviousBrushPoint = false;
}

void AHorrorCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bToolHeld && IsValid(ActiveWindow))
	{
		UpdateWindowBrush();
	}
}

void AHorrorCharacter::UpdateWindowBrush()
{
	APlayerController* PC = CleaningController.Get();
	if (!PC || !IsValid(ActiveWindow) || !IsValid(SelectedTool) || !SelectedTool->bIsEquipped)
	{
		bHasPreviousBrushPoint = false;
		return;
	}
	FVector Origin;
	FVector Direction;
	FVector2D Point;
	if (!PC->DeprojectMousePositionToWorld(Origin, Direction) || !ActiveWindow->ProjectRayToSurface(Origin, Direction, Point))
	{
		bHasPreviousBrushPoint = false;
		return;
	}
	const bool bSpray = SelectedTool == SprayTool;
	if (bSpray || SelectedTool == WiperTool)
	{
		ActiveWindow->ApplyBrush(bHasPreviousBrushPoint ? PreviousBrushPoint : Point, Point, bSpray);
		PreviousBrushPoint = Point;
		bHasPreviousBrushPoint = true;
	}
}

void AHorrorCharacter::InteractWithWindow()
{
	if (IsValid(ActiveWindow))
	{
		ExitWindowCleaning();
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	UCameraComponent* Camera = GetFirstPersonCameraComponent();
	if (!PC || !PC->IsLocalController() || !Camera || !GetWorld())
	{
		return;
	}
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(WindowInteraction), false, this);
	if (IsValid(SprayTool))
	{
		Params.AddIgnoredActor(SprayTool);
	}
	if (IsValid(WiperTool))
	{
		Params.AddIgnoredActor(WiperTool);
	}
	const FVector Start = Camera->GetComponentLocation();
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Camera->GetForwardVector() * InteractionDistance, ECC_Visibility, Params))
	{
		return;
	}
	AWindow* Window = Cast<AWindow>(Hit.GetActor());
	if (!IsValid(Window) || !Window->BeginCleaning(this))
	{
		return;
	}
	if (!IsValid(Window))
	{
		return;
	}

	ActiveWindow = Window;
	CleaningController = PC;
	PreviousViewTarget = PC->GetViewTarget();
	bPreviousShowMouseCursor = PC->bShowMouseCursor;
	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);
	GetCharacterMovement()->StopMovementImmediately();
	PC->SetViewTargetWithBlend(Window, 0.2f);
	PC->bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);

	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PC->GetViewportSize(ViewportWidth, ViewportHeight);
	if (ViewportWidth > 0 && ViewportHeight > 0)
	{
		PC->SetMouseLocation(ViewportWidth / 2, ViewportHeight / 2);
	}

	CleaningHUD = CreateWidget<UWindowCleaningHUD>(PC, UWindowCleaningHUD::StaticClass());
	if (CleaningHUD)
	{
		CleaningHUD->SetWindow(Window);
		CleaningHUD->SetVisibility(ESlateVisibility::HitTestInvisible);
		CleaningHUD->AddToPlayerScreen();
	}
	else
	{
		UE_LOG(LogWindowCleaner, Error, TEXT("Could not create window cleaning HUD for %s"), *GetName());
	}
}

bool AHorrorCharacter::IsCleaningWindow() const
{
	return IsValid(ActiveWindow);
}

void AHorrorCharacter::ExitWindowCleaning()
{
	ReleaseSelectedTool();
	if (CleaningHUD)
	{
		CleaningHUD->RemoveFromParent();
		CleaningHUD = nullptr;
	}
	if (APlayerController* PC = CleaningController.Get())
	{
		PC->SetViewTargetWithBlend(PreviousViewTarget.IsValid() ? PreviousViewTarget.Get() : this, 0.2f);
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
		PC->bShowMouseCursor = bPreviousShowMouseCursor;
		PC->SetInputMode(FInputModeGameOnly());
	}
	CleaningController.Reset();
	PreviousViewTarget.Reset();
	if (IsValid(ActiveWindow))
	{
		ActiveWindow->EndCleaning(this);
	}
	ActiveWindow = nullptr;
}

void AHorrorCharacter::DoJumpStart()
{
	if (!IsValid(ActiveWindow))
	{
		Super::DoJumpStart();
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
