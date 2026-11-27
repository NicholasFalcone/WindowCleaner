#include "WindowCleaningHUD.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Window.h"

void UWindowCleaningHUD::SetWindow(AWindow* InWindow)
{
	Window = InWindow;
}

FText UWindowCleaningHUD::GetStatusText() const
{
	const AWindow* Current = Window.Get();
	if (!Current)
	{
		return FText::GetEmpty();
	}

	const int32 Percent = FMath::FloorToInt(Current->GetCleaningProgress() * 100.0f);
	switch (Current->GetCleaningPhase())
	{
	case EWindowCleaningPhase::Soaping:
		return FText::Format(NSLOCTEXT("WindowCleaning", "SoapProgress", "Insapona tutta la finestra: {0}%"), FText::AsNumber(Percent));
	case EWindowCleaningPhase::Wiping:
		return FText::Format(NSLOCTEXT("WindowCleaning", "WipeProgress", "Rimuovi tutto il sapone: {0}%"), FText::AsNumber(Percent));
	case EWindowCleaningPhase::Complete:
		return NSLOCTEXT("WindowCleaning", "Complete", "Finestra pulita!");
	default:
		return FText::GetEmpty();
	}
}

TSharedRef<SWidget> UWindowCleaningHUD::RebuildWidget()
{
	return SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(20.0f)
		[
			SNew(SBorder)
			.Padding(12.0f)
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 20))
				.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateUObject(this, &UWindowCleaningHUD::GetStatusText)))
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(20.0f)
		[
			SNew(SBorder)
			.Padding(12.0f)
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
				.Text(NSLOCTEXT("WindowCleaning", "Controls", "1: Spruzzino   2: Wiper   Trascina: usa   E: esci"))
			]
		];
}
