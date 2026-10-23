// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Effect/HealEffect.h"
#include "Tile.h"
#include "UnitBase.h"

void UHealEffect::ExecuteEffect_Implementation(const FWildActionData& ActionData, const FEffectData& InEffectData)
{
	if (ActionData.TargetTile && ActionData.TargetTile->GetUnit())
	{
		AUnitBase* TargetUnit = ActionData.TargetTile->GetUnit();
		TargetUnit->SetCardHealth(TargetUnit->GetCardHealth() + Value);

		FWildVisualCueData CueData;
		CueData.CueTag = InEffectData.VisualCueTag.IsValid() 
			? InEffectData.VisualCueTag 
			: FGameplayTag::RequestGameplayTag(FName("Visual.Effect.Heal"), false);
		CueData.InstigatorPlayerID = ActionData.PlayerID;
		CueData.TargetActor = TargetUnit;
		CueData.NumericValue = Value;
		if (ActionData.TargetTile)
		{
			CueData.TargetQ = ActionData.TargetTile->Axial.Q;
			CueData.TargetR = ActionData.TargetTile->Axial.R;
		}

		BroadcastVisualCue(CueData);
	}
}
