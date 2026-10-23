// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Effect/MoveCardZoneEffect.h"
#include "InGameState.h"
#include "InGamePlayerState.h"
#include "Tile.h"
#include "UnitBase.h"

void UMoveCardZoneEffect::ExecuteEffect_Implementation(const FWildActionData& ActionData, const FEffectData& InEffectData)
{
	AInGameState* GS = GetWorld()->GetGameState<AInGameState>();
	AInGamePlayerState* PS = nullptr;

	for (APlayerState* PlayerState : (GS->PlayerArray))
	{
		AInGamePlayerState* WildPS =
			Cast<AInGamePlayerState>(PlayerState);

		if (IsValid(WildPS) && WildPS->GetPlayerId() == ActionData.PlayerID)
		{
			PS = WildPS;
			break;
		}
	}
	//이 카드 베이스가 있던 곳을 알아야하는데, 있던곳을 알아내려고 switch문을 써야하나
	// 어디있었는지를 알아야 거기서 제거하고 옮기는걸 하는데
	UCardBase* Card = PS->FindCardInstanceByID(ActionData.TargetCardInstanceID);
	if(!IsValid(Card))
		return;

	switch (InEffectData.MoveFrom)
	{
		case ECardZone::EXILE:
			PS->CardComponent->ServerExile.Remove(Card);
			break;
		case ECardZone::BATTLEFIELD:
			PS->CardComponent->ServerBattlefield.Remove(Card);
			break;
		case ECardZone::HAND:
			PS->CardComponent->ServerHand.Remove(Card);
			break;
		case ECardZone::DECK:
			PS->CardComponent->ServerDeck.Remove(Card);
			break;
		case ECardZone::OBLITERATE:
			PS->CardComponent->ServerObliterate.Remove(Card);
			break;
	}

	switch (InEffectData.MoveTo)
	{
		case ECardZone::EXILE:
			PS->CardComponent->ServerExile.Add(Card);
			break;
		case ECardZone::BATTLEFIELD:
			PS->CardComponent->ServerBattlefield.Add(Card);
			break;
		case ECardZone::HAND:
			PS->CardComponent->ServerHand.Add(Card);
			break;
		case ECardZone::DECK:
			PS->CardComponent->ServerDeck.Add(Card);
			break;
		case ECardZone::OBLITERATE:
			PS->CardComponent->ServerObliterate.Add(Card);
			break;


	}
	//ActionData.TargetTile->GetUnit()->SetCardHealth(0);
	//PS->CardComponent->ServerExile.Add(ActionData.CardInstanceID);
	PS->CardComponent->SyncHandDataToDisplay();

}
