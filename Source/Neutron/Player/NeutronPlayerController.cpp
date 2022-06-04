// Neutron - Gwennaël Arbona

#include "NeutronPlayerController.h"

#include "Neutron/Actor/NeutronActorTools.h"

#include "Neutron/Settings/NeutronGameUserSettings.h"
#include "Neutron/Settings/NeutronWorldSettings.h"

#include "Neutron/System/NeutronAssetManager.h"
#include "Neutron/System/NeutronContractManager.h"
#include "Neutron/System/NeutronMenuManager.h"
#include "Neutron/System/NeutronPostProcessManager.h"
#include "Neutron/System/NeutronGameInstance.h"
#include "Neutron/System/NeutronSaveManager.h"
#include "Neutron/System/NeutronSessionsManager.h"
#include "Neutron/System/NeutronSoundManager.h"

#include "Neutron/Neutron.h"

#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

#define LOCTEXT_NAMESPACE "ANeutronPlayerController"

/*----------------------------------------------------
    Constructor
----------------------------------------------------*/

ANeutronPlayerController::ANeutronPlayerController()
	: Super()
	, LastNetworkError(ENeutronNetworkError::Success)
	, CurrentCameraState(0)
	, CurrentTimeInCameraState(0)
	, SharedTransitionActive(false)
{}

/*----------------------------------------------------
    Inherited
----------------------------------------------------*/

void ANeutronPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (IsLocalPlayerController())
	{
		UNeutronGameUserSettings* GameUserSettings = Cast<UNeutronGameUserSettings>(GEngine->GetGameUserSettings());

		// Process FOV
		NCHECK(PlayerCameraManager);
		float FOV = PlayerCameraManager->GetFOVAngle();
		if (FOV != GameUserSettings->FOV)
		{
			NLOG("ANeutronPlayerController::PlayerTick : new FOV %d", static_cast<int>(GameUserSettings->FOV));
			PlayerCameraManager->SetFOV(GameUserSettings->FOV);
		}

		// Show network errors
		UNeutronSessionsManager* SessionsManager = UNeutronSessionsManager::Get();
		NCHECK(SessionsManager);
		if (SessionsManager->GetNetworkError() != LastNetworkError)
		{
			LastNetworkError = SessionsManager->GetNetworkError();
			if (LastNetworkError != ENeutronNetworkError::Success)
			{
				Notify(LOCTEXT("NetworkError", "Network error"), SessionsManager->GetNetworkErrorString(), ENeutronNotificationType::Error);
			}
		}

		CurrentTimeInCameraState += DeltaTime;
	}
}

/*----------------------------------------------------
    Game flow
----------------------------------------------------*/

void ANeutronPlayerController::SetGameOnline(bool Online, uint32 MaxPlayers)
{
	NLOG("ANeutronPlayerController::SetGameOnline : online = %d", Online);

	if (UNeutronMenuManager::Get()->IsIdle())
	{
		UNeutronSoundManager::Get()->Mute();

		UNeutronMenuManager::Get()->RunAction(ENeutronLoadingScreen::Launch,    //
			FNeutronAsyncAction::CreateLambda(
				[=]()
				{
					GetGameInstance<UNeutronGameInstance>()->SetGameOnline(GetWorld()->GetName(), Online, MaxPlayers);
				}));
	}
}

void ANeutronPlayerController::GoToMainMenu(bool ShouldSaveGame)
{
	if (UNeutronMenuManager::Get()->IsIdle())
	{
		NLOG("ANeutronPlayerController::GoToMainMenu %d", ShouldSaveGame);

		UNeutronSoundManager::Get()->Mute();

		UNeutronMenuManager::Get()->RunAction(ENeutronLoadingScreen::Black,    //
			FNeutronAsyncAction::CreateLambda(
				[=]()
				{
					if (ShouldSaveGame)
					{
						NLOG("ANeutronPlayerController::GoToMainMenu : saving game");
						SaveGame();
					}

					GetGameInstance<UNeutronGameInstance>()->GoToMainMenu();
				}));
	}
}

void ANeutronPlayerController::ExitGame()
{
	if (UNeutronMenuManager::Get()->IsIdle())
	{
		NLOG("ANeutronPlayerController::ExitGame");

		UNeutronSoundManager::Get()->Mute();

		UNeutronMenuManager::Get()->RunAction(ENeutronLoadingScreen::Black,    //
			FNeutronAsyncAction::CreateLambda(
				[=]()
				{
					FGenericPlatformMisc::RequestExit(false);
				}));
	}
}

void ANeutronPlayerController::InviteFriend(TSharedRef<FOnlineFriend> Friend)
{
	NLOG("ANeutronPlayerController::InviteFriend");

	Notify(LOCTEXT("InviteFriend", "Invited friend"), FText::FromString(Friend->GetDisplayName()), ENeutronNotificationType::Info);

	UNeutronSessionsManager::Get()->InviteFriend(Friend->GetUserId());
}

void ANeutronPlayerController::JoinFriend(TSharedRef<FOnlineFriend> Friend)
{
	NLOG("ANeutronPlayerController::JoinFriend");

	UNeutronMenuManager::Get()->RunAction(ENeutronLoadingScreen::Launch,    //
		FNeutronAsyncAction::CreateLambda(
			[=]()
			{
				Notify(LOCTEXT("JoiningFriend", "Joining friend"), FText::FromString(Friend->GetDisplayName()),
					ENeutronNotificationType::Info);

				UNeutronSessionsManager::Get()->JoinFriend(Friend->GetUserId());
			}));
}

void ANeutronPlayerController::AcceptInvitation(const FOnlineSessionSearchResult& InviteResult)
{
	NLOG("ANeutronPlayerController::AcceptInvitation");

	UNeutronMenuManager::Get()->RunAction(ENeutronLoadingScreen::Launch,    //
		FNeutronAsyncAction::CreateLambda(
			[=]()
			{
				UNeutronSessionsManager::Get()->JoinSearchResult(InviteResult);
			}));
}

/*----------------------------------------------------
    Menus
----------------------------------------------------*/

bool ANeutronPlayerController::IsOnMainMenu() const
{
	return Cast<ANeutronWorldSettings>(GetWorld()->GetWorldSettings())->IsMainMenuMap();
}

bool ANeutronPlayerController::IsMenuOnly() const
{
	return Cast<ANeutronWorldSettings>(GetWorld()->GetWorldSettings())->IsMenuMap();
}

/*----------------------------------------------------
    Shared transitions
----------------------------------------------------*/

void ANeutronPlayerController::SharedTransition(
	uint8 NewCameraState, FNeutronAsyncAction StartAction, FNeutronAsyncCondition Condition, FNeutronAsyncAction FinishAction)
{
	NCHECK(GetLocalRole() == ROLE_Authority);
	NLOG("ANeutronPlayerController::ServerSharedTransition");

	for (ANeutronPlayerController* OtherPlayer : TActorRange<ANeutronPlayerController>(GetWorld()))
	{
		OtherPlayer->ClientStartSharedTransition(NewCameraState);
	}

	SharedTransitionStartAction  = StartAction;
	SharedTransitionFinishAction = FinishAction;
	SharedTransitionCondition    = Condition;
}

void ANeutronPlayerController::ClientStartSharedTransition_Implementation(uint8 NewCameraState)
{
	NLOG("ANeutronPlayerController::ClientStartSharedTransition_Implementation");

	// Shared transitions work like this :
	// - Server signals all players to fade to black through this very method
	// - Once faded, Action is called and all players call ServerSharedTransitionReady() to signal they're dark
	// - Server fires SharedTransitionStartAction when all clients have called ServerSharedTransitionReady()
	// - Server fires SharedTransitionFinishAction once SharedTransitionCondition returns true on the server
	// - Server then calls ClientStopSharedTransition() on all players so that they know to resume
	// - All players then fade back to the game

	SharedTransitionActive = true;

	// Action : mark as in shared transition locally and remotely
	FNeutronAsyncAction Action = FNeutronAsyncAction::CreateLambda(
		[=]()
		{
			SetCameraState(NewCameraState);
			ServerSharedTransitionReady();
			NLOG("ANeutronPlayerController::ClientStartSharedTransition_Implementation : done, waiting for server");
		});

	// Condition : on server, when all clients have reported as ready
	// On client, when the server has signaled to stop
	FNeutronAsyncCondition Condition = FNeutronAsyncCondition::CreateLambda(
		[=]()
		{
			if (GetLocalRole() == ROLE_Authority)
			{
				// Check if all players are in transition
				bool AllPlayersInTransition = true;
				for (ANeutronPlayerController* OtherPlayer : TActorRange<ANeutronPlayerController>(GetWorld()))
				{
					if (!OtherPlayer->SharedTransitionActive)
					{
						AllPlayersInTransition = false;
						break;
					}
				}

				// Once all players are in the transition, fire the start event, wait for the condition, fire the end event, and stop
				if (AllPlayersInTransition)
				{
					SharedTransitionStartAction.ExecuteIfBound();
					SharedTransitionStartAction.Unbind();

					if (!SharedTransitionCondition.IsBound() || SharedTransitionCondition.Execute())
					{
						SharedTransitionFinishAction.ExecuteIfBound();
						SharedTransitionFinishAction.Unbind();
						SharedTransitionCondition.Unbind();

						for (ANeutronPlayerController* OtherPlayer : TActorRange<ANeutronPlayerController>(GetWorld()))
						{
							OtherPlayer->ClientStopSharedTransition();
						}

						return true;
					}
				}

				return false;
			}
			else
			{
				return !SharedTransitionActive;
			}
		});

	// Run the process
	if (GetSharedTransitionMenuState(NewCameraState))
	{
		UNeutronMenuManager::Get()->OpenMenu(Action, Condition);
	}
	else
	{
		UNeutronMenuManager::Get()->CloseMenu(Action, Condition);
	}
}

void ANeutronPlayerController::ClientStopSharedTransition_Implementation()
{
	NLOG("ANeutronPlayerController::ClientStopSharedTransition_Implementation");

	SharedTransitionActive = false;
}

void ANeutronPlayerController::ServerSharedTransitionReady_Implementation()
{
	NCHECK(GetLocalRole() == ROLE_Authority);
	NLOG("ANeutronPlayerController::ServerSharedTransitionReady_Implementation");

	SharedTransitionActive = true;
}

/*----------------------------------------------------
    Input
----------------------------------------------------*/

void ANeutronPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Core bindings
	FInputActionBinding& AnyKeyBinding =
		InputComponent->BindAction("AnyKey", EInputEvent::IE_Pressed, this, &ANeutronPlayerController::AnyKey);
	AnyKeyBinding.bConsumeInput = false;
	InputComponent->BindAction(
		FNeutronPlayerInput::MenuToggle, EInputEvent::IE_Released, this, &ANeutronPlayerController::ToggleMenuOrQuit);
	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		InputComponent->BindKey(EKeys::Enter, EInputEvent::IE_Released, this, &ANeutronPlayerController::ToggleMenuOrQuit);
	}
}

void ANeutronPlayerController::ToggleMenuOrQuit()
{
	if (!Cast<ANeutronWorldSettings>(GetWorld()->GetWorldSettings())->IsMenuMap())
	{
		if (IsOnMainMenu())
		{
			ExitGame();
		}
		else
		{
			UNeutronMenuManager* MenuManager = UNeutronMenuManager::Get();

			if (!MenuManager->IsMenuOpen())
			{
				MenuManager->OpenMenu();
			}
			else
			{
				MenuManager->CloseMenu();
			}
		}
	}
}

void ANeutronPlayerController::AnyKey(FKey Key)
{
	UNeutronMenuManager::Get()->SetUsingGamepad(Key.IsGamepadKey());
}

/*----------------------------------------------------
    Test code
----------------------------------------------------*/

#if WITH_EDITOR

void ANeutronPlayerController::OnJoinRandomFriend(TArray<TSharedRef<FOnlineFriend>> FriendList)
{
	for (auto Friend : FriendList)
	{
		if (Friend.Get().GetDisplayName() == "Deimos Games" || Friend.Get().GetDisplayName() == "Stranger")
		{
			JoinFriend(Friend);
		}
	}
}

void ANeutronPlayerController::OnJoinRandomSession(TArray<FOnlineSessionSearchResult> SearchResults)
{
	for (auto Result : SearchResults)
	{
		if (Result.Session.OwningUserId != GetLocalPlayer()->GetPreferredUniqueNetId())
		{
			UNeutronMenuManager* MenuManager = UNeutronMenuManager::Get();

			MenuManager->RunAction(ENeutronLoadingScreen::Launch,    //
				FNeutronAsyncAction::CreateLambda(
					[=]()
					{
						Notify(FText::FormatNamed(LOCTEXT("JoinFriend", "Joining {session}"), TEXT("session"),
								   FText::FromString(*Result.Session.GetSessionIdStr())),
							FText(), ENeutronNotificationType::Info);

						UNeutronSessionsManager::Get()->JoinSearchResult(Result);
					}));
		}
	}
}

void ANeutronPlayerController::TestJoin()
{
	UNeutronSessionsManager* SessionsManager = UNeutronSessionsManager::Get();

	if (SessionsManager->GetOnlineSubsystemName() != TEXT("Null"))
	{
		SessionsManager->SearchFriends(FNeutronOnFriendSearchComplete::CreateUObject(this, &ANeutronPlayerController::OnJoinRandomFriend));
	}
	else
	{
		SessionsManager->SearchSessions(
			true, FNeutronOnSessionSearchComplete::CreateUObject(this, &ANeutronPlayerController::OnJoinRandomSession));
	}
}

#endif

#undef LOCTEXT_NAMESPACE
