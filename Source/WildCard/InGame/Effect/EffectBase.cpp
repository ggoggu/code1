// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Effect/EffectBase.h"
#include "InGame/Game/InGameState.h"

UWorld* UEffectBase::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	if (UObject* Outer = GetOuter())
	{
		return Outer->GetWorld();
	}

	return nullptr;
}

void UEffectBase::BroadcastVisualCue(const FWildVisualCueData& CueData)
{
	if (UWorld* World = GetWorld())
	{
		if (AInGameState* GS = World->GetGameState<AInGameState>())
		{
			GS->Multicast_BroadcastVisualCue(CueData);
		}
	}
}
