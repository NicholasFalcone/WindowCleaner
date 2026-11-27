#include "Tool.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "WindowCleanerCharacter.h"

AATool::AATool()
{
	bIsInteractable = false;

	OnUsedAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("OnUsedAudioComp"));
	OnUsedAudioComp->SetupAttachment(RootComponent);

	OnEquippedAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("OnEquippedAudioComp"));
	OnEquippedAudioComp->SetupAttachment(RootComponent);
}

void AATool::OnEquipped(ACharacter* InOwnerCharacter)
{
	AWindowCleanerCharacter* Character = Cast<AWindowCleanerCharacter>(InOwnerCharacter);
	if (!IsValid(Character) || !Character->GetFirstPersonCameraComponent())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot equip %s: a WindowCleaner character with a camera is required"), *GetName());
		return;
	}

	USceneComponent* Parent = Character->GetFirstPersonCameraComponent();
	if (!AttachSocketName.IsNone() && !Parent->DoesSocketExist(AttachSocketName))
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot equip %s: camera socket %s does not exist"), *GetName(), *AttachSocketName.ToString());
		return;
	}

	const bool bWasSimulatingPhysics = MeshComp->IsSimulatingPhysics();
	MeshComp->SetSimulatePhysics(false);
	if (!AttachToComponent(Parent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocketName))
	{
		MeshComp->SetSimulatePhysics(bWasSimulatingPhysics);
		UE_LOG(LogTemp, Warning, TEXT("Cannot attach tool %s"), *GetName());
		return;
	}

	bIsEquipped = true;
	SetOwner(Character);
	SetActorHiddenInGame(false);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetVisibility(true, true);
	SetActorRelativeLocation(SocketOffset);
	SetActorRelativeRotation(SocketRotation);
	OnUnhighlight();
	OnToolEquipped.Broadcast();
}

void AATool::OnUnequipped()
{
	bIsEquipped = false;
	SetActorHiddenInGame(true);
	MeshComp->SetSimulatePhysics(false);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetVisibility(false, true);
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}

void AATool::OnUsed()
{
	OnToolUsed.Broadcast();
	CallOnToolUsed();
}

void AATool::OnDropped(ACharacter* InDroppedByCharacter)
{
	bIsEquipped = false;
	SetOwner(nullptr);
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorHiddenInGame(false);

	if (IsValid(InDroppedByCharacter))
	{
		FVector DropLocation = InDroppedByCharacter->GetActorLocation() + InDroppedByCharacter->GetActorForwardVector() * 80.0f;
		DropLocation.Z += 10.0f;
		SetActorLocationAndRotation(DropLocation, InDroppedByCharacter->GetActorRotation());
	}

	MeshComp->SetVisibility(true, true);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComp->SetSimulatePhysics(true);
	OnToolDropped.Broadcast();
}
