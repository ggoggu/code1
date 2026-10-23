// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TileLayoutDataAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class WILDCARD_API UTileLayoutDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
    // 격자의 가로 칸 수 : 기본값
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout", meta = (ClampMin = "1", ClampMax = "64"))
    int32 Width = 10;

    // 격자의 세로 칸 수 : 기본값
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout", meta = (ClampMin = "1", ClampMax = "64"))
    int32 Height = 10;

    // 실제로 생성할 칸 목록. X = 행, Y = 열.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
    TArray<FIntPoint> EnabledCells;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
    FIntPoint Player1Start = FIntPoint(-1, -1);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layout")
    FIntPoint Player2Start = FIntPoint(-1, -1);
};
