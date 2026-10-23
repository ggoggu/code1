// Fill out your copyright notice in the Description page of Project Settings.

#include "InGame/Unit/TacticalSkillBase.h"
#include "InGame/Unit/PlayerUnit.h"
#include "InGame/Map/Tile.h"
#include "InGame/Game/InGamePlayerState.h"
#include "InGame/Game/InGameState.h"
#include "InGame/Game/InGameController.h"
#include "InGame/Game/PlayerCardComponent.h"
#include "InGame/Battle/PlayerStatComponent.h"
#include "InGame/Effect/TileSelectComponent.h"
#include "InGame/Effect/EffectBase.h"
#include "InGame/Unit/MageSearchLibraryTacticalSkill.h"

UTacticalSkillBase::UTacticalSkillBase()
{
	SkillID = FName(TEXT("DefaultTacticalSkill"));
	SkillName = FText::FromString(TEXT("기본 전술 스킬"));
	SkillDescription = FText::FromString(TEXT("체인 페이즈에 사용 가능한 서브 액션 전술 스킬입니다."));
	RequiredClass = EWildPlayerClass::Common;
	ManaCost = 2;
	Range = 1;
	TargetType = ETacticalSkillTargetType::Self;
	SkillEffectClass = nullptr;
}

void UTacticalSkillBase::InitiateSkill(AInGameController* PC, APlayerUnit* SourceUnit)
{
	if (PC && PC->GetTileSelectComponent())
	{
		PC->GetTileSelectComponent()->SelectTacticalSkillAction();
	}
}

bool UTacticalSkillBase::CanInitiate(APlayerUnit* SourceUnit) const
{
	if (!SourceUnit) return false;
	AInGamePlayerState* PS = SourceUnit->GetOwningPlayerState();
	if (PS && PS->GetStatComponent())
	{
		if (PS->GetStatComponent()->Mana < ManaCost)
		{
			return false;
		}
	}
	return true;
}

bool UTacticalSkillBase::CanActivate(APlayerUnit* SourceUnit, ATile* TargetTile, bool bCheckMana) const
{
	if (!SourceUnit)
	{
		return false;
	}

	AInGamePlayerState* PS = SourceUnit->GetOwningPlayerState();
	if (bCheckMana && PS && PS->GetStatComponent())
	{
		if (PS->GetStatComponent()->Mana < ManaCost)
		{
			return false;
		}
	}

	// 사거리 체크
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
	case ETacticalSkillTargetType::Self:
		return true;

	case ETacticalSkillTargetType::TargetTile:
		return TargetTile != nullptr;

	case ETacticalSkillTargetType::TargetEnemyUnit:
		return TargetTile != nullptr && TargetTile->HasUnit() && TargetTile->GetUnit()->GetOwnerId() != SourceUnit->GetOwnerId();

	case ETacticalSkillTargetType::TargetAllyUnit:
		return TargetTile != nullptr && TargetTile->HasUnit() && TargetTile->GetUnit()->GetOwnerId() == SourceUnit->GetOwnerId();

	default:
		break;
	}

	return false;
}

bool UTacticalSkillBase::CanActivateAction(APlayerUnit* SourceUnit, const FWildActionData& ActionData, bool bCheckMana) const
{
	return CanActivate(SourceUnit, ActionData.TargetTile, bCheckMana);
}

void UTacticalSkillBase::GetValidTargetTiles(APlayerUnit* SourceUnit, TArray<ATile*>& OutTiles) const
{
	OutTiles.Empty();
	if (!SourceUnit || !SourceUnit->GetCurrentTile())
	{
		return;
	}

	UWorld* World = SourceUnit->GetWorld();
	if (!World)
	{
		return;
	}

	AInGameState* GS = World->GetGameState<AInGameState>();
	if (!GS)
	{
		return;
	}

	TArray<TWeakObjectPtr<ATile>> AllTilePtrs = GS->GetTilesArray();
	for (const TWeakObjectPtr<ATile>& TilePtr : AllTilePtrs)
	{
		if (ATile* Tile = TilePtr.Get())
		{
			if (CanActivate(SourceUnit, Tile))
			{
				OutTiles.Add(Tile);
			}
		}
	}
}

bool UTacticalSkillBase::Execute(APlayerUnit* SourceUnit, ATile* TargetTile)
{
	return OnExecute(SourceUnit, TargetTile);
}

bool UTacticalSkillBase::ExecuteAction(APlayerUnit* SourceUnit, const FWildActionData& ActionData)
{
	bool bSuccess = Execute(SourceUnit, ActionData.TargetTile);
	if (bSuccess)
	{
		if (SkillEffectClass)
		{
			if (UEffectBase* Effect = NewObject<UEffectBase>(SourceUnit, SkillEffectClass))
			{
				FEffectData EffectData;
				EffectData.VisualCueTag = VisualCueTag;
				Effect->ExecuteEffect(ActionData, EffectData);
			}
		}

		// 시각/UI 연출 브로드캐스트 (서버 권한에서 모든 클라이언트로 전파)
		if (SourceUnit && SourceUnit->GetWorld())
		{
			if (AInGameState* GS = SourceUnit->GetWorld()->GetGameState<AInGameState>())
			{
				FWildVisualCueData CueData;
				CueData.CueTag = VisualCueTag.IsValid() ? VisualCueTag : FGameplayTag::RequestGameplayTag(FName("Visual.TacticalSkill"), false);
				CueData.InstigatorPlayerID = SourceUnit->GetOwnerId();
				CueData.SourceActor = SourceUnit;
				CueData.TargetActor = ActionData.TargetTile ? ActionData.TargetTile->GetUnit() : nullptr;
				CueData.SourceID = SkillID;
				CueData.CueMessage = SkillName;
				CueData.NumericValue = GetSkillNumericValue();
				if (ActionData.TargetTile)
				{
					CueData.TargetQ = ActionData.TargetTile->Axial.Q;
					CueData.TargetR = ActionData.TargetTile->Axial.R;
				}

				GS->Multicast_BroadcastVisualCue(CueData);
			}
		}
	}
	return bSuccess;
}

bool UTacticalSkillBase::OnExecute_Implementation(APlayerUnit* SourceUnit, ATile* TargetTile)
{
	return true;
}

TSubclassOf<UTacticalSkillBase> UTacticalSkillBase::GetDefaultSkillClassForPlayerClass(EWildPlayerClass InClass)
{
	switch (InClass)
	{
	case EWildPlayerClass::Warrior:
		return UWarriorTacticalSkill::StaticClass();
	case EWildPlayerClass::Mage:
		return UMageSearchLibraryTacticalSkill::StaticClass();
	case EWildPlayerClass::Taoist:
		return UTaoistTacticalSkill::StaticClass();
	case EWildPlayerClass::Common:
	default:
		return UWarriorTacticalSkill::StaticClass();
	}
}

// -----------------------------------------------------------------------------
// UWarriorTacticalSkill
// -----------------------------------------------------------------------------
UWarriorTacticalSkill::UWarriorTacticalSkill()
{
	SkillID = FName(TEXT("Warrior_Smash"));
	SkillName = FText::FromString(TEXT("전술 스킬: 강타"));
	SkillDescription = FText::FromString(TEXT("인접한 적 유닛에게 강력한 물리 피해를 입힙니다."));
	RequiredClass = EWildPlayerClass::Warrior;
	ManaCost = 2;
	Range = 1;
	TargetType = ETacticalSkillTargetType::TargetEnemyUnit;
	DamageAmount = 2;
	VisualCueTag = FGameplayTag::RequestGameplayTag(FName("Visual.TacticalSkill.Warrior.Smash"), false);
}

bool UWarriorTacticalSkill::CanActivate(APlayerUnit* SourceUnit, ATile* TargetTile, bool bCheckMana) const
{
	return Super::CanActivate(SourceUnit, TargetTile, bCheckMana);
}

bool UWarriorTacticalSkill::Execute(APlayerUnit* SourceUnit, ATile* TargetTile)
{
	if (!CanActivate(SourceUnit, TargetTile))
	{
		return false;
	}

	AUnitBase* TargetUnit = TargetTile ? TargetTile->GetUnit() : nullptr;
	if (!TargetUnit)
	{
		return false;
	}

	int32 NewHealth = TargetUnit->GetCardHealth() - DamageAmount;
	TargetUnit->SetCardHealth(NewHealth);

	UE_LOG(LogTemp, Log, TEXT("[TacticalSkill] 전사 전술 스킬 강타 발동! %s에게 %d 피해. 남은 체력: %d"),
		*TargetUnit->GetName(), DamageAmount, NewHealth);

	return Super::Execute(SourceUnit, TargetTile);
}

// -----------------------------------------------------------------------------
// UMageTacticalSkill
// -----------------------------------------------------------------------------
UMageTacticalSkill::UMageTacticalSkill()
{
	SkillID = FName(TEXT("Mage_ArcaneBolt"));
	SkillName = FText::FromString(TEXT("전술 스킬: 비전 탄막"));
	SkillDescription = FText::FromString(TEXT("사거리 2 내의 적 유닛에게 마법 피해를 입힙니다."));
	RequiredClass = EWildPlayerClass::Mage;
	ManaCost = 2;
	Range = 2;
	TargetType = ETacticalSkillTargetType::TargetEnemyUnit;
	SpellDamage = 2;
	VisualCueTag = FGameplayTag::RequestGameplayTag(FName("Visual.TacticalSkill.Mage.ArcaneBolt"), false);
}

bool UMageTacticalSkill::CanActivate(APlayerUnit* SourceUnit, ATile* TargetTile, bool bCheckMana) const
{
	return Super::CanActivate(SourceUnit, TargetTile, bCheckMana);
}

bool UMageTacticalSkill::Execute(APlayerUnit* SourceUnit, ATile* TargetTile)
{
	if (!CanActivate(SourceUnit, TargetTile))
	{
		return false;
	}

	AUnitBase* TargetUnit = TargetTile ? TargetTile->GetUnit() : nullptr;
	if (!TargetUnit)
	{
		return false;
	}

	int32 NewHealth = TargetUnit->GetCardHealth() - SpellDamage;
	TargetUnit->SetCardHealth(NewHealth);

	UE_LOG(LogTemp, Log, TEXT("[TacticalSkill] 마법사 전술 스킬 비전 탄막 발동! %s에게 %d 피해. 남은 체력: %d"),
		*TargetUnit->GetName(), SpellDamage, NewHealth);

	return Super::Execute(SourceUnit, TargetTile);
}


// -----------------------------------------------------------------------------
// UTaoistTacticalSkill
// -----------------------------------------------------------------------------
UTaoistTacticalSkill::UTaoistTacticalSkill()
{
	SkillID = FName(TEXT("Taoist_HealingAura"));
	SkillName = FText::FromString(TEXT("전술 스킬: 회복의 진"));
	SkillDescription = FText::FromString(TEXT("사거리 1 내의 자신 또는 아군 유닛의 체력을 회복합니다."));
	RequiredClass = EWildPlayerClass::Taoist;
	ManaCost = 2;
	Range = 1;
	TargetType = ETacticalSkillTargetType::TargetAllyUnit;
	HealAmount = 2;
	VisualCueTag = FGameplayTag::RequestGameplayTag(FName("Visual.TacticalSkill.Taoist.HealingAura"), false);
}

bool UTaoistTacticalSkill::CanActivate(APlayerUnit* SourceUnit, ATile* TargetTile, bool bCheckMana) const
{
	return Super::CanActivate(SourceUnit, TargetTile, bCheckMana);
}

bool UTaoistTacticalSkill::Execute(APlayerUnit* SourceUnit, ATile* TargetTile)
{
	if (!CanActivate(SourceUnit, TargetTile))
	{
		return false;
	}

	AUnitBase* TargetUnit = TargetTile ? TargetTile->GetUnit() : nullptr;
	if (!TargetUnit)
	{
		return false;
	}

	int32 NewHealth = TargetUnit->GetCardHealth() + HealAmount;
	TargetUnit->SetCardHealth(NewHealth);

	UE_LOG(LogTemp, Log, TEXT("[TacticalSkill] 도사 전술 스킬 회복의 진 발동! %s의 체력 %d 회복. 현재 체력: %d"),
		*TargetUnit->GetName(), HealAmount, NewHealth);

	return Super::Execute(SourceUnit, TargetTile);
}
