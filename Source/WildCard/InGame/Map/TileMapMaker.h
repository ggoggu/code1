// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TileMapMaker.generated.h"

class ATile;
class ATileMap;
class UTileLayoutDataAsset;

UCLASS()
class WILDCARD_API ATileMapMaker : public AActor
{
	GENERATED_BODY()
	
public:	
	ATileMapMaker();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	ATileMap* CreateMap(FVector Location);

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MapInfo")
	TSubclassOf<ATile> WildTileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MapInfo")
	TSubclassOf<ATileMap> WildMapClass;

	// Required layout: dimensions and enabled cells come from this asset.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MapInfo")
	TObjectPtr<UTileLayoutDataAsset> LayoutData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MapInfo", meta = (ClampMin = "0.01"))
	float Size;
};
