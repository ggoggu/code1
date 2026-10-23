// Fill out your copyright notice in the Description page of Project Settings.

#include "InGame/Unit/HeroAbilityBase.h"
#include "InGame/Unit/PlayerUnit.h"
#include "InGame/Map/Tile.h"
#include "InGame/Game/InGamePlayerState.h"
#include "InGame/Battle/PlayerStatComponent.h"

UHeroAbilityBase::UHeroAbilityBase()
{
	AbilityName = FText::FromString(TEXT("기본 영웅 능력"));
	AbilityDescription = FText::FromString(TEXT("영웅 능력을 사용합니다."));
	ManaCost = 2;
	Range = 1;
	TargetType = EHeroAbilityTargetType::Self;
	bUsedThisTurn = false;
}

bool UHeroAbilityBase::CanActivate(APlayerUnit* SourceUnit, ATile* TargetTile) const
{
	if (bUsedThisTurn || !SourceUnit)
	{
		return false;
	}

	AInGamePlayerState* PS = SourceUnit->GetOwningPlayerState();
	if (PS && PS->GetStatComponent())
	{
		if (PS->GetStatComponent()->Mana < ManaCost)
		{
			return false;
		}
	}

	// Range check
	if (Range > 0 && TargetTile && SourceUnit->GetCurrentTile())
	{
		int32 dq = SourceUnit->GetCurrentTile()->Axial.Q - TargetTile->Axial.Q;
		int32 dr = SourceUnit->GetCurrentTile()->Axial.R - TargetTile->Axial.R;
		int32 HexDist = (FMath::Abs(dq) + FMath::Abs(dr) + FMath::Abs(dq + dr)) / 2;
		if (HexDist > Range)
		{
			return false;
		}
	}

	switch (TargetType)
	{
	case EHeroAbilityTargetType::Self:
		return true;

	case EHeroAbilityTargetType::TargetTile:
		return TargetTile != nullptr;

	case EHeroAbilityTargetType::TargetEnemyUnit:
		return TargetTile != nullptr && TargetTile->HasUnit() && TargetTile->GetUnit()->GetOwnerId() != SourceUnit->GetOwnerId();

	case EHeroAbilityTargetType::TargetAllyUnit:
		return TargetTile != nullptr && TargetTile->HasUnit() && TargetTile->GetUnit()->GetOwnerId() == SourceUnit->GetOwnerId();

	default:
		break;
	}

	return false;
}

bool UHeroAbilityBase::Activate(APlayerUnit* SourceUnit, ATile* TargetTile)
{
	return OnActivate(SourceUnit, TargetTile);
}

bool UHeroAbilityBase::OnActivate_Implementation(APlayerUnit* SourceUnit, ATile* TargetTile)
{
	return true;
}

void UHeroAbilityBase::ResetTurnUsage()
{
	bUsedThisTurn = false;
}

TSubclassOf<UHeroAbilityBase> UHeroAbilityBase::GetDefaultAbilityClassForPlayerClass(EWildPlayerClass InClass)
{
	switch (InClass)
	{
	case EWildPlayerClass::Warrior:
		return UWarriorHeroAbility::StaticClass();
	case EWildPlayerClass::Mage:
		return UMageHeroAbility::StaticClass();
	case EWildPlayerClass::Taoist:
		return UTaoistHeroAbility::StaticClass();
	case EWildPlayerClass::Common:
	default:
		return UWarriorHeroAbility::StaticClass();
	}
}

// -----------------------------------------------------------------------------
// UWarriorHeroAbility
// -----------------------------------------------------------------------------
UWarriorHeroAbility::UWarriorHeroAbility()
{
	AbilityName = FText::FromString(TEXT("방패 강타"));
	AbilityDescription = FText::FromString(TEXT("인접한 타일의 적 유닛에게 2의 물리 피해를 입힙니다."));
	ManaCost = 2;
	Range = 1;
	TargetType = EHeroAbilityTargetType::TargetEnemyUnit;
	DamageAmount = 2;
}

bool UWarriorHeroAbility::CanActivate(APlayerUnit* SourceUnit, ATile* TargetTile) const
{
	return Super::CanActivate(SourceUnit, TargetTile);
}

bool UWarriorHeroAbility::Activate(APlayerUnit* SourceUnit, ATile* TargetTile)
{
	if (!CanActivate(SourceUnit, TargetTile))
	{
		return false;
	}

	if (TargetTile && TargetTile->HasUnit())
	{
		AUnitBase* TargetUnit = TargetTile->GetUnit();
		TargetUnit->SetCardHealth(TargetUnit->GetCardHealth() - DamageAmount);
		UE_LOG(LogTemp, Log, TEXT("[WarriorHeroAbility] %s struck enemy %s for %d damage!"), *SourceUnit->GetName(), *TargetUnit->GetName(), DamageAmount);
		bUsedThisTurn = true;
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// UMageHeroAbility
// -----------------------------------------------------------------------------
UMageHeroAbility::UMageHeroAbility()
{
	AbilityName = FText::FromString(TEXT("화염구"));
	AbilityDescription = FText::FromString(TEXT("사거리 2 이내의 적 유닛에게 2의 마법 피해를 입힙니다."));
	ManaCost = 2;
	Range = 2;
	TargetType = EHeroAbilityTargetType::TargetEnemyUnit;
	SpellDamage = 2;
}

bool UMageHeroAbility::CanActivate(APlayerUnit* SourceUnit, ATile* TargetTile) const
{
	return Super::CanActivate(SourceUnit, TargetTile);
}

bool UMageHeroAbility::Activate(APlayerUnit* SourceUnit, ATile* TargetTile)
{
	if (!CanActivate(SourceUnit, TargetTile))
	{
		return false;
	}

	if (TargetTile && TargetTile->HasUnit())
	{
		AUnitBase* TargetUnit = TargetTile->GetUnit();
		TargetUnit->SetCardHealth(TargetUnit->GetCardHealth() - SpellDamage);
		UE_LOG(LogTemp, Log, TEXT("[MageHeroAbility] %s cast Fireblast on %s for %d damage!"), *SourceUnit->GetName(), *TargetUnit->GetName(), SpellDamage);
		bUsedThisTurn = true;
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// UTaoistHeroAbility
// -----------------------------------------------------------------------------
UTaoistHeroAbility::UTaoistHeroAbility()
{
	AbilityName = FText::FromString(TEXT("치유의 기운"));
	AbilityDescription = FText::FromString(TEXT("사거리 2 이내의 아군 유닛 또는 자신의 체력을 2 회복합니다."));
	ManaCost = 2;
	Range = 2;
	TargetType = EHeroAbilityTargetType::TargetAllyUnit;
	HealAmount = 2;
}

bool UTaoistHeroAbility::CanActivate(APlayerUnit* SourceUnit, ATile* TargetTile) const
{
	// 도사는 대상을 지정하지 않거나(Self) 자신 타일이면 플레이어 본체 회복 가능
	if (!TargetTile || TargetTile == SourceUnit->GetCurrentTile())
	{
		if (bUsedThisTurn || !SourceUnit) return false;
		AInGamePlayerState* PS = SourceUnit->GetOwningPlayerState();
		if (PS && PS->GetStatComponent() && PS->GetStatComponent()->Mana < ManaCost) return false;
		return true;
	}

	return Super::CanActivate(SourceUnit, TargetTile);
}

bool UTaoistHeroAbility::Activate(APlayerUnit* SourceUnit, ATile* TargetTile)
{
	if (!CanActivate(SourceUnit, TargetTile))
	{
		return false;
	}

	if (TargetTile && TargetTile->HasUnit())
	{
		AUnitBase* TargetUnit = TargetTile->GetUnit();
		TargetUnit->SetCardHealth(TargetUnit->GetCardHealth() + HealAmount);
		UE_LOG(LogTemp, Log, TEXT("[TaoistHeroAbility] %s healed %s for %d HP!"), *SourceUnit->GetName(), *TargetUnit->GetName(), HealAmount);
		bUsedThisTurn = true;
		return true;
	}
	else if (SourceUnit)
	{
		SourceUnit->SetCardHealth(SourceUnit->GetCardHealth() + HealAmount);
		UE_LOG(LogTemp, Log, TEXT("[TaoistHeroAbility] %s self-healed for %d HP!"), *SourceUnit->GetName(), HealAmount);
		bUsedThisTurn = true;
		return true;
	}

	return false;
}
