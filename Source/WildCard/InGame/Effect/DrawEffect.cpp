// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Effect/DrawEffect.h"
#include "InGameState.h"
#include "InGamePlayerState.h"
#include "InGameController.h"

void UDrawEffect::ExecuteEffect_Implementation(const FWildActionData& ActionData, const FEffectData& InEffectData)
{

	AInGameState* GS = GetWorld()->GetGameState<AInGameState>();
	AInGamePlayerState* WildPS = nullptr;
	AInGameController* WildPC = nullptr;

	for (APlayerState* PS : GS->PlayerArray)
	{
		WildPS = Cast<AInGamePlayerState>(PS);

		if (!IsValid(WildPS))
			return;


		if (WildPS->GetPlayerId() == ActionData.PlayerID)
			break;
	}

	//클라이언트에게 드로우 하게 하기
	WildPC = Cast<AInGameController>(WildPS->GetPlayerController());

	for (int i = 0; i < (Value) ; i++ )
	{
		WildPS->DrawCard(TEXT("Effect_Draw"), 1);
	}
}
