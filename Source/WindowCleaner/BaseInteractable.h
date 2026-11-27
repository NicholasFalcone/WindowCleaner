#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractionPromptData.h"
#include "BaseInteractable.generated.h"

class UAudioComponent;
class UInteractionPrompt;
class UStaticMeshComponent;
class UWidgetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractDelegate);

UCLASS()
class WINDOWCLEANER_API ABaseInteractable : public AActor
{
	GENERATED_BODY()

public:
	ABaseInteractable();

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractDelegate OnInteractDelegate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAudioComponent> AudioComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> InteractionWidgetComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	TObjectPtr<UMaterialInterface> OutlineOverlayMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Highlight")
	bool bUseOutlineMaterial = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prompt Data")
	FInteractionPromptData InteractionPromptData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prompt Data")
	TSubclassOf<UInteractionPrompt> InteractionPromptClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bIsInteractable = true;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void OnHighlight();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void OnUnhighlight();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void OnInteract();

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void CallOnInteraction();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void OnEndInteract();

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void CallOnEndInteraction();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void ChangeInteractablePromptText(const FText& NewText);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
};
