// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "../Common/ButtonBase.h"
#include "../Common/UserWidgetBase.h"
#include "HeroeCard/HeroeSelection.h"
#include "PlayerKick/PlayerKickList.h"
#include "Chat/ChatWindow.h"
#include "../../CandyChaosLobbyGameInstance.h"
#include "../../Common/Struct/ConfigurationMaps.h"
#include "../../Common/Struct/LobbyPlayerInfo.h"
#include "../../Common/Struct/PlayerKickNameIndex.h"

#include "Lobby.generated.h"


/**
 * 
 */
UCLASS()
class  ULobby : public UUserWidgetBase
{
	GENERATED_BODY()
	
public:

	UPROPERTY(meta = (BindWidget))
	class UOverlay* ChatWindowBox;

	UPROPERTY(EditAnyWhere)
	TSubclassOf<UChatWindow> ChatWindowClass;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, BlueprintProtected = true, AllowPrivateAccess = true))
	TObjectPtr<UButtonBase> ReadyButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, BlueprintProtected = true, AllowPrivateAccess = true))
	TObjectPtr<UButtonBase> ReadyUpButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, BlueprintProtected = true, AllowPrivateAccess = true))
	TObjectPtr<UButtonBase> HeroesButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, BlueprintProtected = true, AllowPrivateAccess = true))
	TObjectPtr<UButtonBase> PlayerCountButton;

	void SetCurrentPlayersFormat(FString InCurrentPlayersFormat);
	void SetServerName(FString serverName);
	void FillContainerPlayerKickList(const TArray<FPlayerKickNameIndex>& InPlayerNamesIndex);
	void UpdateReadyStatusInPLayerKickList(const TArray<FPlayerKickNameIndex>& InPlayerNamesIndex);
	void UpdateStatus();
	void SetMap(UTexture2D* mapImage, FString mapName);
	void ShowOrHideButton();
	void SetEnablePlayButton(bool bEnabled);
	void SetHiddenHeroesButton(bool bHidden);
	void SetFocusOnChatWindow();
	void InitializeChatWindow();
	void UpdateWindowChat(const FText& InPlayerName, const FText& InMessage);

protected:

	virtual bool Initialize() override;

	UPROPERTY(EditAnyWhere)
	TSubclassOf<UHeroeSelection> HeroeSelection;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, BlueprintProtected = true, AllowPrivateAccess = true))
	TObjectPtr<UPlayerKickList> PlayerKickList;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ServerName;

	UPROPERTY(Replicated, meta = (BindWidget))
	class UImage* MapImage;

	UPROPERTY(Replicated, meta = (BindWidget))
	class UTextBlock* MapName;

	UPROPERTY(meta = (BindWidget))
	class UButton* PreviousMap;

	UPROPERTY(meta = (BindWidget))
	class UButton* NextMap;

	UFUNCTION() void OnReadyButtonClicked();
	UFUNCTION() void OnPlayButtonClicked();
	UFUNCTION() void OnPreviousMapButtonClicked();
	UFUNCTION() void OnNextMapButtonClicked();
	UFUNCTION() void OnHeroesButtonClicked();
	UFUNCTION() void OnPlayerCountButtonClicked();	

private:
	void InitializeMap();
	FConfigurationMaps* GetCurrentMapByName(FString Name);
	FConfigurationMaps GetFirstOrLastMap(bool bIsFirst);
	FConfigurationMaps* GetPreviousNextMap(bool IsIncrement);
	void SetMapSelector(FConfigurationMaps* ConfigurationMaps);
	

	void NotifyMapChaged();
	UCandyChaosLobbyGameInstance* CandyChaosLobbyGameInstance;
	class ALobbyGameMode* LobbyGameMode;
	UTexture2D* CurrentTextureMap;
	UChatWindow* ChatWindow;

};
