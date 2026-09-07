// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Battle/PhaseComponent.h"
#include "InGameState.h"
#include "InGamePlayerState.h"
#include "ChainComponent.h"
#include "WildCard.h"
#include "PlayerStatComponent.h"
#include "CardBase.h"


// Sets default values for this component's properties
UPhaseComponent::UPhaseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	MulliganReadyCount = 0;
}


// Called when the game starts
void UPhaseComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwnerRole() == ROLE_Authority)
	{
		SetMatchState(EWildMatchState::Init);
	}
	
}

void UPhaseComponent::SetMatchState(EWildMatchState NewState)
{
	if (GetOwnerRole() != ROLE_Authority) return;

	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (!GameState) return;

	// GameState를 통해 상태 변경 (UI 브로드캐스트 발생)
	GameState->SetCurrentMatchState(NewState);

	switch (NewState)
	{
	case EWildMatchState::Init:
		GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);
		break;

	case EWildMatchState::Gameset:
		if (GameState->PlayerArray.Num() > 0)
		{
			GameState->SetCurrentPriorityPlayerID(GameState->PlayerArray[0]->GetPlayerId());
			UE_LOG(LogWildPhase, Log, TEXT("[System] 게임 세팅 시작. 최초 선공권은 Player %d 입니다."), GameState->GetCurrentPriorityPlayerID());
		}
		
		GameState->SetPhaseRemainTime(GamesetTime); 
		GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, this, &UPhaseComponent::OnGlobalTimerTick, 1.0f, true);
		break;

	case EWildMatchState::Mulligan:
		UE_LOG(LogWildPhase, Log, TEXT("[Phase] Mulligan 진입"));
		// 모든 플레이어에게 더미 카드 5장 지급
		for (APlayerState* State : GameState->PlayerArray)
		{
			if (AInGamePlayerState* PS = Cast<AInGamePlayerState>(State))
			{
				PS->SyncArtifactToDisplay();
			}
		}

		// (기존 로직) 모든 플레이어에게 더미 카드 5장 지급
		for (APlayerState* State : GameState->PlayerArray)
		{
			if (AInGamePlayerState* PS = Cast<AInGamePlayerState>(State))
			{
				PS->Debug_GiveDummyStartingHand();
			}
		}

		GameState->SetPhaseRemainTime(MulliganTime);
		GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, this, &UPhaseComponent::OnGlobalTimerTick, 1.0f, true);
		break;

	case EWildMatchState::Mulligan_End:
		{
			UE_LOG(LogWildPhase, Log, TEXT("[Phase] Mulligan_End: 서버에서 카드 교체 및 최종 패 확정 중..."));
			GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);

			// 1. 모든 플레이어의 멀리건 교체 로직을 일괄 실행합니다.
			for (APlayerState* State : GameState->PlayerArray)
			{
				if (AInGamePlayerState* PS = Cast<AInGamePlayerState>(State))
				{
					// 2단계에서 만든 교체 함수 호출
					PS->ExecuteMulliganSwap();

					// 2. 교체가 끝난 최종 패 데이터를 클라이언트로 동기화합니다.
					// 이 호출로 인해 각 클라이언트의 OnRep_CurrentHandDisplay가 트리거됩니다.
					PS->SyncHandDataToDisplay();
				}
			}

			// 3. (옵션) 교체 애니메이션을 위한 아주 짧은 대기 시간을 줄 수도 있습니다.
			// 여기서는 즉시 본 게임으로 전환합니다.
			SetMatchState(EWildMatchState::MainGame);
		}
		break;

	case EWildMatchState::MainGame:
		UE_LOG(LogWildPhase, Log, TEXT("[Phase] 본 게임(MainGame) 진입. 첫 턴을 시작합니다!"));
		GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);
		
		// [중요] 여기서 턴 사이클의 시동을 겁니다!
		SetGamePhase(EWildCardPhase::Turn_Start);
		break;

	case EWildMatchState::Match_End:
		GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);
		UE_LOG(LogWildPhase, Log, TEXT("[Phase] 게임 종료!"));
		break;
	}
}

void UPhaseComponent::SetGamePhase(EWildCardPhase NewPhase)
{
	if (GetOwnerRole() != ROLE_Authority) return;

	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (!GameState) return;

	// [안전장치] 본 게임 진행 중이 아닐 때는 페이즈를 변경할 수 없음
	if (GameState->GetCurrentMatchState() != EWildMatchState::MainGame)
	{
		UE_LOG(LogWildPhase, Warning, TEXT("본 게임(MainGame) 상태가 아니므로 페이즈를 %d로 변경할 수 없습니다."), (int32)NewPhase);
		return;
	}

	GameState->SetCurrentPhase(NewPhase);

	switch (NewPhase)
	{
	case EWildCardPhase::Turn_Start:
		GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);
		HandleTurnStart(); 
		break;

	case EWildCardPhase::Action_Private:
		GameState->SetPhaseRemainTime(ActionPrivateTime);
		GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, this, &UPhaseComponent::OnGlobalTimerTick, 1.0f, true);
		break;

	case EWildCardPhase::Action_Public:
		UE_LOG(LogWildPhase, Log, TEXT("[Phase] Action_Public 진입"));
		ConsecutivePassCount = 0; 
		
		GameState->SetPhaseRemainTime(ActionPublicTime);
		GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, this, &UPhaseComponent::OnGlobalTimerTick, 1.0f, true);
		break;

	case EWildCardPhase::Action_Resolution:
		GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);
		if (UChainComponent* ChainComp = GameState->FindComponentByClass<UChainComponent>())
		{
			ChainComp->ResolveChain();
		}
		break;

	case EWildCardPhase::Turn_End:
		GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);
		HandleTurnEnd();
		break;

	case EWildCardPhase::None:
	default:
		GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);
		GameState->SetPhaseRemainTime(0);
		break;
	}
	
}

void UPhaseComponent::ProcessPlayPublicAction(APlayerController* PC, const FWildActionData& ActionData)
{
	if (GetOwnerRole() != ROLE_Authority || !PC) return;

	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	AInGamePlayerState* PS = PC->GetPlayerState<AInGamePlayerState>();

	if (!GameState || !PS) return;

	int32 CallerID = PS->GetPlayerId();
	
	// 현재 페이즈가 Action_Public인지, 그리고 이 플레이어에게 우선권이 있는지 확인
	if (GameState->GetCurrentPhase() != EWildCardPhase::Action_Public)
	{
		UE_LOG(LogWildPhase, Warning, TEXT("[보안] Player %d 가 잘못된 페이즈에 액션을 시도했습니다."), CallerID);
		return;
	}

	if (GameState->GetCurrentChainPromptID() != CallerID)
	{
		UE_LOG(LogWildPhase, Warning, TEXT("[보안] Player %d 가 체인을 시도했으나 현재 대응 권한(Prompt)이 없습니다!"), CallerID);
		return;
	}

	if (ActionData.ActionType != EWildActionType::None && ActionData.CardInstanceID != -1)
	{
		if (!PS->IsCardInHand(ActionData.CardInstanceID))
		{
			UE_LOG(LogWildPhase, Warning, TEXT("[보안] Player %d 가 손패에 없는 카드(InstanceID: %d)를 플레이하려 했습니다."), CallerID, ActionData.CardInstanceID);
			return;
		}

		class UCardBase* PlayedCard = PS->FindCardInstanceByID(ActionData.CardInstanceID);
		if (!PlayedCard)
		{
			UE_LOG(LogWildPhase, Warning, TEXT("[보안] Player %d 의 카드 인스턴스를 찾을 수 없습니다."), CallerID);
			return;
		}

		if (UPlayerStatComponent* Stat = PS->GetStatComponent())
		{
			if (Stat->Mana < PlayedCard->CurrentManaCost)
			{
				UE_LOG(LogWildPhase, Warning, TEXT("[보안] Player %d 의 마나가 부족합니다. (필요: %d, 현재: %d)"), CallerID, PlayedCard->CurrentManaCost, Stat->Mana);
				return;
			}
			Stat->ConsumeMana(PlayedCard->CurrentManaCost);
		}
	}

	// 체인 컴포넌트에 액션 추가
	if (UChainComponent* ChainComp = GameState->FindComponentByClass<UChainComponent>())
	{
		// DeclareAction을 호출하면 내부에서 스택 추가 + 상대방에게 우선권/프롬프트 이전이 자동으로 처리됩니다.
		ChainComp->DeclareAction(ActionData);
		
	}
	else
	{
		UE_LOG(LogWildPhase, Error, TEXT("[오류] GameState에서 ChainComponent를 찾을 수 없습니다!"));
		return;
	}

	// 누군가 액션을 했으므로 연속 패스 카운트 초기화
	ConsecutivePassCount = 0;

	GameState->SetPhaseRemainTime(ActionPublicTime);
	GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, this, &UPhaseComponent::OnGlobalTimerTick, 1.0f, true);
}

void UPhaseComponent::ProcessPublicPass(APlayerController* PC)
{
	// player id 변경 필요 ( 고유 식별자로 변경 해야 됨 )
	if (GetOwnerRole() != ROLE_Authority || !PC) return;

	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	AInGamePlayerState* PS = PC->GetPlayerState<AInGamePlayerState>();

	if (!GameState || !PS) return;

	int32 CallerID = PS->GetPlayerId();

	if (GameState->GetCurrentPhase() != EWildCardPhase::Action_Public)
	{
		UE_LOG(LogWildPhase, Warning, TEXT("[보안] Player %d 가 잘못된 페이즈에 패스를 시도했습니다."), CallerID);
		return;
	}

	if (GameState->GetCurrentChainPromptID() != CallerID)
	{
		UE_LOG(LogWildPhase, Warning, TEXT("[보안] Player %d 가 패스를 시도했으나 현재 대응 권한(Prompt)이 없습니다!"), CallerID);
		return;
	}

	// 패스를 선언했으므로 카운트 증가
	ConsecutivePassCount++;
	UE_LOG(LogWildPhase, Log, TEXT("Player %d Pass! 연속 패스 카운트: %d"), CallerID, ConsecutivePassCount);

	if (ConsecutivePassCount >= 2)
	{
		UE_LOG(LogWildPhase, Log, TEXT("[Server] 양측 모두 패스. 쌓여있는 체인 스택(Chain)만 역순 처리를 시작합니다."));
		SetGamePhase(EWildCardPhase::Action_Resolution);
	}
	else
	{
		int32 OpponentID = -1;
		for (APlayerState* State : GameState->PlayerArray)
		{
			if (State && State->GetPlayerId() != CallerID)
			{
				OpponentID = State->GetPlayerId();
				break;
			}
		}

		if (OpponentID != -1)
		{
			// [핵심] 패스(행동 미공개) 시: 선공권(Priority)은 유지하고, 체인 권한(Prompt)만 상대에게 넘김
			GameState->SetCurrentChainPromptID(OpponentID);
			
			UE_LOG(LogWildPhase, Log, TEXT("[Server] 패스 확인: 선공권은 현재 보유자가 유지하며, 체인 대응 여부만 Player %d 에게 묻습니다."), OpponentID);
			
			GameState->SetPhaseRemainTime(ActionPublicTime);
			GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, this, &UPhaseComponent::OnGlobalTimerTick, 1.0f, true);
		}
	}
}

void UPhaseComponent::ProcessPlayerGamesetReady(APlayerController* PC)
{
	if (GetOwnerRole() != ROLE_Authority || !PC) return;

	if (AInGamePlayerState* PS = PC->GetPlayerState<AInGamePlayerState>())
	{
		
		// [수정됨] 레디하지 않은 상태이며, 성유물이 서버 메모리에 성공적으로 할당되었는지 검증
		if (!PS->bIsGamesetReady && PS->EquippedArtifact != nullptr)
		{
			PS->bIsGamesetReady = true;
			GamesetReadyCount++;

			UE_LOG(LogWildPhase, Log, TEXT("[Server] Player %d 성유물 비공개 선택 및 레디 완료! (현재 %d/2)"), PS->GetPlayerID(), GamesetReadyCount);
		}
		else if (PS->EquippedArtifact == nullptr)
		{
			UE_LOG(LogWildPhase, Warning, TEXT("[Server] Player %d 가 성유물을 장착하지 않은 채로 Ready를 시도했습니다."), PS->GetPlayerID());
		}
		
	}
	
	// 양쪽 모두 완료 시 강제로 다음 페이즈(Mulligan)로 전환
	if (GamesetReadyCount >= 2)
	{
		UE_LOG(LogWildPhase, Log, TEXT("[Server] 양측 성유물 선택 완료. Mulligan 페이즈로 진입합니다."));
		GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);
		SetMatchState(EWildMatchState::Mulligan);
	}
}

void UPhaseComponent::ProcessPlayerMulliganReady(APlayerController* PC)
{
	if (GetOwnerRole() != ROLE_Authority || !PC) return;

	if (AInGamePlayerState* PS = PC->GetPlayerState<AInGamePlayerState>())
	{
		if (!PS->bIsMulliganReady)
		{
			PS->bIsMulliganReady = true;
			MulliganReadyCount++;

			UE_LOG(LogTemp, Warning, TEXT("[Server] Player %d 멀리건 완료 처리됨! 현재 레디 카운트: %d / 2"), PS->GetPlayerID(), MulliganReadyCount);
		}
	}
	
	// 양쪽 모두 완료 시 강제로 다음 페이즈(Turn_Start)로
	if (MulliganReadyCount >= 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] 양측 멀리건 모두 완료! Turn_Start 페이즈로 전환합니다."));
		GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);
		SetMatchState(EWildMatchState::Mulligan_End);
	}
}

void UPhaseComponent::ProcessPrivateActionReady(APlayerController* PC, const FWildActionData& ActionData)
{
	if (GetOwnerRole() != ROLE_Authority || !PC) return;

	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (!GameState || GameState->GetCurrentPhase() != EWildCardPhase::Action_Private) return;

	AInGamePlayerState* PS = PC->GetPlayerState<AInGamePlayerState>();
	if (!PS) return;
	
	if (ActionData.ActionCategory == EWildActionCategory::Sub)
	{
		UE_LOG(LogWildPhase, Error, TEXT("[보안] Player %d 가 비공개 결정 단계(Action_Private)에서 서브 행동을 제출했습니다! 무시합니다."), PS->GetPlayerId());
		return;
	}

	FWildActionData ValidatedAction = ActionData;

	// [보안 검증] 유닛 소환/주문 등 카드 기반 메인 액션인 경우 손패 소유 및 마나 검증
	if (ValidatedAction.ActionType != EWildActionType::None && ValidatedAction.CardInstanceID != -1)
	{
		if (!PS->IsCardInHand(ValidatedAction.CardInstanceID))
		{
			UE_LOG(LogWildPhase, Warning, TEXT("[보안] Player %d 가 손패에 없는 카드(InstanceID: %d)로 메인 행동을 시도했습니다. None으로 처리합니다."), PS->GetPlayerId(), ValidatedAction.CardInstanceID);
			ValidatedAction.ActionType = EWildActionType::None;
			ValidatedAction.CardInstanceID = -1;
		}
		else
		{
			class UCardBase* Card = PS->FindCardInstanceByID(ValidatedAction.CardInstanceID);
			UPlayerStatComponent* Stat = PS->GetStatComponent();
			if (!Card || (Stat && Stat->Mana < Card->CurrentManaCost))
			{
				UE_LOG(LogWildPhase, Warning, TEXT("[보안] Player %d 의 마나가 부족하여 메인 행동 카드를 사용할 수 없습니다. None으로 처리합니다."), PS->GetPlayerId());
				ValidatedAction.ActionType = EWildActionType::None;
				ValidatedAction.CardInstanceID = -1;
			}
		}
	}

	if (!PS->bIsActionReady)
	{
		PS->SetPendingMainAction(ValidatedAction);
		ActionReadyCount++;
		UE_LOG(LogWildPhase, Log, TEXT("[Server] Player %d 메인 행동 결정 완료. (Type: %d, 현재 %d/2)"), 
			PS->GetPlayerId(), (int32)ValidatedAction.ActionType, ActionReadyCount);
	}
	
	// 양쪽 모두 메인 행동 결정을 완료한 시점
	if (ActionReadyCount >= 2)
	{
		ActionReadyCount = 0; // 다음 사이클을 위해 초기화
		GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);

		bool bAnyActionTaken = false;
		for (APlayerState* State : GameState->PlayerArray)
		{
			if (AInGamePlayerState* WildPS = Cast<AInGamePlayerState>(State))
			{
				if (WildPS->GetPendingMainAction().ActionType != EWildActionType::None)
				{
					bAnyActionTaken = true;
					break; 
				}
			}
		}

		if (bAnyActionTaken)
		{
			UE_LOG(LogWildPhase, Log, TEXT("[Server] 최소 1명이 행동을 결정했습니다. Action_Public(체인 대기) 페이즈로 전환합니다."));
			SetGamePhase(EWildCardPhase::Action_Public);
		}
		else
		{
			// [규칙 7 적용] 두 플레이어 모두 행동을 건너뜀
			UE_LOG(LogWildPhase, Warning, TEXT("[Server] 규칙 7: 두 플레이어 모두 행동을 건너뛰었습니다. 턴 종료(Turn_End)로 진입합니다."));
			
			for (APlayerState* State : GameState->PlayerArray)
			{
				if (AInGamePlayerState* WildPS = Cast<AInGamePlayerState>(State))
				{
					WildPS->ClearPendingMainAction();
				}
			}
			
			SetGamePhase(EWildCardPhase::Turn_End);
		}
	}
}

void UPhaseComponent::CheckActionPhaseEnd(bool bActionOccurred)
{
	if (GetOwnerRole() != ROLE_Authority) return;

	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (!GameState) return;
	
	// [규칙 6 적용] 모든 처리가 끝났으므로 다시 비공개 행동 결정 단계로 돌아감
	UE_LOG(LogWildPhase, Log, TEXT("[Phase] 규칙 6: 메인 행동 및 미들 페이즈 종료. 액션 페이즈(Action_Private)를 재시작합니다."));
	
	// 다음 사이클을 위해 모든 레디 상태와 캐싱 데이터를 초기화
	for (APlayerState* State : GameState->PlayerArray)
	{
		if (AInGamePlayerState* WildPS = Cast<AInGamePlayerState>(State))
		{
			WildPS->ClearPendingMainAction(); 
		}
	}

	ActionReadyCount = 0;
	ConsecutivePassCount = 0;

	// 효과 처리를 시각적으로 확인할 수 있도록 2초의 유예 시간을 둔 뒤 재시작
	GetWorld()->GetTimerManager().SetTimer(
		PhaseTimerHandle, 
		FTimerDelegate::CreateUObject(this, &UPhaseComponent::SetGamePhase, EWildCardPhase::Action_Private), 
		2.0f, 
		false
	);
}

void UPhaseComponent::HandleTurnStart()
{
	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		
		if (AInGamePlayerState* PS = It->Get()->GetPlayerState<AInGamePlayerState>())
		{
			UPlayerStatComponent* PlayerStat = PS->GetStatComponent();
			
			if (PlayerStat)
			{
				PlayerStat->RefreshForce();
			}

			PS->DrawCard(TEXT("TestCard_Draw"), 1);

			UE_LOG(LogWildPhase, Log, TEXT("[Server] Player %d 가 턴 시작 드로우를 수행했습니다."), PS->GetPlayerId());
			// PS->DrawCard(); // 구현된 드로우 로직 호출
		}
		
	}

	// 턴 시작 효과 처리 후 액션 페이즈로 자동 전환
	SetGamePhase(EWildCardPhase::Action_Private);
}

void UPhaseComponent::HandleTurnEnd()
{
	if (GetOwnerRole() != ROLE_Authority) return;

	UE_LOG(LogWildPhase, Log, TEXT("[Server] 턴 종료 페이즈 진입. 양측 플레이어의 패 제한을 검사합니다."));
	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AInGamePlayerState* PS = It->Get()->GetPlayerState<AInGamePlayerState>())
		{
			while ((PS->CardComponent)->CurrentHandDisplay.Num() > 10)
			{
				// PS->ExileCard(PS->CurrentHandDisplay.Last()); // 패 추방 로직
			}
		}
	}
	

	// 다시 턴 시작으로
	SetGamePhase(EWildCardPhase::Turn_Start);
}

void UPhaseComponent::OnGlobalTimerTick()
{
	if (AInGameState* GameState = GetWorld()->GetGameState<AInGameState>())
	{
		int32 CurrentTime = GameState->GetPhaseRemainTime();
		if (CurrentTime > 0)
		{
			GameState->SetPhaseRemainTime(CurrentTime - 1);
		}
		else
		{
			// 시간 다 되면 타이머 멈추고 라우팅
			GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);
			
			// 매치 상태가 MainGame(본 게임)이 아니면 매치 타임아웃, 본 게임 중이면 페이즈 타임아웃
			if (GameState->GetCurrentMatchState() != EWildMatchState::MainGame)
			{
				OnMatchTimerExpired();
			}
			else
			{
				OnPhaseTimerExpired();
			}
		}
	}
}

void UPhaseComponent:: OnMatchTimerExpired()
{
	if (GetOwnerRole() != ROLE_Authority) return;

	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (!GameState) return;

	EWildMatchState CurrentState = GameState->GetCurrentMatchState();

	// Gameset 또는 Mulligan: 시간 초과 시 레디하지 않은 유저 즉시 패배 (접속 종료 간주)
	if (CurrentState == EWildMatchState::Gameset || CurrentState == EWildMatchState::Mulligan)
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			AInGamePlayerState* PS = PC ? PC->GetPlayerState<AInGamePlayerState>() : nullptr;

			if (PS)
			{
				bool bIsReady = (CurrentState == EWildMatchState::Gameset) ? PS->bIsGamesetReady : PS->bIsMulliganReady;
				
				if (!bIsReady)
				{
					// 룰북: 시간 제한 초과 플레이어는 접속 종료 간주, 패배 처리
					UE_LOG(LogWildPhase, Error, TEXT("[System] Player %d timed out during %s. Defeat!"), PS->GetPlayerID(), *UEnum::GetValueAsString(CurrentState));
					
					// 체력을 0으로 만들어 패배 처리
					if (PS->GetStatComponent())
					{
						//PS->GetStatComponent()->TakeDamage(PS->GetStatComponent()->Health);
					}
				}
			}
		}
	}
}

void UPhaseComponent::OnPhaseTimerExpired()
{
	if (GetOwnerRole() != ROLE_Authority) return;

	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (!GameState) return;

	EWildCardPhase CurrentPhase = GameState->GetCurrentPhase();

	// Action_Private: 비공개 행동 결정 시간 초과 시
	if (CurrentPhase == EWildCardPhase::Action_Private)
	{
		UE_LOG(LogTemp, Warning, TEXT("[System] 비공개 행동 결정 시간 초과! 강제로 Action_Public으로 전환합니다."));
		
		ActionReadyCount = 0;
		
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (AInGamePlayerState* PS = It->Get()->GetPlayerState<AInGamePlayerState>())
			{
				PS->bIsActionReady = false; 
				// 행동을 안 정했다면 None 메인 행동 강제 주입
				if (PS->GetPendingMainAction().ActionType == EWildActionType::None)
				{
					FWildActionData TimeoutAction;
					TimeoutAction.PlayerID = PS->GetPlayerId();
					TimeoutAction.ActionCategory = EWildActionCategory::Main;
					TimeoutAction.ActionType = EWildActionType::None;
					PS->SetPendingMainAction(TimeoutAction);
				}
			}
		}

		bool bAnyMainActionTaken = false;
		for (APlayerState* State : GameState->PlayerArray)
		{
			if (AInGamePlayerState* WildPS = Cast<AInGamePlayerState>(State))
			{
				const FWildActionData& PendingAction = WildPS->GetPendingMainAction();
				if (PendingAction.ActionCategory == EWildActionCategory::Main && PendingAction.ActionType != EWildActionType::None)
				{
					bAnyMainActionTaken = true;
					break;
				}
			}
		}
		
		if (bAnyMainActionTaken)
		{
			UE_LOG(LogWildPhase, Log, TEXT("[System] 타임아웃 결과, 메인 행동이 존재하므로 Action_Public으로 강제 전환합니다."));
			SetGamePhase(EWildCardPhase::Action_Public);
		}
		else
		{
			UE_LOG(LogWildPhase, Warning, TEXT("[System] 타임아웃 결과, 양측 모두 메인 행동이 없습니다. 즉시 턴 종료(Turn_End)로 강제 직행합니다."));
			SetGamePhase(EWildCardPhase::Turn_End);
		}
	}
	
	// Action_Public: 체인 대기 시간 초과 시 
	else if (CurrentPhase == EWildCardPhase::Action_Public)
	{
		int32 TimeoutPlayerID = GameState->GetCurrentChainPromptID();
		UE_LOG(LogWildPhase, Warning, TEXT("[System] 체인 대기 시간 초과! Player %d 강제 패스 처리 시도"), TimeoutPlayerID);

		bool bPassExecuted = false;
		
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* TargetPC = It->Get();
			if (AInGamePlayerState* PS = TargetPC ? TargetPC->GetPlayerState<AInGamePlayerState>() : nullptr)
			{
				if (PS->GetPlayerId() == TimeoutPlayerID)
				{
					ProcessPublicPass(TargetPC);
					bPassExecuted = true;
					break;
				}
			}
		}

		if (!bPassExecuted)
		{
			UE_LOG(LogWildPhase, Error, TEXT("[System] 타임아웃 플레이어를 찾을 수 없습니다. 강제로 페이즈를 전환합니다."));
			SetGamePhase(EWildCardPhase::Action_Resolution);
		}
	}
	else if (CurrentPhase == EWildCardPhase::Action_Resolution)
	{
		UE_LOG(LogWildPhase, Log, TEXT("[System] 새 사이클 준비: 모든 캐싱 데이터와 레디 상태를 초기화합니다."));

		for (APlayerState* State : GameState->PlayerArray)
		{
			if (AInGamePlayerState* WildPS = Cast<AInGamePlayerState>(State))
			{
				WildPS->ClearPendingMainAction();
			}
		}

		ActionReadyCount = 0; 
		ConsecutivePassCount = 0;

		SetGamePhase(EWildCardPhase::Action_Private);
	}
}