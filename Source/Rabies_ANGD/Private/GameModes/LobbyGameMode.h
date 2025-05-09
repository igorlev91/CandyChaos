// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "../Character/RLobbyCharacter.h"
#include "../Common/Struct/LobbyHeroeSpot.h"
#include "../Widgets/Lobby/LobbyPlayerSpots.h"
#include "../PlayerController/LobbyPlayerController.h"
#include "../CandyChaosLobbyGameInstance.h"

#include "LobbyGameMode.generated.h"


UCLASS()
class ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ALobbyGameMode(const FObjectInitializer& ObjectInitializer);
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	UFUNCTION(BlueprintCallable)
	bool IsAllPlayerReady();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_SwapCharacter(APlayerController* PlayerController, TSubclassOf<ARLobbyCharacter> InHeroeSelected, bool bChangeStatus);
	void Server_SwapCharacter_Implementation(APlayerController* PlayerController, TSubclassOf<ARLobbyCharacter> InHeroeSelected, bool bChangeStatus);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_UpdateGameSettings(UTexture2D* mapImage, const FString& mapName);
	void Server_UpdateGameSettings_Implementation(UTexture2D* mapImage, const FString& mapName);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_RespawnPlayer(ALobbyPlayerController* LobbyPlayerController);
	void Server_RespawnPlayer_Implementation(ALobbyPlayerController* LobbyPlayerController);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_EveryoneUpdate();
	void Server_EveryoneUpdate_Implementation();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_SpawnLobbyPlayerSpot(ALobbyPlayerController* LobbyPlayerController);
	void Server_SpawnLobbyPlayerSpot_Implementation(ALobbyPlayerController* LobbyPlayerController);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_RemoveCharacterFromLobby(ALobbyPlayerController* LobbyPlayerController);
	void Server_RemoveCharacterFromLobby_Implementation(ALobbyPlayerController* LobbyPlayerController);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_UpdatePlayerName();
	void Server_UpdatePlayerName_Implementation();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_SetViewTargetSpot();
	void Server_SetViewTargetSpot_Implementation();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_FillContainerPlayerKickList();
	void Server_FillContainerPlayerKickList_Implementation();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_LaunchTheGame();
	void Server_LaunchTheGame_Implementation();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_ShouldHideLoadingScreen(ALobbyPlayerController* LobbyPlayerController);
	void Server_ShouldHideLoadingScreen_Implementation(ALobbyPlayerController* LobbyPlayerController);
	

	FLobbyHeroeSpot* GetLobbyHeroeSpotByPlayerConnected(ALobbyPlayerController* LobbyPlayerController);

	UPROPERTY(BlueprintReadOnly, Replicated)
		TArray<class ALobbyPlayerController*> AllPlayerControllers;
	UPROPERTY(Replicated)
		TArray<struct FLobbyPlayerInfo> ConnectedPlayers;
	UPROPERTY(Replicated)
		UTexture2D* MapImage;
	UPROPERTY(Replicated)
		FString MapName;
	UPROPERTY(Replicated)
		int32 CurrentPlayers;
	UPROPERTY(Replicated)
		int32 MaxPlayers;
	UPROPERTY(EditAnyWhere)
		TSubclassOf<ALobbyPlayerSpots> LobbyPlayerSpotClass;

private:
	void DestroyCharacterSelectedIfExits(ALobbyPlayerController* LobbyPlayerController);
	void SpawnCharacterOnPlayerSpot(ALobbyPlayerController* LobbyPlayerController);
	void UpdatePlayerName(ALobbyPlayerController* LobbyPlayerController);
	UFUNCTION()
	void UpdateReadyStatus(ALobbyPlayerController* LobbyPlayerController);
	void FillConnectedPlayers();
	void SetPlayerInfoToTransfer();
	TArray<FPlayerKickNameIndex> GetPlayerKickNameIndex();
	class UCandyChaosLobbyGameInstance* CandyChaosLobbyGameInstance;
};
