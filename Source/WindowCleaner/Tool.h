#pragma once

#include "CoreMinimal.h"
#include "BaseInteractable.h"
#include "Tool.generated.h"

class ACharacter;
class UAudioComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnToolUsed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnToolEquipped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnToolDropped);

UCLASS()
class WINDOWCLEANER_API AATool : public ABaseInteractable
{
	GENERATED_BODY()

public:
	AATool();

	UPROPERTY(BlueprintAssignable, Category = "Tool")
	FOnToolUsed OnToolUsed;

	UPROPERTY(BlueprintAssignable, Category = "Tool")
	FOnToolEquipped OnToolEquipped;

	UPROPERTY(BlueprintAssignable, Category = "Tool")
	FOnToolDropped OnToolDropped;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tool")
	bool bIsEquipped = false;

	UFUNCTION(BlueprintCallable, Category = "Tool")
	virtual void OnEquipped(ACharacter* InOwnerCharacter);

	UFUNCTION(BlueprintCallable, Category = "Tool")
	virtual void OnUnequipped();

	UFUNCTION(BlueprintCallable, Category = "Tool")
	virtual void OnUsed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Tool")
	void CallOnToolUsed();

	UFUNCTION(BlueprintCallable, Category = "Tool")
	virtual void OnDropped(ACharacter* InDroppedByCharacter);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool")
	FVector SocketOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool")
	FRotator SocketRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool")
	FName AttachSocketName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAudioComponent> OnUsedAudioComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAudioComponent> OnEquippedAudioComp;
};
