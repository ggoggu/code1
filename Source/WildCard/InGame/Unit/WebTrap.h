// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InGame/Unit/TrapBase.h"
#include "WebTrap.generated.h"

/**
 * 
 */
UCLASS()
class WILDCARD_API AWebTrap : public ATrapBase
{
	GENERATED_BODY()

public:
	virtual void ExecuteTrap_Implementation(AUnitBase* OverlapedUnit) override;
	
};
