#pragma once

#include "CoreMinimal.h"
#include "BaseInteractable.h"
#include "Window.generated.h"

class AHorrorCharacter;
class UBoxComponent;
class UCameraComponent;
class UInstancedStaticMeshComponent;
class USceneComponent;

UENUM(BlueprintType)
enum class EWindowCleaningPhase : uint8
{
	Soaping,
	Wiping,
	Complete
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWindowCleaningProgress, EWindowCleaningPhase, Phase, float, Progress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWindowCleaningComplete);

UCLASS()
class WINDOWCLEANER_API AWindow : public ABaseInteractable
{
	GENERATED_BODY()

public:
	AWindow();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Window|Components")
	TObjectPtr<USceneComponent> CleaningSurface;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Window|Components")
	TObjectPtr<UBoxComponent> InteractionSurface;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Window|Components")
	TObjectPtr<UCameraComponent> CleaningCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Window|Surface", meta = (ClampMin = "1.0", Units = "cm"))
	float SurfaceWidth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Window|Surface", meta = (ClampMin = "1.0", Units = "cm"))
	float SurfaceHeight = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Window|Surface", meta = (Units = "cm"))
	float SurfaceOffset = -51.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Window|Surface", meta = (ClampMin = "1.0", Units = "cm"))
	float BrushRadius = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Window|Camera", meta = (ClampMin = "1.0", Units = "cm"))
	float CameraDistance = 180.0f;

	UPROPERTY(BlueprintAssignable, Category = "Window|Cleaning")
	FWindowCleaningProgress OnCleaningProgress;

	UPROPERTY(BlueprintAssignable, Category = "Window|Cleaning")
	FWindowCleaningComplete OnCleaningComplete;

	UFUNCTION(BlueprintPure, Category = "Window|Cleaning")
	EWindowCleaningPhase GetCleaningPhase() const { return CleaningPhase; }

	UFUNCTION(BlueprintPure, Category = "Window|Cleaning")
	float GetCleaningProgress() const;

	bool ProjectRayToSurface(const FVector& Origin, const FVector& Direction, FVector2D& OutPosition) const;
	void ApplyBrush(const FVector2D& Start, const FVector2D& End, bool bSpray);
	bool BeginCleaning(AHorrorCharacter* Character);
	void EndCleaning(AHorrorCharacter* Character);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	static constexpr int32 GridSize = 16;
	enum class ECellState : uint8 { Dirty, Soaped, Clean };

	UPROPERTY(VisibleAnywhere, Category = "Window|Components")
	TObjectPtr<UInstancedStaticMeshComponent> DirtCells;

	UPROPERTY(VisibleAnywhere, Category = "Window|Components")
	TObjectPtr<UInstancedStaticMeshComponent> SoapCells;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> StateMaterial;

	TArray<ECellState> Cells;
	TArray<FTransform> CellTransforms;
	TWeakObjectPtr<AHorrorCharacter> ActiveCleaner;
	EWindowCleaningPhase CleaningPhase = EWindowCleaningPhase::Soaping;
	int32 SoapedCount = 0;
	int32 CleanCount = 0;

	void InitializeSurface();
};
