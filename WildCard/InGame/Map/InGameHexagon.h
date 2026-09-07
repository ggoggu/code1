// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InGameHexagon.generated.h"

USTRUCT(BlueprintAble)
struct FCube
{
	GENERATED_BODY()

	int Q;
	int R;
	int S;

	static const TArray<FVector> Directions;

	FCube() : Q(0), R(0), S(0) {}
	FCube(int q, int r, int s) : Q(q), R(r), S(s) {}
	FCube(const FAxial& Axial);

	FAxial ToAxial() const;
};

USTRUCT(BlueprintAble, BlueprintType)
struct FAxial
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Q;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 R;

	FAxial() : Q(0), R(0){}
	FAxial(int32 q, int32 r) : Q(q), R(r) {}
	FAxial(const FCube& Cube);
	FCube ToCube() const;
	// flat-top , array-to-array �϶� index
	inline TTuple<int, int> ToIndex() const
	{
		return TTuple<int, int>(Q, R + floor((float)Q / 2.f));
	}

	static inline FAxial IndexToAxial(TTuple<int, int> Idx)
	{
		return FAxial(Idx.Key, Idx.Value - floor(Idx.Key / 2.f));
	}
};



/**
 * 
 */
USTRUCT(BlueprintAble)
struct FHexagon
{
	GENERATED_BODY()

	static FCube CubeNeighbor(FCube Cube, int Direction)
	{
		return FCube(Cube.Q + FCube::Directions[Direction][0], Cube.R + FCube::Directions[Direction][1], Cube.S + FCube::Directions[Direction][2]);
	}


};
