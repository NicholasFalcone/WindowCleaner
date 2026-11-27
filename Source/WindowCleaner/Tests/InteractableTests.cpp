#if WITH_DEV_AUTOMATION_TESTS

#include "BaseInteractable.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystemInterface.h"
#include "EnhancedPlayerInput.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "Misc/AutomationTest.h"
#include "SprayTool.h"
#include "Tool.h"
#include "Variant_Horror/HorrorCharacter.h"
#include "Window.h"
#include "WiperTool.h"

class FTestInputSubsystem : public IEnhancedInputSubsystemInterface
{
public:
	explicit FTestInputSubsystem(UEnhancedPlayerInput* InPlayerInput) : PlayerInput(InPlayerInput) {}
	virtual UEnhancedPlayerInput* GetPlayerInput() const override { return PlayerInput; }

protected:
	virtual TMap<TObjectPtr<const UInputAction>, FInjectedInput>& GetContinuouslyInjectedInputs() override
	{
		return InjectedInputs;
	}

private:
	UEnhancedPlayerInput* PlayerInput;
	TMap<TObjectPtr<const UInputAction>, FInjectedInput> InjectedInputs;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWindowInteractableTest, "WindowCleaner.Interaction.Base",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWindowInteractableTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}

	AWindow* Window = World->SpawnActor<AWindow>();
	AATool* Tool = World->SpawnActor<AATool>();
	if (TestNotNull(TEXT("Window"), Window) && TestNotNull(TEXT("Tool"), Tool))
	{
		TestTrue(TEXT("Window is interactable"), Window->IsA<ABaseInteractable>());
		TestTrue(TEXT("Tool is interactable"), Tool->IsA<ABaseInteractable>());
		TestFalse(TEXT("Tool starts unequipped"), Tool->bIsEquipped);

		const FText NewText = FText::FromString(TEXT("Clean"));
		Window->ChangeInteractablePromptText(NewText);
		TestTrue(TEXT("Prompt text is retained before widget initialization"),
			Window->InteractionPromptData.InteractionPrompt.EqualTo(NewText));

		Window->InteractionWidgetComp->SetVisibility(true);
		Window->bIsInteractable = false;
		Window->OnInteract();
		TestTrue(TEXT("Disabled interaction does not hide the prompt"), Window->InteractionWidgetComp->IsVisible());
		Window->OnUnhighlight();
		Window->OnHighlight();
		TestFalse(TEXT("Disabled interaction cannot highlight"), Window->InteractionWidgetComp->IsVisible());

		Window->bIsInteractable = true;
		Window->InteractionWidgetComp->SetVisibility(true);
		Window->OnInteract();
		TestFalse(TEXT("Interaction clears the highlight"), Window->InteractionWidgetComp->IsVisible());
	}

	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerToolsTest, "WindowCleaner.Tools.Selection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerToolsTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}

	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	UClass* CharacterClass = LoadClass<AHorrorCharacter>(nullptr,
		TEXT("/Game/Variant_Horror/Blueprints/BP_HorrorCharacter.BP_HorrorCharacter_C"));
	AHorrorCharacter* Character = CharacterClass ? World->SpawnActor<AHorrorCharacter>(CharacterClass) : nullptr;
	if (TestNotNull(TEXT("Horror player Blueprint"), Character))
	{
		Character->DispatchBeginPlay();
		ASprayTool* Spray = Character->GetSprayTool();
		AWiperTool* Wiper = Character->GetWiperTool();
		if (TestNotNull(TEXT("Spray exists without pickup"), Spray)
			&& TestNotNull(TEXT("Wiper exists without pickup"), Wiper))
		{
			TestTrue(TEXT("Spray selected initially"), Character->GetSelectedTool() == Spray && Spray->bIsEquipped);
			TestTrue(TEXT("Wiper starts hidden"), Wiper->IsHidden() && !Wiper->bIsEquipped);
			TestTrue(TEXT("Hidden tool has no collision"), Wiper->MeshComp->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
			TestFalse(TEXT("Player tools are not interactable pickups"), Spray->bIsInteractable || Wiper->bIsInteractable);
			Character->SelectWiperTool();
			TestTrue(TEXT("Wiper selected"), Character->GetSelectedTool() == Wiper && Wiper->bIsEquipped && !Wiper->IsHidden());
			TestTrue(TEXT("Spray hidden when switched"), !Spray->bIsEquipped && Spray->IsHidden());
			Character->SelectWiperTool();
			TestTrue(TEXT("Selecting the current tool keeps it equipped"), Wiper->bIsEquipped);
			Character->SelectSprayTool();
			TestTrue(TEXT("Spray can be selected again"), Character->GetSelectedTool() == Spray && Spray->bIsEquipped && Wiper->IsHidden());

			ABaseInteractable* SprayMarker = World->SpawnActor<ABaseInteractable>();
			ABaseInteractable* WiperMarker = World->SpawnActor<ABaseInteractable>();
			APlayerController* Controller = World->SpawnActor<APlayerController>();
			if (TestNotNull(TEXT("Spray event marker"), SprayMarker)
				&& TestNotNull(TEXT("Wiper event marker"), WiperMarker)
				&& TestNotNull(TEXT("Input controller"), Controller))
			{
				Spray->OnToolUsed.AddDynamic(SprayMarker, &ABaseInteractable::OnInteract);
				Wiper->OnToolUsed.AddDynamic(WiperMarker, &ABaseInteractable::OnInteract);
				SprayMarker->InteractionWidgetComp->SetVisibility(true);
				WiperMarker->InteractionWidgetComp->SetVisibility(true);

				UEnhancedInputComponent* Input = NewObject<UEnhancedInputComponent>(Character);
				UEnhancedPlayerInput* PlayerInput = NewObject<UEnhancedPlayerInput>(Controller);
				Character->SetupPlayerInputComponent(Input);
				Character->CreateToolMappingContext();
				FTestInputSubsystem Subsystem(PlayerInput);
				FModifyContextOptions Options;
				Options.bForceImmediately = true;
				Subsystem.AddMappingContext(Character->ToolMappingContext, 0, Options);
				TArray<UInputComponent*> InputStack { Input };
				auto InputFrame = [&](const FKey& Key, EInputEvent Event)
				{
					PlayerInput->InputKey(FInputKeyEventArgs::CreateSimulated(Key, Event, Event == IE_Pressed ? 1.0f : 0.0f));
					PlayerInput->ProcessInputStack(InputStack, 0.016f, false);
				};

				InputFrame(EKeys::Two, IE_Pressed);
				TestTrue(TEXT("Key 2 selects wiper"), Character->GetSelectedTool() == Wiper);
				InputFrame(EKeys::Two, IE_Released);
				InputFrame(EKeys::LeftMouseButton, IE_Pressed);
				TestFalse(TEXT("Click uses selected wiper"), WiperMarker->InteractionWidgetComp->IsVisible());
				TestTrue(TEXT("Click does not use spray"), SprayMarker->InteractionWidgetComp->IsVisible());
				WiperMarker->InteractionWidgetComp->SetVisibility(true);
				PlayerInput->ProcessInputStack(InputStack, 0.016f, false);
				TestTrue(TEXT("Holding click does not repeat use"), WiperMarker->InteractionWidgetComp->IsVisible());
				InputFrame(EKeys::LeftMouseButton, IE_Released);
				InputFrame(EKeys::One, IE_Pressed);
				TestTrue(TEXT("Key 1 selects spray"), Character->GetSelectedTool() == Spray);
				InputFrame(EKeys::One, IE_Released);
				InputFrame(EKeys::LeftMouseButton, IE_Pressed);
				TestFalse(TEXT("Next click uses spray"), SprayMarker->InteractionWidgetComp->IsVisible());
			}
		}
		Character->Destroy();
		TestFalse(TEXT("Spray is destroyed with player"), IsValid(Spray));
		TestFalse(TEXT("Wiper is destroyed with player"), IsValid(Wiper));
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
