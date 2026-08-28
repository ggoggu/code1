// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CardBase.generated.h"

UCLASS()
class WILDCARD_API ACardBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACardBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void InitializeCard(int32 instaceid, FName cardid, int32 currentmanacost, int32 currentforce, int32 currentintelligence, int32 currentattack, int32 currenthealth);

public:
	int32 InstanceID;
	FName CardID;
	int32 CurrentManaCost;
	int32 CurrentForce;
	int32 CurrentIntelligence;
	int32 CurrentAttack;
	int32 CurrentHealth;
	

};
