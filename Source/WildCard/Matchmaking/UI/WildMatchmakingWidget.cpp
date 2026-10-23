// Fill out your copyright notice in the Description page of Project Settings.

#include "WildMatchmakingWidget.h"
#include "WildMatchmakingSubsystem.h"

void UWildMatchmakingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWildMatchmakingSubsystem* MatchSub = GI->GetSubsystem<UWildMatchmakingSubsystem>())
		{
			MatchSub->OnStateChanged.AddDynamic(this, &UWildMatchmakingWidget::HandleStateChanged);
			MatchSub->OnTimerTick.AddDynamic(this, &UWildMatchmakingWidget::HandleTimerTick);
			MatchSub->OnMatchFound.AddDynamic(this, &UWildMatchmakingWidget::HandleMatchFound);
		}
	}
}

void UWildMatchmakingWidget::NativeDestruct()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWildMatchmakingSubsystem* MatchSub = GI->GetSubsystem<UWildMatchmakingSubsystem>())
		{
			MatchSub->OnStateChanged.RemoveDynamic(this, &UWildMatchmakingWidget::HandleStateChanged);
			MatchSub->OnTimerTick.RemoveDynamic(this, &UWildMatchmakingWidget::HandleTimerTick);
			MatchSub->OnMatchFound.RemoveDynamic(this, &UWildMatchmakingWidget::HandleMatchFound);
		}
	}

	Super::NativeDestruct();
}

void UWildMatchmakingWidget::RequestCancelMatchmaking()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWildMatchmakingSubsystem* MatchSub = GI->GetSubsystem<UWildMatchmakingSubsystem>())
		{
			MatchSub->CancelMatchmaking();
		}
	}
}

void UWildMatchmakingWidget::HandleStateChanged(EWildMatchmakingState NewState)
{
	OnMatchmakingStateChanged(NewState);
}

void UWildMatchmakingWidget::HandleTimerTick(int32 ElapsedSeconds, int32 RemainingSecondsUntilBot)
{
	OnMatchmakingTimerTick(ElapsedSeconds, RemainingSecondsUntilBot);
}

void UWildMatchmakingWidget::HandleMatchFound(const FWildMatchResult& MatchResult)
{
	OnMatchFound(MatchResult);
}
