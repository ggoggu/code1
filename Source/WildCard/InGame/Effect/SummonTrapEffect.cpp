// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Effect/SummonTrapEffect.h"
#include "SummonTrapEffect.h"
#include "Tile.h"
#include "WebTrap.h"
#include "UnitBase.h"

void USummonTrapEffect::ExecuteEffect_Implementation(const FWildActionData& ActionData, const FEffectData& InEffectData)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = ActionData.SourceTile->GetUnit();

	AWebTrap* NewWebTrap = GetWorld()->SpawnActor<AWebTrap>(
		ClassToSpawn,
		ActionData.TargetTile->GetActorLocation(),
		FRotator::ZeroRotator,
		SpawnParams
	);
}
