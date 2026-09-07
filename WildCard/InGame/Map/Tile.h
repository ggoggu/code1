// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InGameHexagon.h"
#include "Tile.generated.h"

class UTextRenderComponent;
class AUnitBase;
class AInGameController;
struct FCardData;

UENUM(BlueprintAble)
enum class ETileState : uint8
{
	Idle,
	None,
	Moveable,
	Spawnable,
	MAX
};

UCLASS()
class WILDCARD_API ATile : public AActor
{
	GENERATED_BODY()
	
public:	
	ATile();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// 타일 상태(색상) 변경
	UFUNCTION(BlueprintImplementableEvent)
	void SetTileState(int State);

	UFUNCTION(BlueprintImplementableEvent)
	void SetIdxText(int32 InQ, int32 InR);

	// 타일 위에 올려진 유닛 지정
	UFUNCTION(BlueprintCallable)
	void SetPlacedUnit(AUnitBase* InPlacedUnit) { PlacedUnit = InPlacedUnit; }

	// 타일에 유닛 소환
	UFUNCTION(BlueprintCallable)
	void SpawnUnit(FCardData InCardData, TSubclassOf<AUnitBase> UnitClass, int32 InPlayerCount);

	// 소환 된 유닛을 다른 타일로 유닛 이동
	UFUNCTION(BlueprintCallable)
	void MoveToUnit(AUnitBase* Unit, FVector PositionToMove);

	UFUNCTION()
	void OnRep_TileState();

	UFUNCTION(BlueprintCallable)
	void SetUnitMoveToTileInBP(FVector InHasUnitTileLocation, FVector InEmptyTileLocation);

	UFUNCTION(BlueprintCallable)
	void SetUnitAttackTargetInBP(const FString& InAttacker, const FString& InDefender);

	UFUNCTION(BlueprintCallable)
	bool HasOwnerIDtoActionUnit(int32 InOwnerId);

	UFUNCTION()
	void OnRep_MoveUnit();

	UFUNCTION()
	void OnRep_Axial();

	bool HasUnit() const { return (PlacedUnit != nullptr); }

	AUnitBase* GetUnit() const { return PlacedUnit; }

	ETileState GetTileState() const { return TileState; }

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	UPROPERTY(ReplicatedUsing = OnRep_Axial, BlueprintReadOnly, Category = "Tile")
	FAxial Axial;

	UPROPERTY(ReplicatedUsing = OnRep_MoveUnit, VisibleAnywhere, BlueprintReadOnly, Category = "Tile")
	TObjectPtr<AUnitBase> PlacedUnit;

	// tileState 확인을 위한 임시 Material
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
	TObjectPtr<UMaterialInterface> MNone;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
	TObjectPtr<UMaterialInterface> MIdle;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
	TObjectPtr<UMaterialInterface> MSpawnable;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
	TObjectPtr<UMaterialInterface> MMoveable;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_TileState, EditAnywhere, BlueprintReadWrite, Category = "Tile")
	ETileState TileState;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Tile")
	TObjectPtr<AInGameController> PlayerController;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
