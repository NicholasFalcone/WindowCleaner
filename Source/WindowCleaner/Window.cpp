#include "Window.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Variant_Horror/HorrorCharacter.h"
#include "WindowCleaner.h"

AWindow::AWindow()
{
	CleaningSurface = CreateDefaultSubobject<USceneComponent>(TEXT("CleaningSurface"));
	CleaningSurface->SetupAttachment(RootComponent);

	InteractionSurface = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionSurface"));
	InteractionSurface->SetupAttachment(CleaningSurface);
	InteractionSurface->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSurface->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSurface->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	CleaningCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CleaningCamera"));
	CleaningCamera->SetupAttachment(CleaningSurface);
	CleaningCamera->SetFieldOfView(60.0f);

	DirtCells = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("DirtCells"));
	DirtCells->SetupAttachment(CleaningSurface);
	DirtCells->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DirtCells->SetCastShadow(false);

	SoapCells = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SoapCells"));
	SoapCells->SetupAttachment(CleaningSurface);
	SoapCells->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SoapCells->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		MeshComp->SetStaticMesh(Cube.Object);
		DirtCells->SetStaticMesh(Cube.Object);
		SoapCells->SetStaticMesh(Cube.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/WindowCleaning/M_WindowState.M_WindowState"));
	if (Material.Succeeded())
	{
		StateMaterial = Material.Object;
	}
}

void AWindow::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	CleaningSurface->SetRelativeLocation(FVector(SurfaceOffset, 0.0, 0.0));
	InteractionSurface->SetBoxExtent(FVector(1.0, FMath::Max(SurfaceWidth, 1.0f) * 0.5, FMath::Max(SurfaceHeight, 1.0f) * 0.5));
	CleaningCamera->SetRelativeLocation(FVector(-FMath::Max(CameraDistance, 1.0f), 0.0, 0.0));
	CleaningCamera->SetRelativeRotation(FRotator::ZeroRotator);
}

void AWindow::BeginPlay()
{
	Super::BeginPlay();
	InitializeSurface();
}

void AWindow::InitializeSurface()
{
	if (!StateMaterial || !DirtCells->GetStaticMesh() || !SoapCells->GetStaticMesh())
	{
		UE_LOG(LogWindowCleaner, Error, TEXT("%s cannot display cleaning surface: mesh or state material is missing"), *GetName());
		return;
	}

	UMaterialInstanceDynamic* DirtMaterial = DirtCells->CreateDynamicMaterialInstance(0, StateMaterial);
	UMaterialInstanceDynamic* SoapMaterial = SoapCells->CreateDynamicMaterialInstance(0, StateMaterial);
	if (!DirtMaterial || !SoapMaterial)
	{
		UE_LOG(LogWindowCleaner, Error, TEXT("%s cannot create cleaning materials"), *GetName());
		return;
	}
	DirtMaterial->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.18f, 0.10f, 0.04f));
	SoapMaterial->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.65f, 0.9f, 1.0f));

	const float Width = FMath::Max(SurfaceWidth, 1.0f);
	const float Height = FMath::Max(SurfaceHeight, 1.0f);
	const float CellWidth = Width / GridSize;
	const float CellHeight = Height / GridSize;
	Cells.Init(ECellState::Dirty, GridSize * GridSize);
	CellTransforms.Reserve(Cells.Num());
	for (int32 Row = 0; Row < GridSize; ++Row)
	{
		for (int32 Column = 0; Column < GridSize; ++Column)
		{
			const FVector Position(-0.5, -Width * 0.5 + (Column + 0.5) * CellWidth, -Height * 0.5 + (Row + 0.5) * CellHeight);
			const FTransform Visible(FRotator::ZeroRotator, Position, FVector(0.01, CellWidth / 100.0, CellHeight / 100.0));
			FTransform Hidden = Visible;
			Hidden.SetScale3D(FVector::ZeroVector);
			CellTransforms.Add(Visible);
			DirtCells->AddInstance(Visible);
			SoapCells->AddInstance(Hidden);
		}
	}
}

float AWindow::GetCleaningProgress() const
{
	if (CleaningPhase == EWindowCleaningPhase::Complete)
	{
		return 1.0f;
	}
	return Cells.IsEmpty() ? 0.0f : static_cast<float>(CleaningPhase == EWindowCleaningPhase::Soaping ? SoapedCount : CleanCount) / Cells.Num();
}

bool AWindow::ProjectRayToSurface(const FVector& Origin, const FVector& Direction, FVector2D& OutPosition) const
{
	const FTransform SurfaceTransform = CleaningSurface->GetComponentTransform();
	const FVector LocalOrigin = SurfaceTransform.InverseTransformPosition(Origin);
	const FVector LocalDirection = SurfaceTransform.InverseTransformVector(Direction);
	if (FMath::IsNearlyZero(LocalDirection.X))
	{
		return false;
	}
	const double Distance = -LocalOrigin.X / LocalDirection.X;
	if (Distance < 0.0)
	{
		return false;
	}
	const FVector Hit = LocalOrigin + LocalDirection * Distance;
	if (FMath::Abs(Hit.Y) > SurfaceWidth * 0.5 || FMath::Abs(Hit.Z) > SurfaceHeight * 0.5)
	{
		return false;
	}
	OutPosition = FVector2D(Hit.Y, Hit.Z);
	return true;
}

void AWindow::ApplyBrush(const FVector2D& Start, const FVector2D& End, bool bSpray)
{
	if (Cells.IsEmpty() || CleaningPhase == EWindowCleaningPhase::Complete
		|| (bSpray != (CleaningPhase == EWindowCleaningPhase::Soaping)))
	{
		return;
	}

	const float Width = FMath::Max(SurfaceWidth, 1.0f);
	const float Height = FMath::Max(SurfaceHeight, 1.0f);
	const float Radius = FMath::Max(BrushRadius, 1.0f);
	const float Step = FMath::Min(Width, Height) / (GridSize * 2.0f);
	const int32 Steps = FMath::Max(1, FMath::CeilToInt(FVector2D::Distance(Start, End) / Step));
	bool bChanged = false;
	for (int32 Sample = 0; Sample <= Steps; ++Sample)
	{
		const FVector2D Point = FMath::Lerp(Start, End, static_cast<double>(Sample) / Steps);
		for (int32 Index = 0; Index < Cells.Num(); ++Index)
		{
			const ECellState Required = bSpray ? ECellState::Dirty : ECellState::Soaped;
			if (Cells[Index] != Required)
			{
				continue;
			}
			const FVector Center = CellTransforms[Index].GetLocation();
			if (FVector2D::DistSquared(Point, FVector2D(Center.Y, Center.Z)) > FMath::Square(Radius))
			{
				continue;
			}
			FTransform Hidden = CellTransforms[Index];
			Hidden.SetScale3D(FVector::ZeroVector);
			if (bSpray)
			{
				Cells[Index] = ECellState::Soaped;
				++SoapedCount;
				DirtCells->UpdateInstanceTransform(Index, Hidden, false, false);
				SoapCells->UpdateInstanceTransform(Index, CellTransforms[Index], false, false);
			}
			else
			{
				Cells[Index] = ECellState::Clean;
				++CleanCount;
				SoapCells->UpdateInstanceTransform(Index, Hidden, false, false);
			}
			bChanged = true;
		}
	}
	if (!bChanged)
	{
		return;
	}

	DirtCells->MarkRenderStateDirty();
	SoapCells->MarkRenderStateDirty();
	if (SoapedCount == Cells.Num() && CleaningPhase == EWindowCleaningPhase::Soaping)
	{
		CleaningPhase = EWindowCleaningPhase::Wiping;
	}
	else if (CleanCount == Cells.Num())
	{
		CleaningPhase = EWindowCleaningPhase::Complete;
	}
	OnCleaningProgress.Broadcast(CleaningPhase, GetCleaningProgress());
	if (CleaningPhase == EWindowCleaningPhase::Complete)
	{
		OnCleaningComplete.Broadcast();
	}
}

bool AWindow::BeginCleaning(AHorrorCharacter* Character)
{
	if (!IsValid(Character) || !bIsInteractable || Cells.IsEmpty() || ActiveCleaner.IsValid())
	{
		return false;
	}
	ActiveCleaner = Character;
	OnInteract();
	return true;
}

void AWindow::EndCleaning(AHorrorCharacter* Character)
{
	if (ActiveCleaner.Get() == Character)
	{
		ActiveCleaner.Reset();
		OnEndInteract();
	}
}

void AWindow::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AHorrorCharacter* Character = ActiveCleaner.Get())
	{
		Character->ExitWindowCleaning();
	}
	Super::EndPlay(EndPlayReason);
}
