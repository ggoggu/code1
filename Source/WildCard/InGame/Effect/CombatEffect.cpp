// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Effect/CombatEffect.h"
#include "InGameState.h"
#include "TileMap.h"
#include "Tile.h"
#include "UnitBase.h"
#include "InGamePlayerState.h"

void UCombatEffect::ExecuteEffect_Implementation(const FWildActionData& ActionData, const FEffectData& InEffectData)
{
	AInGameState* GS = GetWorld()->GetGameState<AInGameState>();
	TArray<ATile*> NearbyTiles = GS->GetMap()->GetTilesAtDistance(ActionData.SourceTile, 1);

	TArray<AUnitBase*> NearbyUnits;
	for (ATile* Tile : NearbyTiles)
	{
		if (Tile)
		{
			if (Tile->GetUnit()->GetOwnerId() != ActionData.PlayerID)
			{
				AUnitBase* UnitsOnTile = Tile->GetUnit();
				NearbyUnits.Add(UnitsOnTile);
			}
		}
	}

	AUnitBase* TargetUnit = ActionData.TargetTile->GetUnit();
	AUnitBase* SourceUnit = ActionData.SourceTile->GetUnit();

	TargetUnit->SetCardHealth(TargetUnit->GetCardHealth() - SourceUnit->GetCardAttack());
	SourceUnit->SetCardHealth(SourceUnit->GetCardHealth() - TargetUnit->GetCardAttack());
}