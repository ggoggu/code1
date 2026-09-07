// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Game/InGameState.h"
#include "Net/UnrealNetwork.h"
#include "CardBase.h"
#include "ChainComponent.h"
#include "InGamePlayerState.h"
#include "InGameController.h"
#include "UnitBase.h"
#include "WildCard.h"
#include "TileMapMaker.h"
#include "TileMap.h"

AInGameState::AInGameState()
{
	//bReplicates = true;

	ChainComponent = CreateDefaultSubobject<UChainComponent>(TEXT("ChainComponent"));
	CurrentMatchState = EWildMatchState::Init;
	CurrentPhase = EWildCardPhase::None;
	CurrentPriorityPlayerID = -1;
	PhaseRemainTime = 0;
}

void AInGameState::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		InitMap();
		CreateMapTile();
	}
}

void AInGameState::InitMap()
{
	if (MapMakerClass)
	{
		FVector Location(100.0f, 200.0f, 5000.0f);
		FRotator Rotation(0.0f, 0.0f, 0.0f);
		FActorSpawnParameters SpawnParams;

		SpawnParams.Owner = this;

		MapMaker = GetWorld()->SpawnActor<ATileMapMaker>(
			MapMakerClass,
			Location,
			Rotation,
			SpawnParams
		);

		if (!IsValid(MapMaker))
		{
			WILD_UELOG(Error, TEXT("Map maker spawn error"));
		}
		else
			WILD_UELOG(Warning, TEXT("Map maker spawn"));

	}

	UE_LOG(LogTemp, Warning, TEXT("Actor Location: %s"),
		*MapMaker->GetActorLocation().ToString());

	UE_LOG(LogTemp, Warning, TEXT("Root Location: %s"),
		*MapMaker->GetRootComponent()->GetComponentLocation().ToString());
}

void AInGameState::CreateMapTile()
{

	UE_LOG(LogTemp, Warning, TEXT("HasAuthority: %d"), HasAuthority());
	UE_LOG(LogTemp, Warning, TEXT("GameState Addr: %p"), this);
	UE_LOG(LogTemp, Warning, TEXT("MapMaker Addr: %p"), MapMaker.Get());

	FVector Location(0.0f, 0.0f, 0.0f);
	if (MapMaker)
	{
		Map = MapMaker->CreateMap(Location);
		UE_LOG(LogTemp, Display, TEXT("Map create"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Map create Error"));
	}
}

void AInGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AInGameState, CurrentMatchState);
	DOREPLIFETIME(AInGameState, CurrentPhase);
	DOREPLIFETIME(AInGameState, CurrentPriorityPlayerID);
	DOREPLIFETIME(AInGameState, PhaseRemainTime);
	DOREPLIFETIME(AInGameState, CurrentChainPromptID);
	DOREPLIFETIME(AInGameState, MapMaker);
	DOREPLIFETIME(AInGameState, UnitsArray);
	DOREPLIFETIME(AInGameState, TilesArray);
}

void AInGameState::HandleUnitSpawned(TWeakObjectPtr<AUnitBase> InSpawnedUnit, TWeakObjectPtr<ATile> InSpawnedTile)
{
	WILD_SIMPLE(Warning);
	UnitsArray.AddUnique(InSpawnedUnit);
	TilesArray.AddUnique(InSpawnedTile);

	for (APlayerState* PS : PlayerArray)
	{
		AInGamePlayerState* MyPS = Cast<AInGamePlayerState>(PS);

		UE_LOG(LogTemp, Warning, TEXT("PlayerCount : %d , OwnerID : %d"), MyPS->PlayerCount, InSpawnedUnit->GetOwnerId());
		if (MyPS && MyPS->PlayerCount == InSpawnedUnit->GetOwnerId())
		{
			//  일치하는 PlayerState를 찾았으면 타일 추가
			
			for (ATile* InTile : SelectCanSpawnUnitTiles(InSpawnedTile, 1))
			{
				UE_LOG(LogTemp, Warning, TEXT("%s"), *InTile->GetName());
				MyPS->MySpawnableTiles.AddUnique(InTile);
			}
			break;
		}
		
	}
	
}

void AInGameState::HandleUnitDestroyed(TWeakObjectPtr<AUnitBase> InDestroyedUnit, TWeakObjectPtr<ATile> InDestroyedTile)
{
	UnitsArray.Remove(InDestroyedUnit);
	TilesArray.Remove(InDestroyedTile);

	for (APlayerState* PS : PlayerArray)
	{
		
		AInGamePlayerState* MyPS = Cast<AInGamePlayerState>(PS);
		if (MyPS && MyPS->PlayerCount == InDestroyedUnit->GetOwnerId())
		{
			// 2. 일치하는 PlayerState를 찾았으면 타일 추가
			for (ATile* InTile : SelectCanSpawnUnitTiles(InDestroyedTile, 1))
			{
				MyPS->MySpawnableTiles.Emplace(InTile);
			}
			break;
		}
		
	}
}

bool AInGameState::Server_FindCanMoveUnitTiles_Validate(ATile* InTile, AInGameController* InPlayer)
{
	return true;
}

void AInGameState::Server_FindCanMoveUnitTiles_Implementation(ATile* InTile, AInGameController* InPlayer)
{
	TArray<ATile*> CanMoveTiles = SelectCanSpawnUnitTiles(InTile, 1);
	TSet<TWeakObjectPtr<ATile>> TileSet(TilesArray);

	CanMoveTiles.RemoveAll([&](ATile* Tile) {
		return TileSet.Contains(Tile);
		});

	//InPlayer->Client_ChangeMoveableTiles(CanMoveTiles);
}

void AInGameState::Server_FindCanCombatUnitTiles_Implementation(ATile* InTile, AInGameController* InPlayer)
{
	TArray<ATile*> CombatTiles = SelectCanSpawnUnitTiles(InTile, 1);
	TArray<ATile*> CanCombatTiles;
	TSet<TWeakObjectPtr<ATile>> TileSet(TilesArray);

	int32 PSID = InPlayer->GetPlayerState<AInGamePlayerState>()->PlayerCount;

	for (ATile* Tile : CombatTiles)
	{
		if (TileSet.Contains(Tile) && (Tile->GetUnit()->GetOwnerId() != PSID))
		{
			CanCombatTiles.AddUnique(Tile);
		}
	}

	//InPlayer->Client_ChangeCombatableTiles(CanCombatTiles);
}

bool AInGameState::Server_FindCanCombatUnitTiles_Validate(ATile* InTile, AInGameController* InPlayer)
{
	return true;
}

void AInGameState::OnRep_MapMaker()
{
	WILD_UELOG(Error, TEXT("Map Maker Spawn"));
}



void AInGameState::SetCurrentPhase(EWildCardPhase NewPhase)
{
	if (HasAuthority())
	{

		if (CurrentPhase != NewPhase)
		{
			CurrentPhase = NewPhase;
			OnPhaseChangedDelegate.Broadcast(CurrentPhase);
		}
	}
}

void AInGameState::RegisterUnit(AUnitBase* NewUnit)
{
	if (HasAuthority() && IsValid(NewUnit))
	{
		ActiveUnitsOnBoard.AddUnique(NewUnit);
	}		
}

void AInGameState::UnregisterUnit(AUnitBase* DeadUnit)
{
	if (HasAuthority() && DeadUnit)
	{
		ActiveUnitsOnBoard.Remove(DeadUnit);
	}
}

TArray<ATile*> AInGameState::SelectCanSpawnUnitTiles(TWeakObjectPtr<ATile> InSpawnedTile, int32 InDistance)
{
	
	//#. 소환가능한 타일의 색깔 및 정보 변경
	TArray<ATile*> CanSpawnTiles;
	//#. 해당 Actor기준 거리가 1인 타일만을 찾아

	CanSpawnTiles.Append(Map->GetTilesAtDistance(InSpawnedTile, InDistance));

	return CanSpawnTiles;
	
}


void AInGameState::SetPhaseRemainTime(int32 NewTime)
{
	if (HasAuthority())
	{
		PhaseRemainTime = NewTime;
		OnTimerChangedDelegate.Broadcast(PhaseRemainTime);
	}
}

void AInGameState::SetCurrentPriorityPlayerID(int32 NewPlayerID)
{
	if (HasAuthority() && CurrentPriorityPlayerID != NewPlayerID)
	{
		CurrentPriorityPlayerID = NewPlayerID;
		OnRep_PriorityPlayerChanged();
		UE_LOG(LogWildPhase, Log, TEXT("[Phase] 우선권이 Player %d 에게 넘어갔습니다."), CurrentPriorityPlayerID);
	}
}

void AInGameState::SetCurrentChainPromptID(int32 InPlayerID)
{
	if (HasAuthority())
	{
		CurrentChainPromptID = InPlayerID;
		OnRep_ChainPromptPlayerChanged();
		UE_LOG(LogWildPhase, Log, TEXT(">>> [응답 권한] Player %d 에게 체인 여부를 묻습니다."), CurrentChainPromptID);
	}
}

void AInGameState::SetCurrentMatchState(EWildMatchState NewState)
{
	if (HasAuthority())
	{
		if (CurrentMatchState != NewState)
		{
			CurrentMatchState = NewState;
			
			// 서버에서도 델리게이트 호출
			OnMatchStateChangedDelegate.Broadcast(CurrentMatchState);
			
			// [로직] 매치 상태가 MainGame이 아니게 되면 페이즈를 None으로 초기화
			if (CurrentMatchState != EWildMatchState::MainGame)
			{
				SetCurrentPhase(EWildCardPhase::None);
			}

			UE_LOG(LogWildPhase, Log, TEXT(">>> [MatchState Changed] %s"), *UEnum::GetValueAsString(CurrentMatchState));
		}
	}
}


void AInGameState::OnRep_CurrentMatchState()
{
	OnMatchStateChangedDelegate.Broadcast(CurrentMatchState);
	UE_LOG(LogTemp, Log, TEXT("Client MatchState Sync: %s"), *UEnum::GetValueAsString(CurrentMatchState));
}

void AInGameState::OnRep_CurrentPhase()
{
	OnPhaseChangedDelegate.Broadcast(CurrentPhase);
}

void AInGameState::OnRep_PriorityPlayerChanged()
{
	UE_LOG(LogTemp, Log, TEXT("선공권 변경: Player %d"), CurrentPriorityPlayerID);	
}

void AInGameState::OnRep_PhaseRemainTime()
{
	OnTimerChangedDelegate.Broadcast(PhaseRemainTime);
}

void AInGameState::OnRep_ChainPromptPlayerChanged()
{
	UE_LOG(LogTemp, Log, TEXT("체인 권한 발생: Player %d"), CurrentChainPromptID);
}

void AInGameState::OnRep_UnitArrayChanged()
{
	WILD_SIMPLE(Warning);
}

void AInGameState::OnRep_TileArrayChanged()
{
	WILD_SIMPLE(Warning);
}
