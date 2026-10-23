// Fill out your copyright notice in the Description page of Project Settings.

#include "InGame/Unit/MageSearchLibraryTacticalSkill.h"
#include "InGame/Unit/PlayerUnit.h"
#include "InGame/Map/Tile.h"
#include "InGame/Game/InGamePlayerState.h"
#include "InGame/Game/InGameState.h"
#include "InGame/Game/InGameController.h"
#include "InGame/Game/PlayerCardComponent.h"
#include "InGame/Battle/PlayerStatComponent.h"
#include "InGame/Effect/SearchLibraryEffect.h"
#include "InGame/Effect/TileSelectComponent.h"
#include "WildCard.h"

UMageSearchLibraryTacticalSkill::UMageSearchLibraryTacticalSkill()
{
	SkillID = FName(TEXT("Mage_SearchLibrary"));
	SkillName = FText::FromString(TEXT("서고 탐색"));
	SkillDescription = FText::FromString(TEXT("패에서 카드를 1장 덱으로 되돌리고 드로우한다."));
	RequiredClass = EWildPlayerClass::Mage;
	ManaCost = 1;
	Range = 0;
	TargetType = ETacticalSkillTargetType::Self;
	SkillEffectClass = USearchLibraryEffect::StaticClass();
	VisualCueTag = FGameplayTag::RequestGameplayTag(FName("Visual.TacticalSkill.Mage.SearchLibrary"), false);
}

void UMageSearchLibraryTacticalSkill::InitiateSkill(AInGameController* PC, APlayerUnit* SourceUnit)
{
	if (!PC) return;

	FWildActionData ActionData;
	if (AInGamePlayerState* PS = PC->GetPlayerState<AInGamePlayerState>())
	{
		ActionData.PlayerID = PS->GetPlayerId();
	}
	ActionData.ActionCategory = EWildActionCategory::Sub;
	ActionData.ActionType = EWildActionType::CastTacticalSkill;
	ActionData.CardInstanceID = -1;
	ActionData.SourceTile = SourceUnit ? SourceUnit->GetCurrentTile() : nullptr;
	ActionData.TargetTile = nullptr;

	UE_LOG(LogWildChain, Log, TEXT("[TacticalSkill] 마법사 전술 스킬 '서고 탐색' 체인 등록 (PlayerID: %d)"), ActionData.PlayerID);
	PC->PlayChainCard(ActionData);
	PC->CancelTileAction();
}

bool UMageSearchLibraryTacticalSkill::CanInitiate(APlayerUnit* SourceUnit) const
{
	if (!Super::CanInitiate(SourceUnit))
	{
		return false;
	}

	if (AInGamePlayerState* PS = SourceUnit->GetOwningPlayerState())
	{
		if (UPlayerCardComponent* CardComp = PS->GetCardComponent())
		{
			int32 HandNum = (CardComp->GetOwnerRole() == ROLE_Authority) ? CardComp->ServerHand.Num() : CardComp->HandCount;
			return HandNum > 0;
		}
	}

	return false;
}

bool UMageSearchLibraryTacticalSkill::CanActivate(APlayerUnit* SourceUnit, ATile* TargetTile, bool bCheckMana) const
{
	if (!SourceUnit) return false;

	AInGamePlayerState* PS = SourceUnit->GetOwningPlayerState();
	if (!PS) return false;

	if (bCheckMana && PS->GetStatComponent() && PS->GetStatComponent()->Mana < ManaCost)
	{
		return false;
	}

	if (UPlayerCardComponent* CardComp = PS->GetCardComponent())
	{
		int32 HandNum = (CardComp->GetOwnerRole() == ROLE_Authority) ? CardComp->ServerHand.Num() : CardComp->HandCount;
		return HandNum > 0;
	}

	return false;
}

bool UMageSearchLibraryTacticalSkill::CanActivateAction(APlayerUnit* SourceUnit, const FWildActionData& ActionData, bool bCheckMana) const
{
	if (!CanActivate(SourceUnit, ActionData.TargetTile, bCheckMana))
	{
		return false;
	}

	if (SourceUnit && ActionData.CardInstanceID != -1)
	{
		if (AInGamePlayerState* PS = SourceUnit->GetOwningPlayerState())
		{
			if (UPlayerCardComponent* CardComp = PS->GetCardComponent())
			{
				if (CardComp->GetOwnerRole() == ROLE_Authority)
				{
					return CardComp->IsCardInHand(ActionData.CardInstanceID);
				}
			}
		}
	}

	return true;
}

bool UMageSearchLibraryTacticalSkill::Execute(APlayerUnit* SourceUnit, ATile* TargetTile)
{
	return true;
}

bool UMageSearchLibraryTacticalSkill::ExecuteAction(APlayerUnit* SourceUnit, const FWildActionData& ActionData)
{
	if (!CanActivateAction(SourceUnit, ActionData, false))
	{
		return false;
	}

	// 서고 탐색 전용 이펙트 실행 (USearchLibraryEffect 내부에서 드로우 및 Visual Cue 브로드캐스트 수행)
	if (SkillEffectClass)
	{
		if (UEffectBase* Effect = NewObject<UEffectBase>(SourceUnit, SkillEffectClass))
		{
			FEffectData EffectData;
			EffectData.VisualCueTag = VisualCueTag;
			Effect->ExecuteEffect(ActionData, EffectData);
			UE_LOG(LogTemp, Log, TEXT("[TacticalSkill] 마법사 전술 스킬 서고 탐색 발동! (CardInstanceID: %d)"), ActionData.CardInstanceID);
		}
	}
	else
	{
		// EffectClass가 없을 경우의 Fallback Visual Cue
		if (SourceUnit && SourceUnit->GetWorld())
		{
			if (AInGameState* GS = SourceUnit->GetWorld()->GetGameState<AInGameState>())
			{
				FWildVisualCueData CueData;
				CueData.CueTag = VisualCueTag.IsValid() ? VisualCueTag : FGameplayTag::RequestGameplayTag(FName("Visual.TacticalSkill.Mage.SearchLibrary"), false);
				CueData.InstigatorPlayerID = SourceUnit->GetOwnerId();
				CueData.SourceActor = SourceUnit;
				CueData.SourceID = SkillID;
				CueData.CueMessage = SkillName;
				CueData.NumericValue = 1;

				GS->Multicast_BroadcastVisualCue(CueData);
			}
		}
	}

	return true;
}


bool UMageSearchLibraryTacticalSkill::RequiresPlayerPromptOnResolve(const FWildActionData& ActionData) const
{
	// 카드가 아직 선택되지 않은 경우(-1) 프롬프트 필요
	return ActionData.CardInstanceID == -1;
}

void UMageSearchLibraryTacticalSkill::PromptPlayerOnResolve(AInGameController* PC, const FWildActionData& ActionData)
{
	if (PC)
	{
		PC->Client_PromptSearchLibrarySelection();
	}
}
