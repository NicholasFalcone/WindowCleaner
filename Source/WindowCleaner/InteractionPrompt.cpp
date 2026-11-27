#include "InteractionPrompt.h"
#include "Components/TextBlock.h"

void UInteractionPrompt::SetPromptText(const FText& NewText)
{
	if (PromptText)
	{
		PromptText->SetText(NewText);
	}
}

void UInteractionPrompt::Show(bool bShow)
{
	SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}
