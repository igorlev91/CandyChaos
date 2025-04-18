// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "../Character/RLobbyCharacter.h"
#include "InGamePlayerController.generated.h"

/**
 * 
 */
UCLASS()
class  AInGamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void SetCurrentCharacter(ARLobbyCharacter* InCurrentCharacter);
	ARLobbyCharacter* GetCurrentCharacter();

private:
	ARLobbyCharacter* CurrentCharacter;
};
