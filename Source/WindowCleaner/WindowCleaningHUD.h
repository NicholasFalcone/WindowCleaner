#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WindowCleaningHUD.generated.h"

class AWindow;

UCLASS()
class WINDOWCLEANER_API UWindowCleaningHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetWindow(AWindow* InWindow);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TWeakObjectPtr<AWindow> Window;
	FText GetStatusText() const;
};
