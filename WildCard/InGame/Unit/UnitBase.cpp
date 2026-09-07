// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Unit/UnitBase.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "InGameState.h"
#include "ChainComponent.h"



// Sets default values
AUnitBase::AUnitBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
}

// Called when the game starts or when spawned
void AUnitBase::BeginPlay()
{
	Super::BeginPlay();

}

void AUnitBase::SetCardHealth(int32 InHealth)
{
	UnitCardData.Health = InHealth;

	if (UnitCardData.Health <= 0)
	{
		Destroy();
	}

	SetWidgetScore();
}

void AUnitBase::InitUnitCardData(FCardData InCardData, int32 InOwnerId)
{
	CardData = InCardData;
	UnitCardData = FUnitCardData(InCardData, InOwnerId);
	UE_LOG(LogTemp, Warning, TEXT("Card Initialized! Card Health: %d, OwnerID: %d"), UnitCardData.Health, UnitCardData.OwnerId);

	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (GameState == nullptr)
		return;

	UChainComponent* ChainComponent = GameState->FindComponentByClass<UChainComponent>();
	if (ChainComponent == nullptr)
		return;;

	//#. ActionData 생성 후 ChainComponent에 추가
	FWildActionData ActionData;
	//
	// TODO : 생성관련 action data만들기
	ActionData.ActionCategory = EWildActionCategory::Main;
	ActionData.ActionType = EWildActionType::SummonUnit;
	ActionData.SourceUnit = this;
	ActionData.TargetTile = CurrentTile;
	//
	ChainComponent->AddActionToChain(ActionData);

	//#. 서버에서 widget도 설정
	SetWidgetScore();
}

void AUnitBase::OnRep_UnitCardDataChanged()
{
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AUnitBase::SetWidgetScore);
}

void AUnitBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AUnitBase, UnitCardData);

}




