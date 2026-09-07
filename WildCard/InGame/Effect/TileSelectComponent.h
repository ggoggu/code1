// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TileSelectComponent.generated.h"

class ATile;
class AUnitBase;
class AIngameController;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMoveableTilesFound, TWeakObjectPtr<ATile> InTile);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCombatableTilesFound, TWeakObjectPtr<ATile> InTile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActionMenuRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActionMenuClosed);

UENUM(BlueprintType)
enum class ETileInteractionState : uint8
{
	Idle                  UMETA(DisplayName = "대기 (선택 없음)"),
	ActionMenuOpen        UMETA(DisplayName = "행동 메뉴 열림 (UI 선택 대기중)"),
	AwaitingMoveTarget    UMETA(DisplayName = "이동 목표 타일 대기중"),
	AwaitingAttackTarget  UMETA(DisplayName = "공격 목표 타일 대기중")
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class WILDCARD_API UTileSelectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UTileSelectComponent();

	UFUNCTION(BlueprintCallable, Category = "Tile Interaction")
	void ProcessTileClick(ATile* InClickedTile);

	UFUNCTION(BlueprintCallable, Category = "Tile Interaction")
	void SelectMoveAction();

	UFUNCTION(BlueprintCallable, Category = "Tile Interaction")
	void SelectAttackAction();

	UFUNCTION(BlueprintCallable, Category = "Tile Interaction")
	void CancelAction();

	void SetCurrentState(ETileInteractionState InState) { CurrentState = InState; };
	ETileInteractionState GetCurrentState() { return CurrentState; };

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// 초기화용 내부 함수
	void ClearSelect();

	// 실제 행동을 담당할 함수들
	void ExecuteMove(ATile* TargetTile);
	void ExecuteCombat(AUnitBase* Attacker, AUnitBase* Defender);
	void ExecuteEffect();



public:
	FOnActionMenuRequested OnActionMenuRequested;
	FOnActionMenuClosed OnActionMenuClosed;
	FOnMoveableTilesFound OnMoveableTilesFound;
	FOnMoveableTilesFound OnMoveableTilesCancled;
	FOnCombatableTilesFound OnCombatableTilesFound;
	FOnCombatableTilesFound OnCombatableTilesCancled;


protected:
	// 현재 상태
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile Interaction|State")
	ETileInteractionState CurrentState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile Interaction|Data")
	ATile* SelectedTile;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile Interaction|Data")
	AUnitBase* SelectedUnit;
};