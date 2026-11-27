#pragma once

#include "CoreMinimal.h"
#include "InteractionPromptData.generated.h"

USTRUCT(BlueprintType)
struct WINDOWCLEANER_API FInteractionPromptData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prompt Data")
	bool bShowInteractionPrompt = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prompt Data")
	FVector InteractionPromptOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prompt Data")
	FText InteractionPrompt = NSLOCTEXT("Interaction", "DefaultPrompt", "Press E to interact");
};
