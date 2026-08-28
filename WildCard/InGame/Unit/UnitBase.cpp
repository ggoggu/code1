// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Unit/UnitBase.h"

// Sets default values
AUnitBase::AUnitBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AUnitBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AUnitBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

int32 AUnitBase::GetOwnerId()
{
	//return UnitCardData.OwnerId;
	return 1;
}

