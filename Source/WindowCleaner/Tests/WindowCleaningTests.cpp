#if WITH_DEV_AUTOMATION_TESTS

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Variant_Horror/HorrorCharacter.h"
#include "Window.h"

#if WITH_EDITOR
#include "Editor.h"
#include "GameFramework/PlayerController.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "WiperTool.h"

DEFINE_LATENT_AUTOMATION_COMMAND_THREE_PARAMETER(FWindowInteractionInputCommand,
	FAutomationTestBase*, Test, FKey, Key, bool, bPressed);

bool FWindowInteractionInputCommand::Update()
{
	UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	if (Test->TestNotNull(TEXT("Local PIE player controller"), Controller))
	{
		Controller->InputKey(FInputKeyEventArgs::CreateSimulated(
			Key, bPressed ? IE_Pressed : IE_Released, bPressed ? 1.0f : 0.0f));
	}
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FCheckWindowInteractionCommand,
	FAutomationTestBase*, Test, bool, bCleaning);

bool FCheckWindowInteractionCommand::Update()
{
	UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	AHorrorCharacter* Character = Controller ? Cast<AHorrorCharacter>(Controller->GetPawn()) : nullptr;
	if (Test->TestNotNull(TEXT("PIE Horror character"), Character))
	{
		Test->TestEqual(TEXT("E toggles window cleaning"), Character->IsCleaningWindow(), bCleaning);
		Test->TestEqual(TEXT("Mouse cursor matches cleaning mode"), Controller->bShowMouseCursor, bCleaning);
		Test->TestEqual(TEXT("Move input matches cleaning mode"), Controller->IsMoveInputIgnored(), bCleaning);
		Test->TestEqual(TEXT("Look input matches cleaning mode"), Controller->IsLookInputIgnored(), bCleaning);
		if (bCleaning)
		{
			Test->TestNotNull(TEXT("Window is the camera target"), Cast<AWindow>(Controller->GetViewTarget()));
		}
		else
		{
			Test->TestTrue(TEXT("Player camera is restored"), Controller->GetViewTarget() == Character);
		}
	}
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FCheckWiperSelectedCommand, FAutomationTestBase*, Test);

bool FCheckWiperSelectedCommand::Update()
{
	UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	AHorrorCharacter* Character = Controller ? Cast<AHorrorCharacter>(Controller->GetPawn()) : nullptr;
	if (Test->TestNotNull(TEXT("PIE Horror character"), Character))
	{
		Test->TestTrue(TEXT("Wiper is selected"), Character->GetSelectedTool() == Character->GetWiperTool());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWindowPIEInteractionTest, "WindowCleaner.Gameplay.PIEInteraction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWindowPIEInteractionTest::RunTest(const FString& Parameters)
{
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Variant_Horror/Lvl_Horror"));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FWindowInteractionInputCommand(this, EKeys::E, true));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.1f));
	ADD_LATENT_AUTOMATION_COMMAND(FWindowInteractionInputCommand(this, EKeys::E, false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.3f));
	ADD_LATENT_AUTOMATION_COMMAND(FCheckWindowInteractionCommand(this, true));
	ADD_LATENT_AUTOMATION_COMMAND(FWindowInteractionInputCommand(this, EKeys::E, true));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.1f));
	ADD_LATENT_AUTOMATION_COMMAND(FWindowInteractionInputCommand(this, EKeys::E, false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.3f));
	ADD_LATENT_AUTOMATION_COMMAND(FCheckWindowInteractionCommand(this, false));
	ADD_LATENT_AUTOMATION_COMMAND(FWindowInteractionInputCommand(this, EKeys::Two, true));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.1f));
	ADD_LATENT_AUTOMATION_COMMAND(FWindowInteractionInputCommand(this, EKeys::Two, false));
	ADD_LATENT_AUTOMATION_COMMAND(FCheckWiperSelectedCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FWindowInteractionInputCommand(this, EKeys::E, true));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.1f));
	ADD_LATENT_AUTOMATION_COMMAND(FWindowInteractionInputCommand(this, EKeys::E, false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.3f));
	ADD_LATENT_AUTOMATION_COMMAND(FCheckWindowInteractionCommand(this, true));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWindowCleaningTest, "WindowCleaner.Gameplay.Window",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWindowCleaningTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());

	AWindow* Window = World->SpawnActor<AWindow>();
	UClass* CharacterClass = LoadClass<AHorrorCharacter>(nullptr,
		TEXT("/Game/Variant_Horror/Blueprints/BP_HorrorCharacter.BP_HorrorCharacter_C"));
	AHorrorCharacter* Character = CharacterClass ? World->SpawnActor<AHorrorCharacter>(CharacterClass) : nullptr;
	if (TestNotNull(TEXT("Window"), Window) && TestNotNull(TEXT("Character"), Character))
	{
		Window->DispatchBeginPlay();
		TestTrue(TEXT("Window starts dirty"), Window->GetCleaningPhase() == EWindowCleaningPhase::Soaping);
		TestEqual(TEXT("Initial progress"), Window->GetCleaningProgress(), 0.0f);

		FVector2D Point;
		TestTrue(TEXT("Camera-facing ray hits the surface"), Window->ProjectRayToSurface(
			FVector(-200.0, 0.0, 0.0), FVector::ForwardVector, Point));
		TestTrue(TEXT("Ray hits center"), Point.IsNearlyZero());
		TestFalse(TEXT("Ray outside the surface is rejected"), Window->ProjectRayToSurface(
			FVector(-200.0, 60.0, 0.0), FVector::ForwardVector, Point));
		TestFalse(TEXT("Ray behind the surface is rejected"), Window->ProjectRayToSurface(
			FVector(-200.0, 0.0, 0.0), -FVector::ForwardVector, Point));

		TestTrue(TEXT("First player can begin cleaning"), Window->BeginCleaning(Character));
		TestFalse(TEXT("A second session cannot start on the same window"), Window->BeginCleaning(Character));
		Window->ApplyBrush(FVector2D::ZeroVector, FVector2D::ZeroVector, false);
		TestEqual(TEXT("Wiper cannot clean before soaping"), Window->GetCleaningProgress(), 0.0f);
		Window->ApplyBrush(FVector2D::ZeroVector, FVector2D::ZeroVector, true);
		TestTrue(TEXT("Spray covers part of the window"), Window->GetCleaningProgress() > 0.0f && Window->GetCleaningProgress() < 1.0f);
		const float PartialProgress = Window->GetCleaningProgress();
		Window->EndCleaning(Character);
		TestTrue(TEXT("Player can resume"), Window->BeginCleaning(Character));
		TestEqual(TEXT("Progress survives exit and return"), Window->GetCleaningProgress(), PartialProgress);

		for (int32 Row = 0; Row < 16; ++Row)
		{
			const double Height = -49.0 + Row * 98.0 / 15.0;
			Window->ApplyBrush(FVector2D(-49.0, Height), FVector2D(49.0, Height), true);
		}
		TestTrue(TEXT("Full soap coverage starts wiping phase"), Window->GetCleaningPhase() == EWindowCleaningPhase::Wiping);
		TestEqual(TEXT("Wiping starts at zero progress"), Window->GetCleaningProgress(), 0.0f);
		Window->ApplyBrush(FVector2D::ZeroVector, FVector2D::ZeroVector, true);
		TestEqual(TEXT("Spray cannot clean"), Window->GetCleaningProgress(), 0.0f);

		for (int32 Row = 0; Row < 16; ++Row)
		{
			const double Height = -49.0 + Row * 98.0 / 15.0;
			Window->ApplyBrush(FVector2D(-49.0, Height), FVector2D(49.0, Height), false);
		}
		TestTrue(TEXT("Complete only after all cells are wiped"), Window->GetCleaningPhase() == EWindowCleaningPhase::Complete);
		TestEqual(TEXT("Completed progress"), Window->GetCleaningProgress(), 1.0f);
		Window->EndCleaning(Character);
	}

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
