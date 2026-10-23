// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrapBase.generated.h"

class AUnitBase;

UCLASS()
class WILDCARD_API ATrapBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATrapBase();

	UFUNCTION(BlueprintNativeEvent, Category = "Trap Effect")
	void ExecuteTrap(AUnitBase* OverlapedUnit);
	virtual void ExecuteTrap_Implementation(AUnitBase* OverlapedUnit) PURE_VIRTUAL(ATrapBase::ExecuteTrap_Implementation, );


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
