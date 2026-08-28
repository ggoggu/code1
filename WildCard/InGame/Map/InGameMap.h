// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InGameHexagon.h"
#include "InGameMap.generated.h"

class ATile;
struct FAxial;
class ACardBase;

UCLASS()
class WILDCARD_API AInGameMap : public AActor
{
	GENERATED_BODY()
	
public:	
	AInGameMap();

protected:
	virtual void BeginPlay() override;

public:	
	UFUNCTION(BlueprintCallable, Category="TileControl")
	void SetAllTileIdle();

	UFUNCTION(BlueprintCallable, Category = "TileControl")
	void SetAllTileMoveble();
	
	UFUNCTION(BlueprintCallable, Category = "TileControl")
	void SetAllTileRandom();


	UFUNCTION(BlueprintCallable, Category = "TileControl")
	void ShowMoveableFormCurrent(ATile* SelectTile);

	UFUNCTION(BlueprintCallable, Category = "TileControl")
	void ShowSpawnableFormCurrent(ATile* SelectTile);

	UFUNCTION(BlueprintCallable, Category = "TileControl")
	void SetTileSpawnable(TArray<ATile*>& SelectTiles);

	UFUNCTION(BlueprintCallable, Category = "TileControl")
	void SetTileMoveable(TArray<ATile*>& SelectTiles);

	UFUNCTION()
	void OnRep_InitialSeed();

	void InitSize(int Width, int Height);
	void AddTile(TSubclassOf<ATile> TileClass, TTuple<int, int> Idx, float Size);

	TArray<ATile*> GetTilesAtDistance(TWeakObjectPtr<ATile> InCenterTile, int32 Distance);

	ATile* GetTileAt(FAxial AxialCoord);

private: 
	ATile* SingleTile;
	 
	TArray<TArray<ATile*>> Tiles;

	UPROPERTY(Replicated)
	int32 InitialSeed;
	 
	FRandomStream MapRandomStream;

	const TArray<FAxial> EvenHexDirections = {
		FAxial(1, -1),   // 0: 오른쪽 위(Top Right)
		FAxial(0, -1),  // 1: 오른쪽 아래 (Bottom Right)
		FAxial(-1, 0),  // 2: 아래 (Bottom)
		FAxial(0, 1),  // 3: 왼쪽 아래(Bottom Left)
		FAxial(1, 1),  // 4: 왼쪽 위 (Top Left)
		FAxial(1, 0)    // 5: 위 (Top)
		};

	const TArray<FAxial> OddHexDirections = {
		FAxial(0, -1),   // 0: 오른쪽 위(Top Right)
		FAxial(-1, -1),  // 1: 오른쪽 아래 (Bottom Right)
		FAxial(-1, 0),  // 2: 아래 (Bottom)
		FAxial(-1, 1),  // 3: 왼쪽 아래(Bottom Left)
		FAxial(0, 1),  // 4: 왼쪽 위 (Top Left)
		FAxial(1, 0)    // 5: 위 (Top)
		};

};
