// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TileMapMaker.generated.h"

class ATile;
class ATileMap;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MapInfo")
	int32 Width;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MapInfo")
	int32 Height;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MapInfo")
	float Size;
};
