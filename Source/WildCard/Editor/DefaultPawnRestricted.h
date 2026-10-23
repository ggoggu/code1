// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DefaultPawn.h"
#include "InGame/Data/BattleData.h"
#include "DefaultPawnRestricted.generated.h"

class ATile;
class AUnitBase;
class UEffectBase;
class UCameraTransfromDataAsset;

/**
 * 
 */
UCLASS()
class WILDCARD_API ADefaultPawnRestricted : public ADefaultPawn
{
	GENERATED_BODY()

public:
	ADefaultPawnRestricted();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<UCameraTransfromDataAsset> CameraDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	bool bAutoApplyCameraOnBeginPlay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	int32 DefaultCameraIndex;

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ApplySavedCameraTransform(int32 Index);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SpawnUnitActor(FCardData InCardData, ATile* InTile, TSubclassOf<AUnitBase> InUnitClass);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_TestEffect(UEffectBase* InEffect, FWildActionData InActionData);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveUnitActor(ATile* InToMoveTile, ATile* InTargetTile, AUnitBase* InUnitActor);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_FightUnitActor(AUnitBase* InAttacker, AUnitBase* InDefender);

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void Restart() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	virtual void AddControllerYawInput(float Val) override;

private:
	bool bIsYawLocked;
	void ToggleYawLock();
};
