// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Card/CardBase.h"

// Sets default values
ACardBase::ACardBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACardBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACardBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACardBase::InitializeCard(int32 instaceid, FName cardid, int32 currentmanacost, int32 currentforce,
	int32 currentintelligence, int32 currentattack, int32 currenthealth)
{
	InstanceID = instaceid;
	CardID = cardid;
	CurrentManaCost = currentmanacost;
	CurrentForce = currentforce;
	CurrentIntelligence = currentintelligence;
	CurrentAttack = currentattack;
	CurrentHealth = currenthealth;
}

