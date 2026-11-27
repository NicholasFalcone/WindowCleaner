#include "BaseInteractable.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InteractionPrompt.h"

ABaseInteractable::ABaseInteractable()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetMobility(EComponentMobility::Movable);
	RootComponent = MeshComp;

	InteractionWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionWidgetComp"));
	InteractionWidgetComp->SetupAttachment(RootComponent);
	InteractionWidgetComp->SetWidgetSpace(EWidgetSpace::World);
	InteractionWidgetComp->SetDrawSize(FVector2D(200.0f, 50.0f));
	InteractionWidgetComp->SetVisibility(false);

	AudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComp"));
	AudioComp->SetupAttachment(RootComponent);
}

void ABaseInteractable::BeginPlay()
{
	Super::BeginPlay();

	InteractionWidgetComp->SetRelativeLocation(InteractionPromptData.InteractionPromptOffset);
	if (InteractionPromptClass)
	{
		InteractionWidgetComp->SetWidgetClass(InteractionPromptClass);
		InteractionWidgetComp->InitWidget();
		if (UInteractionPrompt* PromptWidget = Cast<UInteractionPrompt>(InteractionWidgetComp->GetUserWidgetObject()))
		{
			PromptWidget->SetPromptText(InteractionPromptData.InteractionPrompt);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Could not initialize interaction prompt for %s"), *GetName());
		}
	}
}

void ABaseInteractable::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (InteractionWidgetComp->IsVisible())
	{
		const APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (PC && PC->PlayerCameraManager)
		{
			const FVector Direction = PC->PlayerCameraManager->GetCameraLocation() - InteractionWidgetComp->GetComponentLocation();
			InteractionWidgetComp->SetWorldRotation(Direction.Rotation());
		}
	}
}

void ABaseInteractable::OnHighlight()
{
	if (!bIsInteractable)
	{
		return;
	}

	InteractionWidgetComp->SetVisibility(InteractionPromptData.bShowInteractionPrompt && InteractionPromptClass != nullptr);
	if (bUseOutlineMaterial && OutlineOverlayMaterial)
	{
		MeshComp->SetOverlayMaterial(OutlineOverlayMaterial);
	}
}

void ABaseInteractable::OnUnhighlight()
{
	InteractionWidgetComp->SetVisibility(false);
	if (bUseOutlineMaterial && OutlineOverlayMaterial)
	{
		MeshComp->SetOverlayMaterial(nullptr);
	}
}

void ABaseInteractable::OnInteract()
{
	if (!bIsInteractable)
	{
		return;
	}

	OnUnhighlight();
	OnInteractDelegate.Broadcast();
	CallOnInteraction();
}

void ABaseInteractable::OnEndInteract()
{
	CallOnEndInteraction();
}

void ABaseInteractable::ChangeInteractablePromptText(const FText& NewText)
{
	InteractionPromptData.InteractionPrompt = NewText;
	if (UInteractionPrompt* PromptWidget = Cast<UInteractionPrompt>(InteractionWidgetComp->GetUserWidgetObject()))
	{
		PromptWidget->SetPromptText(NewText);
	}
}
