// Fill out your copyright notice in the Description page of Project Settings.

#include "InGame/Effect/SearchLibraryEffect.h"
#include "InGameState.h"
#include "InGamePlayerState.h"
#include "PlayerCardComponent.h"

void USearchLibraryEffect::ExecuteEffect_Implementation(const FWildActionData& ActionData, const FEffectData& InEffectData)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AInGameState* GS = World->GetGameState<AInGameState>();
	if (!GS)
	{
		return;
	}

	AInGamePlayerState* TargetPS = nullptr;
	for (APlayerState* PS : GS->PlayerArray)
	{
		if (AInGamePlayerState* WildPS = Cast<AInGamePlayerState>(PS))
		{
			if (WildPS->GetPlayerId() == ActionData.PlayerID)
			{
				TargetPS = WildPS;
				break;
			}
		}
	}

	if (!TargetPS)
	{
		return;
	}

	if (UPlayerCardComponent* CardComp = TargetPS->GetCardComponent())
	{
		CardComp->ReturnCardToDeckAndDraw(ActionData.CardInstanceID);

		FWildVisualCueData CueData;
		CueData.CueTag = InEffectData.VisualCueTag.IsValid() 
			? InEffectData.VisualCueTag 
			: FGameplayTag::RequestGameplayTag(FName("Visual.TacticalSkill.Mage.SearchLibrary"), false);
		CueData.InstigatorPlayerID = ActionData.PlayerID;
		CueData.NumericValue = 1;
		CueData.CueMessage = FText::FromString(TEXT("서고 탐색"));
		BroadcastVisualCue(CueData);
	}
}
