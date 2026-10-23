// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Unit/WebTrap.h"
#include "UnitBase.h"

void AWebTrap::ExecuteTrap_Implementation(AUnitBase* OverlapedUnit)
{
	OverlapedUnit->SetCardAttack(OverlapedUnit->GetCardAttack() - 1);
}
