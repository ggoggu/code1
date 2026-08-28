// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InGameMapMaker.generated.h"

class ATile;
class AInGameMap;

UCLASS()
class WILDCARD_API AInGameMapMaker : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AInGameMapMaker();

	UFUNCTION(BlueprintCallable)
	AInGameMap* CreateMap(FVector Location);

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MapInfo")
	TSubclassOf<ATile> WildTileClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MapInfo")
	TSubclassOf<AInGameMap> WildMapClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MapInfo")
	int Width;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MapInfo")
	int Height;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MapInfo")
	int Size;

protected:
	virtual void BeginPlay() override;
	

};
