// Fill out your copyright notice in the Description page of Project Settings.

#include "Editor/DefaultPawnRestricted.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "WildCard.h"
#include "InGame/Game/InGameController.h"
#include "InGame/Game/InGamePlayerState.h"
#include "InGame/Game/InGameMode.h"
#include "InGame/Game/InGameState.h"
#include "InGame/Data/BattleData.h"
#include "InGame/Unit/UnitBase.h"
#include "InGame/Map/Tile.h"
#include "InGame/Effect/EffectBase.h"
#include "Editor/SaveCameraTransform/CameraTransfromDataAsset.h"
#include "UObject/ConstructorHelpers.h"

ADefaultPawnRestricted::ADefaultPawnRestricted()
{
	bIsYawLocked = true;
	bAutoApplyCameraOnBeginPlay = true;
	DefaultCameraIndex = 0;

	static ConstructorHelpers::FObjectFinder<UCameraTransfromDataAsset> CameraAssetFinder(TEXT("/Game/Editor/SaveCameraTrasform/CameraTransform.CameraTransform"));
	if (CameraAssetFinder.Succeeded())
	{
		CameraDataAsset = CameraAssetFinder.Object;
	}
}

void ADefaultPawnRestricted::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoApplyCameraOnBeginPlay && GetNetMode() != NM_DedicatedServer)
	{
		ApplySavedCameraTransform(DefaultCameraIndex);
	}
}

void ADefaultPawnRestricted::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (bAutoApplyCameraOnBeginPlay && GetNetMode() != NM_DedicatedServer)
	{
		ApplySavedCameraTransform(DefaultCameraIndex);
	}
}

void ADefaultPawnRestricted::Restart()
{
	Super::Restart();

	if (bAutoApplyCameraOnBeginPlay && IsLocallyControlled())
	{
		ApplySavedCameraTransform(DefaultCameraIndex);
	}
}

void ADefaultPawnRestricted::ApplySavedCameraTransform(int32 Index)
{
	if (!CameraDataAsset)
	{
		CameraDataAsset = LoadObject<UCameraTransfromDataAsset>(nullptr, TEXT("/Game/Editor/SaveCameraTrasform/CameraTransform.CameraTransform"));
	}

	if (!CameraDataAsset || !CameraDataAsset->SavedTransforms.IsValidIndex(Index))
	{
		return;
	}

	const FTransform& TargetTransform = CameraDataAsset->SavedTransforms[Index];
	SetActorLocationAndRotation(TargetTransform.GetLocation(), TargetTransform.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetControlRotation(TargetTransform.Rotator());
	}
}

void ADefaultPawnRestricted::Server_SpawnUnitActor_Implementation(FCardData InCardData, ATile* InTile, TSubclassOf<AUnitBase> InUnitClass)
{
	if (!HasAuthority()) return;

	AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>();
	WILD_CHECK(PS);

	int32 PlayerId = PS->GetPlayerId();
	if (InTile)
	{
		//InTile->SpawnUnit(InCardData, InUnitClass, PlayerId, CardInstanceID);
	}

	WILD_SIMPLE(Warning);
	APlayerController* APC = Cast<APlayerController>(GetController());
	AInGameController* PC = Cast<AInGameController>(APC);
	if (!PC) return;

	FWildActionData ActionData;
	ActionData.PlayerID = PlayerId;
	ActionData.ActionType = EWildActionType::SummonUnit;
	ActionData.CardInstanceID = 0;
	ActionData.TargetTile = InTile;

	PC->CompletePrivateAction(ActionData);
}

bool ADefaultPawnRestricted::Server_SpawnUnitActor_Validate(FCardData InCardData, ATile* InTile, TSubclassOf<AUnitBase> InUnitClass)
{
	return true;
}

void ADefaultPawnRestricted::Server_TestEffect_Implementation(UEffectBase* InEffect, FWildActionData InActionData)
{
	if (InEffect)
	{
		//InEffect->ExecuteEffect(InActionData);
	}
}

bool ADefaultPawnRestricted::Server_TestEffect_Validate(UEffectBase* InEffect, FWildActionData InActionData)
{
	return true;
}

void ADefaultPawnRestricted::Server_MoveUnitActor_Implementation(ATile* InToMoveTile, ATile* InTargetTile, AUnitBase* InUnitActor)
{
	WILD_SIMPLE(Warning);
	if (InTargetTile && InToMoveTile && InUnitActor)
	{
		InTargetTile->PlacedUnit = InUnitActor;
		InToMoveTile->PlacedUnit = nullptr;
		InUnitActor->SetActorLocation(InTargetTile->GetActorLocation());
	}
}

bool ADefaultPawnRestricted::Server_MoveUnitActor_Validate(ATile* InToMoveTile, ATile* InTargetTile, AUnitBase* InUnitActor)
{
	return true;
}

void ADefaultPawnRestricted::Server_FightUnitActor_Implementation(AUnitBase* InAttacker, AUnitBase* InDefender)
{
	if (!InAttacker || !InDefender) return;

	int32 AttackerAttack = InAttacker->GetCardAttack();
	int32 DefenderAttack = InDefender->GetCardAttack();
	int32 AttackerHealth = InAttacker->GetCardHealth();
	int32 DefenderHealth = InDefender->GetCardHealth();

	InAttacker->SetCardHealth(AttackerHealth - DefenderAttack);
	InDefender->SetCardHealth(DefenderHealth - AttackerAttack);
}

bool ADefaultPawnRestricted::Server_FightUnitActor_Validate(AUnitBase* InAttacker, AUnitBase* InDefender)
{
	return true;
}

void ADefaultPawnRestricted::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);
	
	PlayerInputComponent->BindAction("LockCamera", IE_Pressed, this, &ADefaultPawnRestricted::ToggleYawLock);
}

void ADefaultPawnRestricted::AddControllerYawInput(float Val)
{
	if (!bIsYawLocked)
	{
		Super::AddControllerYawInput(Val);
	}
}

void ADefaultPawnRestricted::ToggleYawLock()
{
	bIsYawLocked = !bIsYawLocked;
}
