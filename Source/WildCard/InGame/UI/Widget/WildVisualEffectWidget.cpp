// Fill out your copyright notice in the Description page of Project Settings.

#include "InGame/UI/Widget/WildVisualEffectWidget.h"
#include "TimerManager.h"

void UWildVisualEffectWidget::PlayVisualEffect(const FWildVisualCueData& InCueData)
{
	CachedCueData = InCueData;

	// 안전 자동 소멸 타이머 등록 (블루프린트에서 FinishVisualEffect 미호출 시 화면 잔류 방지)
	if (SafetyAutoFinishDuration > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(AutoFinishTimerHandle);
			World->GetTimerManager().SetTimer(
				AutoFinishTimerHandle,
				this,
				&UWildVisualEffectWidget::FinishVisualEffect,
				SafetyAutoFinishDuration,
				false
			);
		}
	}

	BP_OnPlayVisualEffect(InCueData);
}

void UWildVisualEffectWidget::FinishVisualEffect()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoFinishTimerHandle);
	}

	BP_OnVisualEffectFinished();
	OnVisualEffectFinished.Broadcast();
	RemoveFromParent();
}

void UWildVisualEffectMoveWidget::PlayVisualEffect(const FWildVisualCueData& InCueData)
{
	Super::PlayVisualEffect(InCueData);

	BP_OnMoveVisualEffect(
		InCueData.SourceActor.Get(),
		InCueData.SourceQ,
		InCueData.SourceR,
		InCueData.TargetQ,
		InCueData.TargetR
	);
}

void UWildVisualEffectAttackWidget::PlayVisualEffect(const FWildVisualCueData& InCueData)
{
	Super::PlayVisualEffect(InCueData);

	BP_OnAttackVisualEffect(
		InCueData.SourceActor.Get(),
		InCueData.TargetActor.Get(),
		InCueData.NumericValue
	);
}

