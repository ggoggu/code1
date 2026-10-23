// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerCardComponent.h"
#include "Net/UnrealNetwork.h"
#include "CardBase.h"
#include "InGameController.h"
#include "InGameMode.h"
#include "InGameState.h"
#include "WildCardGameInstance.h"
#include "GameFramework/PlayerState.h"

UPlayerCardComponent::UPlayerCardComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	HandCount = 0;
	DeckCount = 0;
}

void UPlayerCardComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwnerRole() == ROLE_Authority)
	{
		DeckRandomStream.Initialize(FMath::Rand());
	}
}

void UPlayerCardComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPlayerCardComponent, HandCount);
	DOREPLIFETIME(UPlayerCardComponent, DeckCount);
	DOREPLIFETIME_CONDITION(UPlayerCardComponent, CurrentHandDisplay, COND_OwnerOnly);
}

void UPlayerCardComponent::ExecuteMulliganSwap()
{
	if (GetOwnerRole() != ROLE_Authority) return;

	APlayerState* OwnerPS = Cast<APlayerState>(GetOwner());
	int32 PlayerID = OwnerPS ? OwnerPS->GetPlayerId() : -1;

	// 교체할 카드가 없다면 바로 종료
	if (CachedMulliganCards.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] Player %d - 멀리건 교체할 카드가 없습니다 (0장)."), PlayerID);
		return;
	}

	TArray<UCardBase*> CardsToReturn;
	FString DroppedCardsLog = TEXT(""); 
	FString NewCardsLog = TEXT("");     

	// 1. 내 손패를 역순으로 순회하며 교체 대상 카드를 덱으로 반환 준비
	for (int32 i = ServerHand.Num() - 1; i >= 0; --i)
	{
		UCardBase* Card = ServerHand[i];
		if (Card && CachedMulliganCards.Contains(Card->InstanceID))
		{
			DroppedCardsLog += FString::Printf(TEXT("[%s] "), *Card->CardID.ToString());
			CardsToReturn.Add(Card);
			ServerHand.RemoveAt(i);
		}
	}

	int32 SwapCount = CardsToReturn.Num();

	// 2. 버린 장수만큼 덱에서 '새로운' 카드를 뽑아 손패에 넣기
	for (int32 i = 0; i < SwapCount; ++i)
	{
		if (ServerDeck.Num() > 0)
		{
			UCardBase* NewCard = ServerDeck.Pop();
			NewCardsLog += FString::Printf(TEXT("[%s] "), *NewCard->CardID.ToString());
			ServerHand.Add(NewCard);
		}
	}

	// 3. 빼두었던 예전 카드들을 덱에 다시 집어넣고 섞기
	ServerDeck.Append(CardsToReturn);
	ShuffleDeck();

	// 4. 남은 덱 장수 갱신 및 캐싱 데이터 청소
	DeckCount = ServerDeck.Num();
	CachedMulliganCards.Empty();

	UE_LOG(LogTemp, Warning, TEXT("================[ 멀리건 교체 결과 ]================"));
	UE_LOG(LogTemp, Warning, TEXT("Player ID: %d"), PlayerID);
	UE_LOG(LogTemp, Warning, TEXT("버린 카드들: %s"), *DroppedCardsLog);
	UE_LOG(LogTemp, Warning, TEXT("새로 뽑은 카드들: %s"), *NewCardsLog);
	UE_LOG(LogTemp, Warning, TEXT("===================================================="));
}

TArray<int32> UPlayerCardComponent::GetCardInstanceIDsInArray(ECardZone InZone) const
{
	TArray<int32> CardInstanceIDs;

	switch (InZone)
	{
		case ECardZone::DECK:
			for (UCardBase* Card : ServerDeck)
			{
				if (Card)
				{
					CardInstanceIDs.Add(Card->InstanceID);
				}
			}
			return CardInstanceIDs;

		case ECardZone::HAND:
			for (UCardBase* Card : ServerHand)
			{
				if (Card)
				{
					CardInstanceIDs.Add(Card->InstanceID);
				}
			}
			return CardInstanceIDs;

		case ECardZone::BATTLEFIELD:
			for (UCardBase* Card : ServerBattlefield)
			{
				if (Card)
				{
					CardInstanceIDs.Add(Card->InstanceID);
				}
			}
			return CardInstanceIDs;

		case ECardZone::EXILE:
			for (UCardBase* Card : ServerExile)
			{
				if (Card)
				{
					CardInstanceIDs.Add(Card->InstanceID);
				}
			}
			return CardInstanceIDs;

		case ECardZone::OBLITERATE:
			for (UCardBase* Card : ServerObliterate)
			{
				if (Card)
				{
					CardInstanceIDs.Add(Card->InstanceID);
				}
			}
			return CardInstanceIDs;
	}

	return CardInstanceIDs;
}

UCardBase* UPlayerCardComponent::DrawCard(FName CardIDToDraw, int32 BaseMana)
{
	if (GetOwnerRole() != ROLE_Authority) return nullptr;

	if (ServerDeck.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] 덱이 비어있어 카드를 뽑을 수 없습니다!"));
		return nullptr;
	}

	UCardBase* DrawnCard = ServerDeck.Pop();
	ServerHand.Add(DrawnCard);

	DeckCount = ServerDeck.Num();
	OnDeckCountChangedDelegate.Broadcast(DeckCount);

	SyncHandDataToDisplay();

	UE_LOG(LogTemp, Log, TEXT("[Server] %s 카드를 드로우했습니다. 남은 덱: %d장"), *DrawnCard->CardID.ToString(), DeckCount);
	return DrawnCard;
}

void UPlayerCardComponent::Debug_GiveDummyStartingHand()
{
	if (GetOwnerRole() != ROLE_Authority) return;

	if (ServerDeck.Num() == 0)
	{
		CreateTestDeck();
	}

	ServerHand.Empty();

	const int32 StartingHandCount = 5;
	for (int32 i = 0; i < StartingHandCount; i++)
	{
		if (ServerDeck.Num() > 0)
		{
			UCardBase* DrawnCard = ServerDeck.Pop();
			ServerHand.Add(DrawnCard);
		}
	}

	DeckCount = ServerDeck.Num();
	SyncHandDataToDisplay();
	OnDeckCountChangedDelegate.Broadcast(DeckCount);

	UE_LOG(LogTemp, Log, TEXT("[Server] 시작 패 5장을 덱에서 드로우했습니다. 남은 덱: %d장"), DeckCount);
}

void UPlayerCardComponent::CreateTestDeck()
{
	if (GetOwnerRole() != ROLE_Authority) return;

	ServerDeck.Empty();

	UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(GetWorld()->GetGameInstance());
	if (!GI || !GI->CardDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[Server] 카드 테이블을 찾을 수 없습니다."));
		return;
	}

	TArray<FName> RowNames = GI->CardDataTable->GetRowNames();
	if (RowNames.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[Server] 카드 테이블이 비어있습니다."));
		return;
	}

	for (int32 i = 0; i < 30; i++)
	{
		const int32 RandIndex = DeckRandomStream.RandRange(0, RowNames.Num() - 1);
		const FName RowName = RowNames[RandIndex];

		UCardBase* NewCard = GenerateCardInstance(RowName);

		if (NewCard)
		{
			ServerDeck.Add(NewCard);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Server] %s 카드 생성에 실패하여 덱에 추가하지 못했습니다."), *RowName.ToString());
		}
	}

	ShuffleDeck();
	
	DeckCount = ServerDeck.Num();
	OnDeckCountChangedDelegate.Broadcast(DeckCount);

	UE_LOG(LogTemp, Log, TEXT("[Server] 랜덤 덱 %d장 생성 및 셔플 완료"), DeckCount);
}

void UPlayerCardComponent::InitDeckFromData(const FWildDeckData& InDeckData)
{
	if (GetOwnerRole() != ROLE_Authority) return;

	ServerDeck.Empty();

	UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(GetWorld()->GetGameInstance());
	if (!GI || !GI->CardDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[Server] 카드 테이블을 찾을 수 없습니다."));
		return;
	}

	for (const FName& CardRowName : InDeckData.CardIDs)
	{
		UCardBase* NewCard = GenerateCardInstance(CardRowName);
		if (NewCard)
		{
			ServerDeck.Add(NewCard);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Server] %s 카드 생성에 실패하여 덱에 추가하지 못했습니다."), *CardRowName.ToString());
		}
	}

	ShuffleDeck();

	DeckCount = ServerDeck.Num();
	OnDeckCountChangedDelegate.Broadcast(DeckCount);

	UE_LOG(LogTemp, Log, TEXT("[Server] 덱 '%s' (%d장) 로드 및 셔플 완료"), *InDeckData.DeckName, DeckCount);
}

void UPlayerCardComponent::ShuffleDeck()
{
	if (ServerDeck.Num() > 0)
	{
		const int32 LastIndex = ServerDeck.Num() - 1;
		for (int32 i = 0; i <= LastIndex; ++i)
		{
			int32 Index = DeckRandomStream.RandRange(i, LastIndex);
			if (i != Index) ServerDeck.Swap(i, Index);
		}
	}
}

UCardBase* UPlayerCardComponent::GenerateCardInstance(FName CardRowName)
{
	if (GetOwnerRole() != ROLE_Authority) return nullptr;

	UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(GetWorld()->GetGameInstance());
	AInGameMode* GameMode = GetWorld()->GetAuthGameMode<AInGameMode>();

	if (!GI || !GI->CardDataTable || !GameMode)
	{
		UE_LOG(LogTemp, Error, TEXT("[Server] 카드 생성을 위한 필수 시스템을 찾을 수 없습니다."));
		return nullptr;
	}

	FCardData* FoundCardInfo = GI->CardDataTable->FindRow<FCardData>(CardRowName, TEXT("GenerateCard"));
	if (FoundCardInfo)
	{
		// 소유자를 PlayerState(GetOwner)로 설정하여 UObject 생성
		UCardBase* NewCard = NewObject<UCardBase>(GetOwner());
		
		int32 NewInstanceID = GameMode->GenerateNextCardInstanceID(); 
		int32 BaseMana = FoundCardInfo->BaseManaCost; 

		NewCard->InitializeCard(
			NewInstanceID, 
			CardRowName, 
			BaseMana,
			FoundCardInfo->Force,
			FoundCardInfo->Intelligence,
			FoundCardInfo->Attack,
			FoundCardInfo->Health
		);
		
		return NewCard;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Server] %s 카드를 데이터 테이블에서 찾을 수 없습니다."), *CardRowName.ToString());
	return nullptr;
}

UCardBase* UPlayerCardComponent::FindCardInstanceByID(int32 InInstanceID) const
{
	if (GetOwnerRole() != ROLE_Authority || InInstanceID == -1) return nullptr;

	for (UCardBase* Card : ServerHand)
	{
		if (Card && Card->InstanceID == InInstanceID) return Card;
	}
	for (UCardBase* Card : ServerDeck)
	{
		if (Card && Card->InstanceID == InInstanceID) return Card;
	}
	for (UCardBase* Card : ServerObliterate)
	{
		if (Card && Card->InstanceID == InInstanceID) return Card;
	}
	for (UCardBase* Card : ServerExile)
	{
		if (Card && Card->InstanceID == InInstanceID) return Card;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Server] CardComponent 내부에서 InstanceID %d 번 카드를 찾을 수 없습니다."), InInstanceID);
	return nullptr;
}

bool UPlayerCardComponent::IsCardInHand(int32 InInstanceID) const
{
	if (InInstanceID == -1) return false;

	for (const UCardBase* Card : ServerHand)
	{
		if (Card && Card->InstanceID == InInstanceID)
		{
			return true;
		}
	}
	return false;
}

bool UPlayerCardComponent::RemoveCardFromHand(int32 InInstanceID, ECardZone DestinationZone)
{
	if (GetOwnerRole() != ROLE_Authority || InInstanceID == -1) return false;

	for (int32 i = 0; i < ServerHand.Num(); ++i)
	{
		UCardBase* Card = ServerHand[i];
		if (Card && Card->InstanceID == InInstanceID)
		{
			ServerHand.RemoveAt(i);

			if (DestinationZone == ECardZone::EXILE)
			{
				ServerExile.Add(Card);
			}
			else if (DestinationZone == ECardZone::OBLITERATE)
			{
				ServerObliterate.Add(Card);
			}

			else if (DestinationZone == ECardZone::BATTLEFIELD)
			{
				ServerBattlefield.Add(Card);
			}

			SyncHandDataToDisplay();
			return true;
		}
	}
	return false;
}

bool UPlayerCardComponent::ReturnCardToDeckAndDraw(int32 InInstanceID)
{
	if (GetOwnerRole() != ROLE_Authority) return false;

	UCardBase* CardToReturn = nullptr;
	int32 FoundIdx = INDEX_NONE;

	for (int32 i = 0; i < ServerHand.Num(); ++i)
	{
		if (ServerHand[i] && (ServerHand[i]->InstanceID == InInstanceID || (InInstanceID == -1 && i == 0)))
		{
			CardToReturn = ServerHand[i];
			FoundIdx = i;
			break;
		}
	}

	if (!CardToReturn || FoundIdx == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerCardComponent] ReturnCardToDeckAndDraw: 패에서 카드(ID: %d)를 찾을 수 없습니다."), InInstanceID);
		return false;
	}

	// 1. 패에서 카드 제거
	ServerHand.RemoveAt(FoundIdx);

	// 2. 덱에 추가 후 셔플
	ServerDeck.Add(CardToReturn);
	ShuffleDeck();
	DeckCount = ServerDeck.Num();
	OnDeckCountChangedDelegate.Broadcast(DeckCount);

	// 3. 덱에서 1장 드로우
	DrawCard();

	// 4. 패 데이터 동기화
	SyncHandDataToDisplay();

	UE_LOG(LogTemp, Log, TEXT("[PlayerCardComponent] ReturnCardToDeckAndDraw 완료: 카드 %s (ID: %d) 덱 반환 및 1장 드로우."),
		*CardToReturn->CardID.ToString(), CardToReturn->InstanceID);
	return true;
}

void UPlayerCardComponent::SyncHandDataToDisplay()
{
	if (GetOwnerRole() != ROLE_Authority) return;

	CurrentHandDisplay.Empty();
	CurrentHandDisplay.Reserve(ServerHand.Num());
	
	for (UCardBase* CardInstance : ServerHand)
	{
		if (IsValid(CardInstance))
		{
			FWildCardDisplayData DisplayData(
				CardInstance->InstanceID,
				CardInstance->CardID,
				CardInstance->CurrentManaCost,
				CardInstance->CurrentForce,
				CardInstance->CurrentIntelligence,
				CardInstance->CurrentAttack,
				CardInstance->CurrentHealth
			);
			CurrentHandDisplay.Add(DisplayData);
		}
	}

	HandCount = CurrentHandDisplay.Num();
	OnHandCountChangedDelegate.Broadcast(HandCount);
	OnMyHandUpdatedDelegate.Broadcast(CurrentHandDisplay);

	// [호스트/로컬 플레이어 대응] 로컬 플레이어가 서버(Authority)인 경우 RepNotify 수동 호출
	if (APlayerState* OwnerPS = Cast<APlayerState>(GetOwner()))
	{
		if (AInGameController* PC = Cast<AInGameController>(OwnerPS->GetPlayerController()))
		{
			if (PC->IsLocalController())
			{
				OnRep_CurrentHandDisplay();
			}
		}
	}
}

void UPlayerCardComponent::OnRep_CurrentHandDisplay()
{
	APlayerState* OwnerPS = Cast<APlayerState>(GetOwner());
	if (!OwnerPS) return;

	AInGameController* PC = Cast<AInGameController>(OwnerPS->GetPlayerController());
	if (!PC)
	{
		if (APlayerController* FirstPC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			if (FirstPC->GetPlayerState<APlayerState>() == OwnerPS)
			{
				PC = Cast<AInGameController>(FirstPC);
			}
		}
	}
	if (!PC || !PC->IsLocalController()) return;

	AInGameState* GS = GetWorld()->GetGameState<AInGameState>();
	if (!GS) return;
	
	EWildMatchState MatchState = GS->GetCurrentMatchState();

	if (MatchState == EWildMatchState::Mulligan)
	{
		PC->RefreshMulliganUI(CurrentHandDisplay);
	}
	else if (MatchState == EWildMatchState::MainGame || MatchState == EWildMatchState::Mulligan_End)
	{
		OnMyHandUpdatedDelegate.Broadcast(CurrentHandDisplay);
		UE_LOG(LogTemp, Log, TEXT("[Client] 최종 손패 데이터 수신됨 (MatchState: %s)"), *UEnum::GetValueAsString(MatchState));
	}
}

void UPlayerCardComponent::OnRep_HandCount()
{
	OnHandCountChangedDelegate.Broadcast(HandCount);
	
	if (APlayerState* OwnerPS = Cast<APlayerState>(GetOwner()))
	{
		UE_LOG(LogTemp, Log, TEXT("Player %d의 HandCount가 %d장으로 변경되었습니다!"), OwnerPS->GetPlayerId(), HandCount);
	}
}

void UPlayerCardComponent::OnRep_DeckCount()
{
	OnDeckCountChangedDelegate.Broadcast(DeckCount);
}