// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TileSelectComponent.generated.h"

class ATile;
class AUnitBase;
class APlayerUnit;
class AInGameController;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMoveableTilesFound, TWeakObjectPtr<ATile> InTile);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCombatableTilesFound, TWeakObjectPtr<ATile> InTile);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEffectableTilesFound, TWeakObjectPtr<ATile> InTile);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTacticalSkillTilesFound, TWeakObjectPtr<ATile> InTile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActionMenuRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActionMenuClosed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerUnitActionMenuRequested, APlayerUnit*, Unit);

UENUM(BlueprintType)
enum class ETileInteractionState : uint8
{
	Idle                  UMETA(DisplayName = "대기"),
	ActionMenuOpen        UMETA(DisplayName = "행동 메뉴 오픈"),
	AwaitingMoveTarget    UMETA(DisplayName = "이동 대상 타일 대기"),
	AwaitingAttackTarget  UMETA(DisplayName = "공격 대상 타일 대기"),
	AWaitingEffectTarget  UMETA(DisplayName = "이펙트 대상 타일 대기"),
	AwaitingTacticalSkillTarget UMETA(DisplayName = "전술 스킬 대상 타일 대기")
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class WILDCARD_API UTileSelectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTileSelectComponent();

	UFUNCTION(BlueprintCallable, Category = "Tile Interaction")
	void ProcessTileClick(ATile* InClickedTile);

	UFUNCTION(BlueprintCallable, Category = "Tile Interaction")
	void SelectMoveAction();

	UFUNCTION(BlueprintCallable, Category = "Tile Interaction")
	void SelectAttackAction();

	UFUNCTION(BlueprintCallable, Category = "Tile Interaction")
	void SelectEffectAction();

	UFUNCTION(BlueprintCallable, Category = "Tile Interaction")
	void SelectTacticalSkillAction();

	UFUNCTION(BlueprintCallable, Category = "Tile Interaction")
	void CancelAction();

	void SetCurrentState(ETileInteractionState InState) { CurrentState = InState; }
	ETileInteractionState GetCurrentState() const { return CurrentState; }

	ATile* GetSelectedTile() const { return SelectedTile; }
	AUnitBase* GetSelectedUnit() const { return SelectedUnit; }
	APlayerUnit* GetSelectedPlayerUnit() const;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	void ClearSelect();

	void ExecuteMove(ATile* TargetTile);
	void ExecuteCombat(AUnitBase* Attacker, AUnitBase* Defender);
	void ExecuteEffect(ATile* TargetTile);
	void ExecuteTacticalSkill(ATile* TargetTile);

public:
	FOnActionMenuRequested OnActionMenuRequested;
	FOnActionMenuClosed OnActionMenuClosed;
	FOnPlayerUnitActionMenuRequested OnPlayerUnitActionMenuRequested;

	FOnMoveableTilesFound OnMoveableTilesFound;
	FOnMoveableTilesFound OnMoveableTilesCancled;
	FOnCombatableTilesFound OnCombatableTilesFound;
	FOnCombatableTilesFound OnCombatableTilesCancled;
	FOnEffectableTilesFound OnEffectableTilesFound;
	FOnEffectableTilesFound OnEffectableTilesCancled;
	FOnTacticalSkillTilesFound OnTacticalSkillTilesFound;
	FOnTacticalSkillTilesFound OnTacticalSkillTilesCancled;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile Interaction|State")
	ETileInteractionState CurrentState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile Interaction|Data")
	TObjectPtr<ATile> SelectedTile;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile Interaction|Data")
	TObjectPtr<AUnitBase> SelectedUnit;
};