#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractionPrompt.generated.h"

class UTextBlock;

UCLASS()
class WINDOWCLEANER_API UInteractionPrompt : public UUserWidget
{
	GENERATED_BODY()

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PromptText;

public:
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetPromptText(const FText& NewText);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Show(bool bShow);
};
