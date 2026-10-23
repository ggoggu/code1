#include "InGame/Game/InGamePlayerState.h"
#include "InGame/Game/InGamePawn.h"
#include "Net/UnrealNetwork.h"
#include "InGameController.h"
#include "PhaseComponent.h"
#include "InGameMode.h"
#include "InGameState.h"
#include "PlayerStatComponent.h"
#include "WildCard.h"
#include "WildCardGameInstance.h"
#include "ArtifactBase.h"
#include "InGame/Unit/PlayerUnit.h"
#include "InGame/Map/TileMap.h"
#include "InGame/Unit/TacticalSkillBase.h"
#include "UObject/ConstructorHelpers.h"
#include "InGame/Battle/WildCombatLogComponent.h"

AInGamePlayerState::AInGamePlayerState()
{
	bReplicates = true;
	SetNetUpdateFrequency(10.0f);

	PlayerClass = EWildPlayerClass::Common;
	SelectedTacticalSkillClass = nullptr;
	SelectedTacticalSkillID = NAME_None;

	static ConstructorHelpers::FClassFinder<APlayerUnit> PlayerBP(
		TEXT("/Game/InGame/Player/BP/BP_PlayerUnit.BP_PlayerUnit_C")
	);
	if (PlayerBP.Succeeded())
	{
		SelectedPlayerUnitClass = PlayerBP.Class;
	}

	StatComponent = CreateDefaultSubobject<UPlayerStatComponent>(TEXT("StatComponent"));
	
	// [신규] 카드 로직 전담 컴포넌트 생성 및 네트워크 복제 설정
	CardComponent = CreateDefaultSubobject<UPlayerCardComponent>(TEXT("CardComponent"));
	CardComponent->SetIsReplicated(true);
}

void AInGamePlayerState::BeginPlay()
{
	Super::BeginPlay();
}

void AInGamePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AInGamePlayerState, PlayerCount);
	DOREPLIFETIME(AInGamePlayerState, MySpawnableTiles);

	DOREPLIFETIME(AInGamePlayerState, bIsGamesetReady);
	DOREPLIFETIME(AInGamePlayerState, bIsMulliganReady);
	DOREPLIFETIME(AInGamePlayerState, bIsActionReady);
	
	DOREPLIFETIME(AInGamePlayerState, ArtifactDisplay);
	DOREPLIFETIME(AInGamePlayerState, MyPlayerUnit);
	DOREPLIFETIME(AInGamePlayerState, PlayerClass);
	DOREPLIFETIME(AInGamePlayerState, SelectedTacticalSkillClass);
	DOREPLIFETIME(AInGamePlayerState, SelectedTacticalSkillID);
}

void AInGamePlayerState::OnRep_PlayerCount()
{

	UE_LOG(LogTemp, Warning, TEXT("Client | %s | PlayerCount now: %d"), *GetName(), PlayerCount);

	if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
	{
		if (AInGamePawn* Pawn = PC->GetPawn<AInGamePawn>())
		{
			Pawn->ApplyCameraSettings();
		}
	}
	else if (APlayerController* LocalPC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (LocalPC->GetPlayerState<AInGamePlayerState>() == this)
		{
			if (AInGamePawn* Pawn = LocalPC->GetPawn<AInGamePawn>())
			{
				Pawn->ApplyCameraSettings();
			}
		}
	}
}

void AInGamePlayerState::SetPlayerUnit(APlayerUnit* InUnit)
{
	MyPlayerUnit = InUnit;
	if (MyPlayerUnit && PlayerClass != EWildPlayerClass::Common)
	{
		MyPlayerUnit->SetPlayerClass(PlayerClass);
	}
}

void AInGamePlayerState::OnRep_MyPlayerUnit()
{
	if (MyPlayerUnit)
	{
		UE_LOG(LogTemp, Log, TEXT("[Client] Player %d 의 PlayerUnit 이 복제되었습니다: %s"), GetPlayerId(), *MyPlayerUnit->GetName());

		if (PlayerClass != EWildPlayerClass::Common && MyPlayerUnit->GetPlayerClass() != PlayerClass)
		{
			MyPlayerUnit->SetPlayerClass(PlayerClass);
		}
		else
		{
			MyPlayerUnit->UpdateCardVisual();
		}

		// 로컬 플레이어의 Pawn에 카메라 갱신 통지
		if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
		{
			if (AInGamePawn* Pawn = PC->GetPawn<AInGamePawn>())
			{
				Pawn->ApplyCameraSettings();
			}
		}
		else if (APlayerController* LocalPC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			if (LocalPC->GetPlayerState<AInGamePlayerState>() == this)
			{
				if (AInGamePawn* Pawn = LocalPC->GetPawn<AInGamePawn>())
				{
					Pawn->ApplyCameraSettings();
				}
			}
		}
	}
}

void AInGamePlayerState::OnRep_MySpawnableTiles()
{
	// 클라이언트에서 소환 가능 타일 복제 완료 시 처리
}

// ==========================================================
// 멀리건 및 시작 데이터 처리
// ==========================================================

void AInGamePlayerState::Server_SubmitMulligan_Implementation(const TArray<int32>& CardsToSwap)
{
	UE_LOG(LogTemp, Warning, TEXT("[Server] Player %d 의 Server_SubmitMulligan RPC 수신 완료!"), GetPlayerId());

	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (!GameState || GameState->GetCurrentMatchState() != EWildMatchState::Mulligan) 
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] 멀리건 단계가 아니므로 요청을 무시합니다."));
		return;
	}

	if (CardsToSwap.Num() > 5)
	{
		UE_LOG(LogTemp, Error, TEXT("[Server] Player %d 가 비정상적인 개수(%d장)의 교체를 요청했습니다."), GetPlayerId(), CardsToSwap.Num());
		return;
	}

	// [보안 강화] 요청된 카드들이 실제 플레이어의 ServerHand에 존재하는지 서버 검증
	TArray<int32> ValidatedCardsToSwap;
	if (CardComponent)
	{
		for (int32 CardID : CardsToSwap)
		{
			if (CardComponent->IsCardInHand(CardID))
			{
				ValidatedCardsToSwap.AddUnique(CardID);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[Server] 보안: Player %d 가 손패에 없는 카드(InstanceID: %d)를 멀리건 교체 요청했습니다. 무시됩니다."), GetPlayerId(), CardID);
			}
		}
		CardComponent->CachedMulliganCards = ValidatedCardsToSwap;
	}

	AInGameController* PC = Cast<AInGameController>(GetPlayerController());
	AInGameMode* GameMode = Cast<AInGameMode>(GetWorld()->GetAuthGameMode());
	
	if (PC && GameMode && GameMode->PhaseComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] Player %d 멀리건 데이터 캐싱 완료 (%d장). 레디 상태로 전환합니다."), GetPlayerId(), ValidatedCardsToSwap.Num());
		GameMode->PhaseComponent->ProcessPlayerMulliganReady(PC);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Server] 필요한 컴포넌트를 찾을 수 없어 멀리건 처리를 중단합니다."));
	}
}

bool AInGamePlayerState::Server_SubmitMulligan_Validate(const TArray<int32>& CardsToSwap)
{
	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (GameState && GameState->GetCurrentMatchState() != EWildMatchState::Mulligan)
	{
		return false;
	}
	if (CardsToSwap.Num() > 5)
	{
		return false;
	}
	return true;
}


// ==========================================================
// 비공개 메인 행동 (Action_Private) 상태 관리
// ==========================================================

void AInGamePlayerState::OnRep_IsActionReady()
{
	OnActionReadyChangedDelegate.Broadcast(bIsActionReady);
}

void AInGamePlayerState::SetPendingMainAction(const FWildActionData& InActionData)
{
	if (HasAuthority())
	{
		PendingMainAction = InActionData;
		bIsActionReady = true; 
		OnRep_IsActionReady();

		UE_LOG(LogWildPhase, Log, TEXT("[Server] Player %d 의 비공개 행동이 성공적으로 캐싱되었습니다. ActionType: %d"), 
			GetPlayerId(), (int32)PendingMainAction.ActionType);
	}
}

const FWildActionData& AInGamePlayerState::GetPendingMainAction() const
{
	return PendingMainAction;
}

void AInGamePlayerState::ClearPendingMainAction()
{
	if (HasAuthority())
	{
		PendingMainAction = FWildActionData();
		bIsActionReady = false; 
		OnRep_IsActionReady();
	}
}

// ==========================================================
// 성유물 연동 로직
// ==========================================================

void AInGamePlayerState::Server_SubmitArtifact_Implementation(FName ArtifactRowName)
{
	UE_LOG(LogTemp, Warning, TEXT("[Server] Player %d 의 Server_SubmitArtifact RPC 수신! 선택 품목: %s"), GetPlayerId(), *ArtifactRowName.ToString());

	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (!GameState || GameState->GetCurrentMatchState() != EWildMatchState::Gameset) 
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] Gameset 단계가 아니므로 성유물 선택 요청을 무시합니다."));
		return;
	}

	UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(GetGameInstance());
	AInGameMode* GameMode = Cast<AInGameMode>(GetWorld()->GetAuthGameMode());

	if (!GI || !GI->ArtifactDataTable || !GameMode)
	{
		UE_LOG(LogTemp, Error, TEXT("[Server] 성유물 생성을 위한 필수 시스템을 찾을 수 없습니다."));
		return;
	}

	FArtifactData* FoundArtifactInfo = GI->ArtifactDataTable->FindRow<FArtifactData>(ArtifactRowName, TEXT("GenerateArtifact"));
	if (!FoundArtifactInfo)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] %s 성유물을 데이터 테이블에서 찾을 수 없습니다."), *ArtifactRowName.ToString());
		return;
	}

	UArtifactBase* NewArtifact = NewObject<UArtifactBase>(this);
	if (NewArtifact)
	{
		NewArtifact->InitializeArtifact(
			ArtifactRowName, 
			FoundArtifactInfo->MaxDurability, 
			FoundArtifactInfo->EffectClasses
		);
		
		EquippedArtifact = NewArtifact;
		UE_LOG(LogTemp, Log, TEXT("[Server] Player %d 의 성유물 캐싱 완료 (비공개 상태)"), GetPlayerId());
	}
}

bool AInGamePlayerState::Server_SubmitArtifact_Validate(FName ArtifactRowName)
{
	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (GameState && GameState->GetCurrentMatchState() != EWildMatchState::Gameset)
	{
		return false;
	}
	return true;
}

void AInGamePlayerState::SyncArtifactToDisplay()
{
	if (!HasAuthority() || !EquippedArtifact) return;

	ArtifactDisplay = FWildArtifactDisplayData(
		EquippedArtifact->GetArtifactID(),
		EquippedArtifact->GetCurrentDurability(),
		EquippedArtifact->GetMaxDurability(),
		EquippedArtifact->IsArtifactActive()
	);
	
	ForceNetUpdate(); 
	UE_LOG(LogTemp, Warning, TEXT("[Server] Player %d 의 성유물 데이터가 클라이언트로 공개 동기화되었습니다!"), GetPlayerId());
}

void AInGamePlayerState::OnRep_ArtifactDisplay()
{
	UE_LOG(LogTemp, Log, TEXT("[Client] 성유물 정보 동기화 완료: %s (내구도: %d)"), 
		*ArtifactDisplay.ArtifactID.ToString(), ArtifactDisplay.CurrentDurability);
}

TSubclassOf<APlayerUnit> AInGamePlayerState::GetPlayerUnitClass() const
{
	return SelectedPlayerUnitClass ? SelectedPlayerUnitClass : TSubclassOf<APlayerUnit>(APlayerUnit::StaticClass());
}

void AInGamePlayerState::OnRep_PlayerClass()
{
	UE_LOG(LogTemp, Log, TEXT("[Client] Player %d Class Replicated: %d"), GetPlayerId(), static_cast<int32>(PlayerClass));
	if (APlayerUnit* Unit = GetPlayerUnit())
	{
		Unit->SetPlayerClass(PlayerClass);
	}
}

void AInGamePlayerState::SetSelectedTacticalSkillClass(TSubclassOf<UTacticalSkillBase> InSkillClass)
{
	SelectedTacticalSkillClass = InSkillClass;
	if (HasAuthority() && MyPlayerUnit)
	{
		MyPlayerUnit->SetTacticalSkill(InSkillClass);
	}
}

void AInGamePlayerState::SetSelectedTacticalSkillID(FName InSkillID)
{
	SelectedTacticalSkillID = InSkillID;
}

void AInGamePlayerState::OnRep_SelectedTacticalSkillClass()
{
	UE_LOG(LogTemp, Log, TEXT("[Client] Player %d SelectedTacticalSkillClass Replicated: %s"),
		GetPlayerId(), SelectedTacticalSkillClass ? *SelectedTacticalSkillClass->GetName() : TEXT("None"));
}

void AInGamePlayerState::OnRep_SelectedTacticalSkillID()
{
	UE_LOG(LogTemp, Log, TEXT("[Client] Player %d SelectedTacticalSkillID Replicated: %s"),
		GetPlayerId(), *SelectedTacticalSkillID.ToString());
}

void AInGamePlayerState::ApplyDeckData(const FWildDeckData& InDeckData)
{
	if (!HasAuthority()) return;

	PlayerClass = InDeckData.DeckClass;
	SelectedTacticalSkillID = InDeckData.SelectedTacticalSkillID;

	if (InDeckData.SelectedTacticalSkillClass)
	{
		SetSelectedTacticalSkillClass(InDeckData.SelectedTacticalSkillClass);
	}
	else if (InDeckData.SelectedTacticalSkillID != NAME_None)
	{
		if (UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(GetGameInstance()))
		{
			if (GI->TacticalSkillDataTable)
			{
				FTacticalSkillData* Row = GI->TacticalSkillDataTable->FindRow<FTacticalSkillData>(
					InDeckData.SelectedTacticalSkillID, TEXT("ApplyDeckData"));
				if (Row && Row->SkillClass)
				{
					SetSelectedTacticalSkillClass(Row->SkillClass);
				}
			}
		}
	}

	if (APlayerUnit* Unit = GetPlayerUnit())
	{
		Unit->SetPlayerClass(PlayerClass);
		if (SelectedTacticalSkillClass)
		{
			Unit->SetTacticalSkill(SelectedTacticalSkillClass);
		}
	}
	else if (AInGameState* GS = GetWorld()->GetGameState<AInGameState>())
	{
		if (ATileMap* Map = GS->GetMap())
		{
			Map->TESTUnitSpawn();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Server] Player %d 에게 덱 데이터 적용 완료 (직업: %d, 스킬ID: %s)"),
		GetPlayerId(), static_cast<int32>(PlayerClass), *SelectedTacticalSkillID.ToString());
}

void AInGamePlayerState::Client_ReceiveCombatLogEntry_Implementation(const FWildCombatLogEntry& Entry)
{
	if (AInGameState* GS = GetWorld()->GetGameState<AInGameState>())
	{
		if (UWildCombatLogComponent* LogComp = GS->GetCombatLogComponent())
		{
			LogComp->AddLogEntryOnClient(Entry);
		}
	}
}
