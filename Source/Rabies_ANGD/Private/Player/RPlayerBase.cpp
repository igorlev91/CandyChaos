// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/RPlayerBase.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Player/RPlayerController.h"
#include "Framework/EOSPlayerState.h"

#include "Actors/EscapeToWin.h"
#include "Widgets/ReviveUI.h"
#include "GameplayAbilities/RAttributeSet.h"
#include "GameplayAbilities/RAbilitySystemComponent.h"
#include "GameplayAbilities/RAbilityGenericTags.h"
#include "Framework/EOSActionGameState.h"

#include "Framework/RItemDataAsset.h"

#include "Actors/ItemChest.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimInstance.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

#include "Components/SphereComponent.h"

#include "Net/UnrealNetwork.h"

#include "Actors/ItemPickup.h"
#include "Components/WidgetComponent.h"

#include "Framework/RGameMode.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SceneComponent.h"

#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"

#define ECC_PingInput ECC_GameTraceChannel4

ARPlayerBase::ARPlayerBase()
{
	viewPivot = CreateDefaultSubobject<USceneComponent>("Camera Pivot");
	viewCamera = CreateDefaultSubobject<UCameraComponent>("View Camera");

	viewCamera->SetupAttachment(viewPivot, USpringArmComponent::SocketName);
	viewCamera->SetRelativeLocation(DefaultCameraLocal);
	viewCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(1080.f);
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 1.0f;
	

	ReviveUIWidgetComp = CreateDefaultSubobject<UWidgetComponent>("Revive Widget Comp");
	ReviveUIWidgetComp->SetupAttachment(GetRootComponent());

	AudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio Component"));
	AudioComp->SetupAttachment(GetRootComponent());
	AudioComp->bAutoActivate = false;

	// sphere radius
	ItemPickupCollider = CreateDefaultSubobject<USphereComponent>(TEXT("Item Collider"));
	ItemPickupCollider->SetupAttachment(GetRootComponent());
	ItemPickupCollider->SetCollisionProfileName(TEXT("OverlapAll"));

	cameraClampMax = 50;
	cameraClampMin = -80;
	bIsScoping = false;

	GroundCheckComp = CreateDefaultSubobject<USphereComponent>("Ground Check Comp");
	GroundCheckComp->SetupAttachment(GetRootComponent());
	GroundCheckComp->OnComponentBeginOverlap.AddDynamic(this, &ARPlayerBase::GroundCheckCompOverlapped);
}

void ARPlayerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (EOSPlayerState && IsLocallyControlled())
	{
		//UE_LOG(LogTemp, Error, TEXT(""), *GetName());
		SetAllyHealthBars();
		viewPivot->SetRelativeLocation(GetActorLocation()); // centers the pivot on the player without getting the players rotation
		EOSPlayerState->Server_UpdatePlayerVelocity(GetCharacterMovement()->Velocity);
		EOSPlayerState->Server_UpdateSocketLocations(GetMesh()->GetSocketLocation(RootAimingSocketName), GetMesh()->GetSocketLocation(RangedAttackSocketName));

		if (bIsScoping)
		{
			RotatePlayer(DeltaTime);
		}
	}

}

void ARPlayerBase::SetAllyHealthBars()
{
	FVector viewLoc;
	FRotator viewRot;

	playerController->GetPlayerViewPoint(viewLoc, viewRot);

	// Find all actors in the world
	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(playerController->GetWorld(), ARPlayerBase::StaticClass(), AllActors);

	for (AActor* Actor : AllActors)
	{
		ARPlayerBase* allyPlayer = Cast<ARPlayerBase>(Actor);

		if (!allyPlayer || allyPlayer == this)
			continue;

		allyPlayer->SetHealthBarFromAllyPerspective(GetActorLocation());
	}
}

void ARPlayerBase::BeginPlay()
{
	Super::BeginPlay();

	ItemPickupCollider->OnComponentBeginOverlap.AddDynamic(this, &ARPlayerBase::OnOverlapBegin);
	ItemPickupCollider->OnComponentEndOverlap.AddDynamic(this, &ARPlayerBase::OnOverlapEnd);

	OnDeadStatusChanged.AddUObject(this, &ARPlayerBase::DeadStatusUpdated);
	OnInvisStatusChanged.AddUObject(this, &ARPlayerBase::InvisStatusUpdated);

	DynamicTexMaterialInstance = UMaterialInstanceDynamic::Create(TexStealthMat, GetMesh());

	ReviveUIWidgetComp->SetWidgetClass(ReviveUIClass);
	ReviveUI = CreateWidget<UReviveUI>(GetWorld(), ReviveUIWidgetComp->GetWidgetClass());
	if (ReviveUI)
	{
		ReviveUIWidgetComp->SetWidget(ReviveUI);
	}
	if (!ReviveUI)
	{
		UE_LOG(LogTemp, Error, TEXT("%s can't spawn revive has the wrong widget setup"), *GetName());
		return;
	}

	ReviveUI->SetRenderScale(FVector2D{ 2.5f });
	ReviveUI->SetVisibility(ESlateVisibility::Hidden);
}

void ARPlayerBase::PawnClientRestart()
{
	Super::PawnClientRestart();

	APlayerController* PlayerController = GetController<APlayerController>();
	if (PlayerController)
	{
		UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
		InputSubsystem->ClearAllMappings();
		InputSubsystem->AddMappingContext(inputMapping, 0);
	}
}

void ARPlayerBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	UEnhancedInputComponent* enhancedInputComp = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (enhancedInputComp)
	{
		enhancedInputComp->BindAction(moveInputAction, ETriggerEvent::Triggered, this, &ARPlayerBase::Move);
		enhancedInputComp->BindAction(lookInputAction, ETriggerEvent::Triggered, this, &ARPlayerBase::Look);
		enhancedInputComp->BindAction(jumpInputAction, ETriggerEvent::Started, this, &ARPlayerBase::StartJump);
		enhancedInputComp->BindAction(jumpInputAction, ETriggerEvent::Completed, this, &ARPlayerBase::ReleaseJump);
		enhancedInputComp->BindAction(QuitOutAction, ETriggerEvent::Triggered, this, &ARPlayerBase::QuitOut);
		enhancedInputComp->BindAction(basicAttackAction, ETriggerEvent::Started, this, &ARPlayerBase::DoBasicAttack);
		enhancedInputComp->BindAction(basicAttackAction, ETriggerEvent::Triggered, this, &ARPlayerBase::DoAutomaticAttack);
		enhancedInputComp->BindAction(basicAttackAction, ETriggerEvent::Completed, this, &ARPlayerBase::StopBasicAttack);
		enhancedInputComp->BindAction(scopeInputAction, ETriggerEvent::Started, this, &ARPlayerBase::EnableScoping);
		enhancedInputComp->BindAction(scopeInputAction, ETriggerEvent::Completed, this, &ARPlayerBase::DisableScoping);
		enhancedInputComp->BindAction(scrollInputAction, ETriggerEvent::Triggered, this, &ARPlayerBase::Scroll);
		enhancedInputComp->BindAction(specialAttackAction, ETriggerEvent::Started, this, &ARPlayerBase::TryActivateSpecialAttack);
		enhancedInputComp->BindAction(specialAttackAction, ETriggerEvent::Completed, this, &ARPlayerBase::FinishSpecialAttack);
		enhancedInputComp->BindAction(ultimateAttackAction, ETriggerEvent::Started, this, &ARPlayerBase::TryActivateUltimateAttack);
		enhancedInputComp->BindAction(ultimateAttackAction, ETriggerEvent::Completed, this, &ARPlayerBase::FinishUltimateAttack);
		enhancedInputComp->BindAction(InteractInputAction, ETriggerEvent::Triggered, this, &ARPlayerBase::Interact);
		enhancedInputComp->BindAction(PausingInputAction, ETriggerEvent::Triggered, this, &ARPlayerBase::Pause);
		enhancedInputComp->BindAction(PingInputAction, ETriggerEvent::Triggered, this, &ARPlayerBase::Ping);
		enhancedInputComp->BindAction(LoadDebugInputAction, ETriggerEvent::Triggered, this, &ARPlayerBase::LoadDebug);
	}
}

void ARPlayerBase::Move(const FInputActionValue& InputValue)
{
	if (GetAbilitySystemComponent()->HasMatchingGameplayTag(URAbilityGenericTags::GetUnActionableTag()) || GetAbilitySystemComponent()->HasMatchingGameplayTag(URAbilityGenericTags::GetDeadTag())) return;

	MoveInput = InputValue.Get<FVector2D>();
	MoveInput.Normalize();

	AddMovementInput(MoveInput.Y * GetMoveFwdDir() + MoveInput.X * GetMoveRightDir());
}

void ARPlayerBase::RotatePlayer(float DeltaTime)
{
	FRotator cameraRot = viewPivot->GetComponentRotation();
	cameraRot.Roll = 0;

	FRotator currentRot = GetActorRotation();

	float yawDiff = cameraRot.Yaw - currentRot.Yaw;
	float pitchDiff = cameraRot.Pitch - currentRot.Pitch;

	if (yawDiff > 180.0f) yawDiff -= 360.0f;
	if (yawDiff < -180.0f) yawDiff += 360.0f;

	if (pitchDiff > 180.0f) pitchDiff -= 360.0f;
	if (pitchDiff < -180.0f) pitchDiff += 360.0f;

	float yawInput = FMath::Lerp(0.0f, yawDiff, 10 * DeltaTime);
	float pitchInput = FMath::Lerp(0.0f, pitchDiff, 10 * DeltaTime);

	AddControllerYawInput(yawInput);
	AddControllerPitchInput(pitchInput);
}

void ARPlayerBase::Look(const FInputActionValue& InputValue)
{
	FVector2D input = InputValue.Get<FVector2D>();

	FRotator newRot = viewPivot->GetComponentRotation();
	newRot.Pitch += input.Y;
	newRot.Yaw += input.X;

	newRot.Pitch = FMath::ClampAngle(newRot.Pitch, cameraClampMin, cameraClampMax);

	if (EOSPlayerState && IsLocallyControlled()) 
	{
		EOSPlayerState->Server_UpdateHitscanRotator(newRot, viewPivot->GetRelativeLocation());
	}
	viewPivot->SetWorldRotation(newRot);
}

void ARPlayerBase::SetRabiesPlayerController(ARPlayerController* newController)
{
	playerController = newController;

}

void ARPlayerBase::StartJump()
{
	if (!IsLocallyControlled())
	{
		return;
	}
	
	if (bInRangeToRevive)
	{
		GetAbilitySystemComponent()->PressInputID((int)EAbilityInputID::Revive);
		return;
	}

	GetAbilitySystemComponent()->PressInputID((int)EAbilityInputID::HoldJump);

	if (bInstantJump)
	{
		if (!GetCharacterMovement()->IsFalling())
		{
			AudioComp->SetSound(JumpAudio);
			AudioComp->Play();
		}

		Jump();
		return;
	}

	GetAbilitySystemComponent()->PressInputID((int)EAbilityInputID::Passive); // Dot's jump fly
}

void ARPlayerBase::ReleaseJump()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	FGameplayEventData eventData;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, URAbilityGenericTags::GetEndTakeOffChargeTag(), eventData);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, URAbilityGenericTags::GetEndRevivingTag(), eventData);

	if (bInstantJump) return;

	if (!IsFlying())
	{
		if (AudioComp && JumpAudio)
		{
			AudioComp->SetSound(JumpAudio);
			AudioComp->Play();
		}

		Jump();
	}
}

void ARPlayerBase::QuitOut()
{
	/*APlayerController* PlayerController = GetController<APlayerController>();
	if (PlayerController)
	{
		UKismetSystemLibrary::QuitGame(GetWorld(), PlayerController, EQuitPreference::Quit, true);
	}*/
}

void ARPlayerBase::DoAutomaticAttack()
{
	if (IsScoping())
	{
		//if (AudioComp->!IsPlaying())
		//{

		//}
		GetAbilitySystemComponent()->PressInputID((int)EAbilityInputID::BasicAttack);
	}
}

void ARPlayerBase::DoBasicAttack()
{
	if (IsScoping())
	{
		//if (AudioComp->!IsPlaying())
		//{

		//}
		GetAbilitySystemComponent()->PressInputID((int)EAbilityInputID::BasicAttack);
	}
	else
	{
		GetAbilitySystemComponent()->PressInputID((int)EAbilityInputID::MeleeAttack);
	}
}

void ARPlayerBase::Ping()
{
	if (!EOSPlayerState || !IsLocallyControlled())
	{
		return;
	}

	FVector viewLoc;
	FRotator viewRot;

	playerController->GetPlayerViewPoint(viewLoc, viewRot);
	if (bIsScoping == false)
	{
		viewLoc = GetActorLocation();
	}

	FVector startPos = viewLoc + viewRot.Vector();
	FVector endPos = startPos + viewRot.Vector() * 8000.0f;

	FCollisionShape collisionShape = FCollisionShape::MakeSphere(1);
	ECollisionChannel collisionChannel = ECC_PingInput;
	FHitResult newHitResult;
	bool hit = GetWorld()->SweepSingleByChannel(newHitResult, startPos, endPos, FQuat::Identity, collisionChannel, collisionShape);
	if (hit)
	{
		FVector hitPoint = newHitResult.ImpactPoint;
		AActor* hitActor = newHitResult.GetActor();
		Server_HandlePing(hitPoint, hitActor);
		//FColor debugColor = FColor::Blue;
		//DrawDebugCylinder(GetWorld(), startPos, endPos, 1.0f, 32, debugColor, false, 0.2f, 0U, 1.0f);
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Hit: %s"), *newHitResult.GetActor()->GetName()));


	}
}

void ARPlayerBase::Server_HandlePing_Implementation(FVector hitPoint, AActor* hitActor)
{
	AEOSActionGameState* gameState = GetWorld()->GetGameState<AEOSActionGameState>();
	if (gameState)
	{
		gameState->Server_Ping(hitPoint, hitActor);
	}
}

void ARPlayerBase::StopBasicAttack()
{
	FGameplayEventData eventData;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, URAbilityGenericTags::GetEndAttackTag(), eventData);
}

void ARPlayerBase::EnableScoping()
{
	GetAbilitySystemComponent()->PressInputID((int)EAbilityInputID::Scoping);
}

void ARPlayerBase::DisableScoping()
{
	FGameplayEventData eventData;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, URAbilityGenericTags::GetEndScopingTag(), eventData);
}

void ARPlayerBase::Scroll(const FInputActionValue& InputActionVal)
{

	float scrollAmount = InputActionVal.Get<float>();

	scrollAmount *= -1;

	DefaultCameraLocal.X += (scrollAmount * 100);

	DefaultCameraLocal.X = FMath::Clamp(DefaultCameraLocal.X, -800, -200);

	if (!bIsScoping)
	{
		GetWorldTimerManager().ClearTimer(CameraLerpHandle);
		viewCamera->SetRelativeLocation(DefaultCameraLocal);
	}
}

void ARPlayerBase::TryActivateSpecialAttack()
{
	GetWorldTimerManager().ClearTimer(CameraLerpHandle);

	GetAbilitySystemComponent()->PressInputID((int)EAbilityInputID::SpecialAttack);
}

void ARPlayerBase::FinishSpecialAttack()
{
	GetAbilitySystemComponent()->TargetConfirm();
}

void ARPlayerBase::TryActivateUltimateAttack()
{
	GetAbilitySystemComponent()->PressInputID((int)EAbilityInputID::UltimateAttack);
}

void ARPlayerBase::FinishUltimateAttack()
{
	GetAbilitySystemComponent()->TargetConfirm();

	//GetAbilitySystemComponent()->InputCancel();
}

void ARPlayerBase::Interact()
{
	PlayerInteraction.Broadcast();

	bool bLucky = false;
	if (interactionChest != nullptr)
	{
		 bLucky = CashMyLuck();
	}

	ServerRequestInteraction(interactionChest, escapeToWin, bLucky); // if there's lag this might not work... Reconsider your options carefully
}

void ARPlayerBase::Pause()
{
	/*ARGameMode* GameMode = GetWorld()->GetAuthGameMode<ARGameMode>();
	isPaused = !isPaused;

	if (isPaused)
	{
		GameMode->PausingGame(isPaused);
		//Remove all controls to the characters when they are paused.
	}
	else
	{
		GameMode->PausingGame(isPaused);
		//Return all controls to the characters when they are unpaused.
	}*/
}

void ARPlayerBase::LoadDebug()
{
	UWorld* World = GetWorld();
	
	if (World)
	{
		UGameplayStatics::OpenLevel(World, TEXT("EndGameTestRoom"));
	}
}

FVector ARPlayerBase::GetMoveFwdDir() const
{
	FVector CamerFwd = viewCamera->GetForwardVector();
	CamerFwd.Z = 0;
	return CamerFwd.GetSafeNormal();
}

FVector ARPlayerBase::GetMoveRightDir() const
{
	return viewCamera->GetRightVector();
}

void ARPlayerBase::ScopingTagChanged(bool bNewIsAiming)
{
	bUseControllerRotationYaw = bNewIsAiming;
	bIsScoping = bNewIsAiming;
	GetCharacterMovement()->bOrientRotationToMovement = !bNewIsAiming;


	if (IsValid(playerController))
		playerController->ChangeCrosshairState(bNewIsAiming);

	if (bNewIsAiming)
	{
		cameraClampMax = 50;
		cameraClampMin = -80;
		LerpCameraToLocalOffset(AimCameraLocalOffset);
	}
	else
	{
		DisableScoping();
		cameraClampMax = 50;
		cameraClampMin = -80;
		LerpCameraToLocalOffset(DefaultCameraLocal);
	}
}

void ARPlayerBase::LerpCameraToLocalOffset(const FVector& LocalOffset)
{
	GetWorldTimerManager().ClearTimer(CameraLerpHandle);
	CameraLerpHandle = GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ARPlayerBase::TickCameraLocalOffset, LocalOffset));
}

void ARPlayerBase::TickCameraLocalOffset(FVector Goal)
{
	FVector CurrentLocalOffset = viewCamera->GetRelativeLocation();
	if (FVector::Dist(CurrentLocalOffset, Goal) < 1)
	{
		viewCamera->SetRelativeLocation(Goal);
		return;
	}

	FVector NewLocalOffset = FMath::Lerp(CurrentLocalOffset, Goal, GetWorld()->GetDeltaSeconds() * AimCameraLerpingSpeed);
	viewCamera->SetRelativeLocation(NewLocalOffset);
	CameraLerpHandle = GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ARPlayerBase::TickCameraLocalOffset, Goal));
}

void ARPlayerBase::SetPausetoFalse()
{
	isPaused = false;
}

bool ARPlayerBase::CashMyLuck()
{
	if (playerController == nullptr || bFeelinLucky == false)
		return false;

	return playerController->CashMyLuck();
}

void ARPlayerBase::PlayPickupAudio()
{
	if (AudioComp && PickupAudio)
	{
		AudioComp->Play();
	}
}

void ARPlayerBase::GroundCheckCompOverlapped(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor != this && GetCharacterMovement()->IsFalling())
	{
		UE_LOG(LogTemp, Warning, TEXT("Grounded"));
		
		if (AudioComp && LandAudio)
		{
			AudioComp->SetSound(LandAudio);
			AudioComp->Play();
		}

	}
}

void ARPlayerBase::SetPlayerReviveState_Implementation(bool state)
{
	if (ReviveUI == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to set player revive UI"));
		return;
	}

	if (state)
	{
		if (IsLocallyControlled() == false)
		{
			ReviveUI->SetVisibility(ESlateVisibility::Visible);
		}
	}
	else 
	{
		ReviveUI->SetVisibility(ESlateVisibility::Collapsed);// do collasped instead of hidden
	}
}

void ARPlayerBase::ServerRequestPickupItem_Implementation(AItemPickup* itemPickup, URItemDataAsset* itemAsset)
{
	if (itemPickup && itemAsset)
	{
		itemPickup->Server_PickupItem();
	}
}

void ARPlayerBase::ServerRequestInteraction_Implementation(AItemChest* Chest, AEscapeToWin* winPoint, bool bLucky)
{
	if (Chest)
	{
		Chest->Server_OpenChest(bLucky);
	}

	if (winPoint)
	{
		if (!winPoint->bStartBoss)
		{
			winPoint->CheckKeyCard();
		}
		else
		{
			winPoint->UseKeycard();
		}
	}
}

void ARPlayerBase::SetPlayerState()
{
	EOSPlayerState = Cast<AEOSPlayerState>(GetPlayerState());
	if (EOSPlayerState)
	{
		EOSPlayerState->Server_OnPossessPlayer(this);
	}
}

AEOSPlayerState* ARPlayerBase::GetPlayerBaseState() // this will crash if someone other than the player calls. Other players do not have this replicated.
{
	return EOSPlayerState;
}

void ARPlayerBase::SetInteractionChest(AItemChest* chest)
{
	interactionChest = chest;
}

void ARPlayerBase::SetInteractionWin(AEscapeToWin* winPoint)
{
	escapeToWin = winPoint;
}

void ARPlayerBase::SetItemPickup(AItemPickup* itemPickup, URItemDataAsset* itemAsset)
{
	ServerRequestPickupItem(itemPickup, itemAsset);
}

void ARPlayerBase::AddNewItem_Implementation(URItemDataAsset* newItemAsset)
{
	playerController->AddNewItemToUI(newItemAsset);
}

void ARPlayerBase::DeadStatusUpdated(bool bIsDead)
{
	if (bIsDead)
	{
		
	}
	else
	{
		
	}
}

void ARPlayerBase::InvisStatusUpdated(bool bIsDead)
{
	if (bIsDead)
	{
		if (DynamicTexMaterialInstance)
		{
			GetMesh()->SetMaterial(0, DynamicTexMaterialInstance);
		}
	}
	else
	{
		GetMesh()->SetMaterial(0, TexDefaultMat);
	}
}

void ARPlayerBase::FrameDelayItemPickup(AItemPickup* newItem)
{
	newItem->PlayerPickupRequest(this);
}

void ARPlayerBase::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (GetAbilitySystemComponent()->HasMatchingGameplayTag(URAbilityGenericTags::GetUnActionableTag()) || GetAbilitySystemComponent()->HasMatchingGameplayTag(URAbilityGenericTags::GetDeadTag())) return;

	AItemPickup* newItem = Cast<AItemPickup>(OtherActor);
	if (newItem)
	{
		PickupItemHandle = GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ARPlayerBase::FrameDelayItemPickup, newItem));
	}

	ARPlayerBase* player = Cast<ARPlayerBase>(OtherActor);
	if (player && player != this)
	{
		if (!nearbyFaintedActors.Contains(OtherActor))
			nearbyFaintedActors.Add(OtherActor);

		if (player->GetAbilitySystemComponent()->HasMatchingGameplayTag(URAbilityGenericTags::GetDeadTag()))
		{
			bInRangeToRevive = true;
			player->SetPlayerReviveState(true);
		}
	}
}

void ARPlayerBase::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ARPlayerBase* player = Cast<ARPlayerBase>(OtherActor);
	if (player && player != this)
	{
		if (nearbyFaintedActors.Contains(OtherActor))
			nearbyFaintedActors.Remove(OtherActor);

		if (player->GetAbilitySystemComponent()->HasMatchingGameplayTag(URAbilityGenericTags::GetDeadTag()))
		{
			bInRangeToRevive = false;
			player->SetPlayerReviveState(false);
		}
	}
}
