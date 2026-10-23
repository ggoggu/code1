// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InGameHexagon.h"
#include "Tile.generated.h"

class UTextRenderComponent;
class AUnitBase;
class APlayerUnit;
class AInGamePlayerState;
class AInGameController;
enum class EWildPlayerClass : uint8;
struct FCardData;

UENUM(BlueprintType)
enum class ETileState : uint8
{
	Idle = 0        UMETA(DisplayName = "Idle"),
	Combatable = 1  UMETA(DisplayName = "Combatable"),
	Moveable = 2    UMETA(DisplayName = "Moveable"),
	Spawnable = 3   UMETA(DisplayName = "Spawnable"),
	Effectable = 4  UMETA(DisplayName = "Effectable"),
	None = 5        UMETA(DisplayName = "None"),
	MAX             UMETA(Hidden)
};

UCLASS()
class WILDCARD_API ATile : public AActor
{
	GENERATED_BODY()
	
public:	
	ATile();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// 타일 상태(색상) 변경 (블루프린트 구현 이벤트)
	UFUNCTION(BlueprintImplementableEvent)
	void SetTileState(int State);

	/** 강타입 Enum 기반 타일 상태 변경 헬퍼 */
	FORCEINLINE void SetTileStateEnum(ETileState State)
	{
		SetTileState(static_cast<int>(State));
	}

	UFUNCTION(BlueprintImplementableEvent)
	void SetIdxText(int32 InQ, int32 InR);

	// 타일 호버(감지) 시각 효과 처리
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Tile")
	void SetTileHovered(bool bHovered);
	virtual void SetTileHovered_Implementation(bool bHovered);

	UFUNCTION(BlueprintCallable, Category = "Tile")
	bool IsTileHovered() const { return bIsTileHovered; }

	// 타일 위에 올려진 유닛 지정
	UFUNCTION(BlueprintCallable)
	void SetPlacedUnit(AUnitBase* InPlacedUnit);

	// 타일에 유닛 소환
	UFUNCTION(BlueprintCallable)
	void SpawnUnit(FCardData InCardData, TSubclassOf<AUnitBase> UnitClass, int32 InPlayerCount, int32 CardInstanceID);

	// 타일에 플레이어 영웅 유닛 소환
	UFUNCTION(BlueprintCallable, Category = "Tile")
	APlayerUnit* SpawnPlayerUnit(TSubclassOf<APlayerUnit> PlayerUnitClass, AInGamePlayerState* InPlayerState, EWildPlayerClass InClass);

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

	UFUNCTION(BlueprintCallable)
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

	// 카드/마우스 호버 시 사용할 머티리얼 및 상태
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile|Visual")
	TObjectPtr<UMaterialInterface> MHovered;

	// 카드/마우스 호버 시 변경될 하이라이트 색상 (기본: 선명한 오렌지 골드)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile|Visual")
	FLinearColor HoverColor = FLinearColor(1.0f, 0.55f, 0.0f, 1.0f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile|Visual")
	bool bIsTileHovered = false;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> CachedMaterialBeforeHover;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> HoverDynamicMaterial;

	// 타일의 기본 스태틱 메시 컴포넌트(BP_Tile의 Cube 등) 반환 및 캐싱
	UFUNCTION(BlueprintCallable, Category = "Tile|Visual")
	UStaticMeshComponent* GetBaseStaticMeshComponent();

	UFUNCTION(BlueprintCallable, Category = "Tile|Visual")
	void SetHoverColor(FLinearColor InColor) { HoverColor = InColor; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile")
	ETileState TileState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile")
	TObjectPtr<AInGameController> PlayerController;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
