// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InGameHexagon.h"
#include "Tile.h"
#include "TileMap.generated.h"

class ATile;
class AUnitBase;
struct FAxial;

UCLASS()
class WILDCARD_API ATileMap : public AActor
{
	GENERATED_BODY()
	
public:	
	ATileMap();

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_InitialSeed();

	void InitSize(int32 Width, int32 Height);
	void AddTile(TSubclassOf<ATile> TileClass, TTuple<int32, int32> Idx, float Size);

	TArray<ATile*> GetTilesAtDistance(TWeakObjectPtr<ATile> InCenterTile, int32 Distance);

	ATile* GetTileAt(FAxial AxialCoord);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY()
	TObjectPtr<ATile> SingleTile;

	TArray<TArray<ATile*>> Tiles;

	UPROPERTY(ReplicatedUsing = OnRep_InitialSeed)
	int32 InitialSeed;

	FRandomStream MapRandomStream;

	const TArray<FAxial> EvenHexDirections = {
		FAxial(1, -1),   // 0: 오른쪽 위(Top Right)
		FAxial(0, -1),  // 1: 오른쪽 아래 (Bottom Right)
		FAxial(-1, 0),  // 2: 아래 (Bottom)
		FAxial(0, 1),   // 3: 왼쪽 아래(Bottom Left)
		FAxial(1, 1),   // 4: 왼쪽 위 (Top Left)
		FAxial(1, 0)    // 5: 위 (Top)
	};

	const TArray<FAxial> OddHexDirections = {
		FAxial(0, -1),   // 0: 오른쪽 위(Top Right)
		FAxial(-1, -1),  // 1: 오른쪽 아래 (Bottom Right)
		FAxial(-1, 0),  // 2: 아래 (Bottom)
		FAxial(-1, 1),  // 3: 왼쪽 아래(Bottom Left)
		FAxial(0, 1),   // 4: 왼쪽 위 (Top Left)
		FAxial(1, 0)    // 5: 위 (Top)
	};
};
