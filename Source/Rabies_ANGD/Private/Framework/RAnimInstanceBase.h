// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "RAnimInstanceBase.generated.h"

class ACharacter;
class UCharacterMovementComponent;
/**
 * 
 */
UCLASS()
class URAnimInstanceBase : public UAnimInstance
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = "Animation", meta = (BlueprintThreadSafe))
	float GetSpeed() const { return Speed; }

	UFUNCTION(BlueprintCallable, Category = "Animation", meta = (BlueprintThreadSafe))
	bool IsMoving() const { return Speed != 0; }

	UFUNCTION(BlueprintCallable, Category = "Animation", meta = (BlueprintThreadSafe))
	bool IsNotMoving() const { return Speed == 0; }

	UFUNCTION(BlueprintCallable, Category = "Animation", meta = (BlueprintThreadSafe))
	bool IsJumping() const { return bIsJumping; }

	UFUNCTION(BlueprintCallable, Category = "Animation", meta = (BlueprintThreadSafe))
	bool IsOnGround() const { return !bIsJumping; }

	UFUNCTION(BlueprintCallable, Category = "Animation", meta = (BlueprintThreadSafe))
	FRotator GetLookOffset() const { return LookOffset; }

	UFUNCTION(BlueprintCallable, Category = "Animation", meta = (BlueprintThreadSafe))
	float GetYawSpeed() const { return YawSpeed; }

	UFUNCTION(BlueprintCallable, Category = "Animation", meta = (BlueprintThreadSafe))
	bool ShouldDoUpperBody() const;

	UFUNCTION(BlueprintCallable, Category = "Animation", meta = (BlueprintThreadSafe))
	bool GetIsAiming() const { return bIsScoping; }

	UFUNCTION(BlueprintCallable, Category = "Animation", meta = (BlueprintThreadSafe))
	float GetFwdSpeed() const { return FwdSpeed; }

	UFUNCTION(BlueprintCallable, Category = "Animation", meta = (BlueprintThreadSafe))
	float GetRightSpeed() const { return RightSpeed; }

	UFUNCTION(BlueprintCallable, Category = "Animation", meta = (BlueprintThreadSafe))
	bool GetRangedAttacking() const { return bAttacking && bIsScoping; }
private:

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* StartMontage;

	// the below functions are the native overrides for each phase
	// Native initialization override point BEGIN PLAY
	virtual void NativeInitializeAnimation() override;

	// Native thread safe update override point. Executed on a worker thread just prior to graph update 
	// for linked anim instances, only called when the hosting node(s) are relevant TICK
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	const ACharacter* OwnerCharacter;
	const UCharacterMovementComponent* OwnerMovementComp;

	float Speed;
	bool bIsJumping;
	FRotator LookOffset;

	FRotator PrevRot;
	float YawSpeed;

	float FwdSpeed;
	float RightSpeed;

	bool bIsScoping;

	bool bAttacking;

	void AttackingTagChanged(const FGameplayTag TagChanged, int32 NewStackCount);
	void ScopingTagChanged(const FGameplayTag TagChanged, int32 NewStackCount);
};
