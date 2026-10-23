// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Game/InGameMode.h"
#include "PhaseComponent.h"
#include "InGamePlayerState.h"
#include "InGameState.h"
#include "InGame/Unit/PlayerUnit.h"
#include "InGame/Unit/TacticalSkillBase.h"
#include "InGame/Map/Tile.h"
#include "InGame/Battle/PlayerStatComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "WildCardGameInstance.h"
#include "WildDeckSubsystem.h"
#include "WildMatchmakingSubsystem.h"

AInGameMode::AInGameMode()
{
	PhaseComponent = CreateDefaultSubobject<UPhaseComponent>(TEXT("PhaseComponent"));
	Server_PlayerCount = 0;
}

void AInGameMode::BeginPlay()
{
	Super::BeginPlay();

	/*
	if (EffectManagerClass)
	{
		EffectManagerInstance = GetWorld()->SpawnActor<AWildProtoEffectManager>(
			EffectManagerClass,
			FVector(0.f, 0.f, 10000.f),
			FRotator::ZeroRotator
		);
	}
	*/
}

int32 AInGameMode::GenerateNextCardInstanceID()
{
	return CurrentCardInstanceID++;
}

void AInGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AInGamePlayerState* PS = NewPlayer->GetPlayerState<AInGamePlayerState>();
	if (PS)
	{
		PS->PlayerCount = Server_PlayerCount;

		bool bDeckLoaded = false;
		if (UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(GetGameInstance()))
		{
			if (UWildDeckSubsystem* DeckSub = GI->GetSubsystem<UWildDeckSubsystem>())
			{
				if (DeckSub->HasSelectedDeck())
				{
					const FWildDeckData& DeckData = DeckSub->GetSelectedDeck();
					PS->ApplyDeckData(DeckData);
					PS->InitDeckFromData(DeckData);
					bDeckLoaded = true;
					UE_LOG(LogTemp, Log, TEXT("[InGameMode] Player %d 에게 선택된 덱 '%s' (%d장) 적용 완료"),
						PS->GetPlayerId(), *DeckData.DeckName, DeckData.CardIDs.Num());
				}
			}

			if (!bDeckLoaded && GI->SelectedDeck.CardIDs.Num() > 0)
			{
				PS->ApplyDeckData(GI->SelectedDeck);
				PS->InitDeckFromData(GI->SelectedDeck);
				bDeckLoaded = true;
				UE_LOG(LogTemp, Log, TEXT("[InGameMode] Player %d 에게 GI 선택 덱 '%s' (%d장) 적용 완료"),
					PS->GetPlayerId(), *GI->SelectedDeck.DeckName, GI->SelectedDeck.CardIDs.Num());
			}
		}

		if (!bDeckLoaded)
		{
			PS->CreateTestDeck();
		}

		// 개발자 모드: 시작 마나 설정
		bool bEnableDevMana = bDevStartWithTenMana;
		int32 TargetMana = DevStartingMana;

		if (FParse::Param(FCommandLine::Get(), TEXT("DevMana10")))
		{
			bEnableDevMana = true;
			TargetMana = 10;
		}
		else if (FParse::Param(FCommandLine::Get(), TEXT("NormalMana")))
		{
			bEnableDevMana = false;
		}

		if (bEnableDevMana)
		{
			if (UPlayerStatComponent* Stat = PS->GetStatComponent())
			{
				Stat->SetMana(TargetMana);
				UE_LOG(LogTemp, Log, TEXT("[Server] 개발자 모드 활성화: Player %d 의 시작 마나를 %d 로 설정했습니다."), PS->GetPlayerId(), TargetMana);
			}
		}

		PS->ForceNetUpdate();
	}

	Server_PlayerCount++;

	bool bIsBotMatch = false;
	if (UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(GetGameInstance()))
	{
		if (UWildMatchmakingSubsystem* MatchSub = GI->GetSubsystem<UWildMatchmakingSubsystem>())
		{
			bIsBotMatch = MatchSub->GetLastMatchResult().Opponent.bIsBot;
		}
	}

	const bool bIsStandaloneSolo = (GetNetMode() == NM_Standalone && GetNumPlayers() == 1);

	if (GetNumPlayers() >= 2 || bIsBotMatch || bIsStandaloneSolo) 
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] Match start conditions satisfied (Players: %d, BotMatch: %d, StandaloneSolo: %d). Starting Gameset Phase!"),
			GetNumPlayers(), bIsBotMatch ? 1 : 0, bIsStandaloneSolo ? 1 : 0);
		PhaseComponent->SetMatchState(EWildMatchState::Gameset);
	}

}

void AInGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	/*
		AWildProtoPlayerState* PS = NewPlayer->GetPlayerState<AWildProtoPlayerState>();
		PS->PlayerCount = Server_PlayerCount;
		PS->ForceNetUpdate();
	
		Server_PlayerCount++;
	
		// ÷̾ 2  Ȯ
		if (GetNumPlayers() >= 2) {
			// TODO :    ߰
			WILD_UELOG(Warning, TEXT("Not Logic in here"));
		}
		*/
}

void AInGameMode::HandlePlayerDefeat(int32 DefeatedPlayerId)
{
	if (!HasAuthority()) return;

	UE_LOG(LogTemp, Warning, TEXT("[InGameMode] Player %d was defeated! Concluding match."), DefeatedPlayerId);

	AInGameState* GS = GetGameState<AInGameState>();
	if (GS)
	{
		int32 WinnerId = -1;
		for (APlayerState* PS : GS->PlayerArray)
		{
			AInGamePlayerState* WildPS = Cast<AInGamePlayerState>(PS);
			if (WildPS && WildPS->GetPlayerId() != DefeatedPlayerId)
			{
				WinnerId = WildPS->GetPlayerId();
				break;
			}
		}

		GS->SetWinningPlayerID(WinnerId, DefeatedPlayerId);
	}

	if (PhaseComponent)
	{
		PhaseComponent->SetMatchState(EWildMatchState::Match_End);
	}
}

APlayerUnit* AInGameMode::SpawnPlayerUnitForPlayer(AInGamePlayerState* InPlayerState, ATile* StartTile, TSubclassOf<APlayerUnit> PlayerUnitClass)
{
	if (!HasAuthority() || !InPlayerState || !StartTile) return nullptr;

	TSubclassOf<APlayerUnit> ClassToSpawn = PlayerUnitClass ? PlayerUnitClass : InPlayerState->GetPlayerUnitClass();
	EWildPlayerClass InClass = InPlayerState->GetPlayerClass();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = StartTile;

	APlayerUnit* NewPlayerUnit = GetWorld()->SpawnActor<APlayerUnit>(
		ClassToSpawn,
		StartTile->GetActorLocation(),
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (NewPlayerUnit)
	{
		StartTile->SetPlacedUnit(NewPlayerUnit);
		NewPlayerUnit->SetCurrentTile(StartTile);
		TSubclassOf<UTacticalSkillBase> SelectedSkill = InPlayerState->GetSelectedTacticalSkillClass();
		NewPlayerUnit->InitializePlayerUnit(InPlayerState, InClass, nullptr, SelectedSkill);
		UE_LOG(LogTemp, Log, TEXT("[InGameMode] Spawned PlayerUnit for Player %d on Tile %s (Class: %s, TacticalSkill: %s)"), 
			InPlayerState->GetPlayerId(), *StartTile->GetName(), *ClassToSpawn->GetName(),
			SelectedSkill ? *SelectedSkill->GetName() : TEXT("Default"));
	}

	return NewPlayerUnit;
}

void AInGameMode::SpawnInitialPlayerUnits()
{
	if (!HasAuthority()) return;

	AInGameState* GS = GetGameState<AInGameState>();
	if (!GS) return;

	TArray<TWeakObjectPtr<ATile>> AllTiles = GS->GetTilesArray();
	if (AllTiles.Num() == 0) return;

	int32 PlayerIdx = 0;
	for (APlayerState* PS : GS->PlayerArray)
	{
		if (AInGamePlayerState* WildPS = Cast<AInGamePlayerState>(PS))
		{
			if (WildPS->GetPlayerUnit() != nullptr)
			{
				continue;
			}

			int32 TileIndex = (PlayerIdx == 0) ? 0 : FMath::Clamp(AllTiles.Num() - 1, 0, AllTiles.Num() - 1);
			if (AllTiles.IsValidIndex(TileIndex) && AllTiles[TileIndex].IsValid())
			{
				ATile* StartTile = AllTiles[TileIndex].Get();
				SpawnPlayerUnitForPlayer(WildPS, StartTile);
			}
			PlayerIdx++;
		}
	}
}

