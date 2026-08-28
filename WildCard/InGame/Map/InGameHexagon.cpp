// Fill out your copyright notice in the Description page of Project Settings.

#include "InGame/Map/InGameHexagon.h"

const TArray<FVector> FCube::Directions = {
	FVector(1, -1, 0), FVector(1, 0, -1), FVector(0, 1, -1),
	FVector(-1, 1, 0), FVector(-1, 0, 1), FVector(0, -1, 1)
};

FCube::FCube(const FAxial& Axial)
{
	Q = Axial.Q;
	R = Axial.R;
	S = -Axial.Q - Axial.R;
}

FAxial FCube::ToAxial() const
{
	return FAxial(Q, R);
}

FAxial::FAxial(const FCube& Cube)
{
	Q = Cube.Q;
	R = Cube.R;
}

FCube FAxial::ToCube() const
{
	return FCube(Q, R, -Q - R);
}