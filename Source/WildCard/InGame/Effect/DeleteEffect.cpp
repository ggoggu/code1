// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Effect/DeleteEffect.h"
#include "Tile.h"
#include "UnitBase.h"

void UDeleteEffect::ExecuteEffect_Implementation(const FWildActionData& ActionData, const FEffectData& InEffectData)
{
	ActionData.TargetTile->GetUnit()->SetCardHealth(0);
}
