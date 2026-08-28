// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Game/InGameMode.h"
#include "PhaseComponent.h"
#include "InGamePlayerState.h"

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
		PS->CreateTestDeck();
		PS->ForceNetUpdate();
	}

	Server_PlayerCount++;
	
	if (GetNumPlayers() >= 2) 
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] 2 Players connected. Starting Gameset Phase!"));
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
	
		// �÷��̾� 2�� ���� Ȯ��
		if (GetNumPlayers() >= 2) {
			// TODO : ���� ���� ���� �߰�
			WILD_UELOG(Warning, TEXT("Not Logic in here"));
		}
		*/
}
