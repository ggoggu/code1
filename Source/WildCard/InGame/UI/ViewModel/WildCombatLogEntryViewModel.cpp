// Fill out your copyright notice in the Description page of Project Settings.

#include "InGame/UI/ViewModel/WildCombatLogEntryViewModel.h"
#include "InGame/Battle/WildCombatLogComponent.h"

void UWildCombatLogEntryViewModel::InitializeFromEntry(const FWildCombatLogEntry& InEntry, int32 LocalPlayerID, int32 FirstPlayerID)
{
	LogIndex = InEntry.LogIndex;
	Category = InEntry.LogType;
	bIsAllyAction = (InEntry.InstigatorPlayerID == LocalPlayerID);

	FString OutTitle;
	TArray<FString> OutDetails;
	UWildCombatLogComponent::FormatLogEntry(InEntry, LocalPlayerID, FirstPlayerID, OutTitle, OutDetails);

	SetTitleText(FText::FromString(OutTitle));

	FString CombinedDetails = FString::Join(OutDetails, TEXT("\n"));
	SetFullDetailText(FText::FromString(CombinedDetails));

	// 카테고리별 헤더 색상 설정
	switch (Category)
	{
	case EWildCombatLogType::TurnPriority:
	case EWildCombatLogType::Gameset:
	case EWildCombatLogType::ArtifactReveal:
		SetHeaderColor(FLinearColor(1.0f, 0.85f, 0.35f, 1.0f)); // Gold
		break;
	case EWildCombatLogType::PhasePublic:
	case EWildCombatLogType::ChainCast:
		SetHeaderColor(bIsAllyAction ? FLinearColor(0.45f, 0.82f, 1.0f, 1.0f) : FLinearColor(1.0f, 0.45f, 0.45f, 1.0f));
		break;
	case EWildCombatLogType::Combat:
		SetHeaderColor(FLinearColor(1.0f, 0.35f, 0.2f, 1.0f)); // Orange-Red
		break;
	case EWildCombatLogType::ActionFailed:
		SetHeaderColor(FLinearColor(0.8f, 0.3f, 0.3f, 1.0f)); // Dark Red
		break;
	default:
		SetHeaderColor(FLinearColor(0.9f, 0.9f, 0.9f, 1.0f)); // Off-White
		break;
	}
}
