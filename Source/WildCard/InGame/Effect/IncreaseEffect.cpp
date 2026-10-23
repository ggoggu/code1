// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Effect/IncreaseEffect.h"
#include "IncreaseEffect.h"
#include "Tile.h"
#include "UnitBase.h"

void UIncreaseEffect::ExecuteEffect_Implementation(const FWildActionData& ActionData, const FEffectData& InEffectData)
{
	switch (SelectedNum)
	{
	case 0:
		ActionData.TargetTile->GetUnit()->SetCardAttack(ActionData.TargetTile->GetUnit()->GetCardAttack() + Value);
		break;
	case 1:
		ActionData.TargetTile->GetUnit()->SetCardHealth(ActionData.TargetTile->GetUnit()->GetCardHealth() + Value);
		break;
	case 2:
		ActionData.TargetTile->GetUnit()->SetCardAttack(ActionData.TargetTile->GetUnit()->GetCardAttack() * Value);
		break;
	case 3:
		ActionData.TargetTile->GetUnit()->SetCardHealth(ActionData.TargetTile->GetUnit()->GetCardHealth() * Value);
		break;
	case 4:
		ActionData.TargetTile->GetUnit()->SetCardAttack(ActionData.TargetTile->GetUnit()->GetCardAttack() - Value);
		break;
	case 5:
		ActionData.TargetTile->GetUnit()->SetCardHealth(ActionData.TargetTile->GetUnit()->GetCardHealth() - Value);
		break;
	case 6:
		ActionData.TargetTile->GetUnit()->SetCardAttack(ActionData.TargetTile->GetUnit()->GetCardAttack() * Value);
		ActionData.TargetTile->GetUnit()->SetCardHealth(ActionData.TargetTile->GetUnit()->GetCardHealth() * Value);
		break;
	}
}
