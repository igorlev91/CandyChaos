// Fill out your copyright notice in the Description page of Project Settings.


#include "InGameGameMode.h"
#include "Net/UnrealNetwork.h"
#include "../PlayerState/CandyChaosLobbyPlayerState.h"
#include "../Common/Library/CandyChaosLoobyBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"


AInGameGameMode::AInGameGameMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	this->CandyChaosLobbyGameInstance = UCandyChaosLoobyBlueprintLibrary::GetCandyChaosLobbyGameInstance(this);
}

void AInGameGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (HasAuthority())
	{
		if (NewPlayer) {
			AInGamePlayerController* InGamePlayerController = Cast<AInGamePlayerController>(NewPlayer);

			this->AllPlayerControllers.Add(InGamePlayerController);

			if (IsValid(this->CandyChaosLobbyGameInstance))
				this->Server_RespawnPlayer(InGamePlayerController);
		}
	}
}

void AInGameGameMode::Logout(AController* Exiting)
{

}

void AInGameGameMode::Server_RespawnPlayer_Implementation(AInGamePlayerController* InGamePlayerController)
{
	FActorSpawnParameters params;
	params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	int32 InPlayerIndex = this->AllPlayerControllers.IndexOfByKey(InGamePlayerController);

	FTransform Transform = this->GetTransformOfPlayerStartByIndex(InPlayerIndex);

	FInGamePlayerInfo* InGamePlayerInfo = this->GetPlayerInfoByIndex((InPlayerIndex + 1));

	ARCharacterBase* CharacterBase = Cast<ARCharacterBase>(GetWorld()->SpawnActor<ARCharacterBase>(InGamePlayerInfo->HeroeSelected, Transform, params));

	FTimerHandle MemberTimerHandle;
	FTimerDelegate TimerDel;

	TimerDel.BindUFunction(this, FName("Server_SetOverheadPlayer"), true, FString("FFFFFF"));

	GetWorld()->GetTimerManager().SetTimer(MemberTimerHandle, TimerDel, 0.1f, false);

	InGamePlayerController->SetCurrentCharacter(CharacterBase);
	InGamePlayerController->Possess(CharacterBase);
}

void AInGameGameMode::Server_UpdatePlayerName_Implementation()
{
	for (AInGamePlayerController* InGamePlayerController : this->AllPlayerControllers)
		this->UpdatePlayerName(InGamePlayerController);
}

void AInGameGameMode::Server_SetOverheadPlayer_Implementation(bool bIsHiddenIcon, const FString& ColorName)
{
	for (AInGamePlayerController* InGamePlayerController : this->AllPlayerControllers) {

		ARCharacterBase* CharacterBase = InGamePlayerController->GetCurrentCharacter();

		if (IsValid(CharacterBase))
			CharacterBase->Multi_SetIconAndColorOverheadWidget(bIsHiddenIcon, ColorName);
	}

}

void AInGameGameMode::UpdatePlayerName(AInGamePlayerController* InGamePlayerController)
{
	ACandyChaosLobbyPlayerState* CandyChaosLobbyPlayerState = InGamePlayerController->GetPlayerState<ACandyChaosLobbyPlayerState>();

	ARCharacterBase* CharacterBase = InGamePlayerController->GetCurrentCharacter();

	if (IsValid(CharacterBase) && IsValid(CandyChaosLobbyPlayerState))
		CharacterBase->Multi_SetPlayerName(CandyChaosLobbyPlayerState->GetPlayerName());
}


TArray<AActor*> AInGameGameMode::GetAllPlayersStart()
{
	TArray<AActor*> AllPlayersStart;

	this->PlayerStart = APlayerStart::StaticClass();

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), this->PlayerStart, AllPlayersStart);

	return AllPlayersStart;
}

FTransform AInGameGameMode::GetTransformOfPlayerStartByIndex(int32 InPlayerIndex)
{
	TArray<AActor*> PlayersStart = this->GetAllPlayersStart();

	FTransform Transform = PlayersStart[InPlayerIndex]->GetTransform();

	return Transform;
}

FInGamePlayerInfo* AInGameGameMode::GetPlayerInfoByIndex(int32 InPlayerIndex)
{
	auto GetPlayerByIndex = [InPlayerIndex](const FInGamePlayerInfo& Player) {
		return Player.PlayerIndex == InPlayerIndex;
		};

	FInGamePlayerInfo* InGamePlayerInfo = this->CandyChaosLobbyGameInstance->InGamePlayersInfo.FindByPredicate(GetPlayerByIndex);

	return InGamePlayerInfo;
}

void AInGameGameMode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AInGameGameMode, ConnectedPlayers);
	DOREPLIFETIME(AInGameGameMode, AllPlayerControllers);
}
