// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class ARPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	//this function calls only on the server
	virtual void OnPossess(APawn* NewPawn) override;

	//this function calls on both the listening server, and the client.
	virtual void AcknowledgePossession(APawn* NewPawn) override;

	UFUNCTION()
	void ChangeCrosshairState(bool state);

	UFUNCTION()
	void ChangeTakeOffState(bool state, float charge);

	UFUNCTION()
	void ChangeRevivalState(bool state, float charge);

	UFUNCTION()
	void AddNewItemToUI(class URItemDataAsset* newItemAsset);

	UFUNCTION()
	bool CashMyLuck();

	UFUNCTION(Server, Reliable)
	void Server_RequestRevive(AEOSPlayerState* TargetPlayerState);

	UFUNCTION()
	void AddBossEnemy(int level, class AREnemyBase* bossEnemy);

private:
	void PostPossessionSetup(APawn* NewPawn);

	void CreateGameplayUI();

	UFUNCTION(Client, Reliable)
	void ImplimentBossEnemy(int level, class AREnemyBase* bossEnemy);

	UPROPERTY()
	class ARPlayerBase* PlayerBase;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UGameplayUI> GameplayUIClass;

	UPROPERTY(VisibleAnywhere, Category = "UI")
	UGameplayUI* GameplayUI;
};
