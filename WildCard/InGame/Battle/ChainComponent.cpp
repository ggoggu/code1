// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Battle/ChainComponent.h"
#include "Net/UnrealNetwork.h"
#include "UnitBase.h"
#include "InGameState.h"
#include "PhaseComponent.h"
#include "InGamePlayerState.h"
#include "InGameMode.h"
#include "WildCard.h"
#include "CardBase.h"


// Sets default values for this component's properties
UChainComponent::UChainComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	ActionCount = 0;
}


// Called when the game starts
void UChainComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority()) // 서버에서만 클라에서만 실행 시켜야하는 코드를 구분해서 델리게이트에 넘겨 주면 된다 ?
	{
		if (AInGameState* GameState = Cast<AInGameState>(GetOwner()))
		{
			GameState->OnPhaseChangedDelegate.AddDynamic(this, &UChainComponent::OnServerPhaseChanged);
		}
	}
	
}


void UChainComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UChainComponent, ActionStack);
	DOREPLIFETIME(UChainComponent, ActionCount);
}

void UChainComponent::AddActionToChain(const FWildActionData& NewAction)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		ActionStack.Add(NewAction);
		ActionCount = ActionStack.Num();
		OnChainUpdatedDelegate.Broadcast(ActionCount);
	}
}

void UChainComponent::ResolveChain()
{
	if (GetOwnerRole() != ROLE_Authority) return;

	UE_LOG(LogWildChain, Warning, TEXT("[Server] === 체인 스택 역순(LIFO) 처리 시작 ==="));

	// 1. 체인 스택 역순(LIFO) 처리 (메인 행동은 여기 없으므로 체인들만 처리됨)
	for (int32 i = ActionStack.Num() - 1; i >= 0; --i)
	{
		ExecuteAction(ActionStack[i]);
	}

	ActionStack.Empty();
	ActionCount = 0;
	OnChainUpdatedDelegate.Broadcast(ActionCount); 

	UE_LOG(LogWildChain, Warning, TEXT("[Server] === 체인 스택 처리 완료. 미들 페이즈 확인 진입 ==="));

	// 2. 체인 처리가 끝났으므로 유발 효과(미들 페이즈)가 있는지 확인
	ProcessMiddlePhase();
}

void UChainComponent::DeclareAction(const FWildActionData& ActionData)
{
	if (GetOwnerRole() != ROLE_Authority) return;
	if (ActionData.ActionType == EWildActionType::None) return;

	// 1. 체인 스택에 행동 추가 (단, 메인 행동은 밖에서 별도로 보관해야 함)
	AddActionToChain(ActionData);

	// 2. 게임 상태 및 상대방 ID 찾기
	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (!GameState) return;

	int32 ActivatorID = ActionData.PlayerID;
	int32 OpponentID = -1;

	for (APlayerState* State : GameState->PlayerArray)
	{
		if (State && State->GetPlayerId() != ActivatorID)
		{
			OpponentID = State->GetPlayerId();
			break;
		}
	}
	
	// [수정된 핵심] 행동이 공개(시전)되었으므로, 기존 보유 여부와 무조건 상관없이 상대에게 선공권과 체인 권한을 모두 넘김
	GameState->SetCurrentPriorityPlayerID(OpponentID);
	GameState->SetCurrentChainPromptID(OpponentID);

	UE_LOG(LogWildChain, Log, TEXT("[Server] Player %d 행동 공개! 선공권과 체인 권한이 무조건 Player %d 에게 넘어갑니다."), ActivatorID, OpponentID);
}

void UChainComponent::ProcessNextMainAction()
{
	if (MainActionQueue.Num() > 0)
	{
		FWildActionData NextMainAction = MainActionQueue[0];
		MainActionQueue.RemoveAt(0);

		UE_LOG(LogWildChain, Log, TEXT("[Server] Player %d 의 메인 행동 시전(공개)!"), NextMainAction.PlayerID);

		// [추가] 메인 행동은 스택에 넣지 않고, 마지막 동시 처리를 위해 따로 보관합니다.
		PendingResolutionMainActions.Add(NextMainAction);

		AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
		if (GameState)
		{
			int32 ActivatorID = NextMainAction.PlayerID;
			int32 OpponentID = -1;
			for (APlayerState* State : GameState->PlayerArray)
			{
				if (State && State->GetPlayerId() != ActivatorID)
				{
					OpponentID = State->GetPlayerId();
					break;
				}
			}

			// 선공권과 체인 권한 이전 (규칙 0)
			GameState->SetCurrentPriorityPlayerID(OpponentID);
			GameState->SetCurrentChainPromptID(OpponentID);
		}
	}
	else
	{
		// 모든 메인 행동의 공방과 미들 페이즈가 끝났다면 최종 동시 처리로 이동
		ExecuteMainActionsSimultaneously();
	}
}

void UChainComponent::ProcessMiddlePhase()
{
	bool bHasTriggerEffects = false; 

	if (bHasTriggerEffects)
	{
		UE_LOG(LogWildChain, Log, TEXT("[Server] 유발 효과 발생! 미들 페이즈 체인 공방을 시작합니다."));
	}
	else
	{
		UE_LOG(LogWildChain, Log, TEXT("[Server] 발동할 유발 효과가 없습니다. 다음 메인 행동 검사로 넘어갑니다."));

		if (MainActionQueue.Num() > 0)
		{
			// [수정] 직접 호출하지 않고 페이즈만 넘깁니다. 델리게이트가 처리해 줄 것입니다.
			if (AInGameMode* GameMode = Cast<AInGameMode>(GetWorld()->GetAuthGameMode()))
			{
				if (GameMode->PhaseComponent)
				{
					GameMode->PhaseComponent->SetGamePhase(EWildCardPhase::Action_Public);
				}
			}
		}
		else
		{
			ExecuteMainActionsSimultaneously();
		}
	}
}

void UChainComponent::ExecuteMainActionsSimultaneously()
{
	UE_LOG(LogWildChain, Warning, TEXT("[Server] ★★★ 모든 체인/미들 페이즈 종료. 보관된 메인 행동들을 동시에 처리합니다! ★★★"));

	// 보관해둔 메인 행동 처리 (기존 기획의 '동시 적용' 구현)
	for(const FWildActionData& Action : PendingResolutionMainActions)
	{
		ExecuteAction(Action);
	}

	bool bActionOccurred = (PendingResolutionMainActions.Num() > 0);
	
	// 처리 완료 후 보관함 비우기
	PendingResolutionMainActions.Empty();

	// 모든 액션 처리가 완전히 끝났으므로 액션 페이즈 재시작 또는 턴 종료 체크
	if (AInGameMode* GameMode = Cast<AInGameMode>(GetWorld()->GetAuthGameMode()))
	{
		if (GameMode->PhaseComponent)
		{
			GameMode->PhaseComponent->CheckActionPhaseEnd(bActionOccurred);
		}
	}
}

void UChainComponent::OnRep_ActionStack()
{
	OnChainUpdatedDelegate.Broadcast(ActionStack.Num());
}

void UChainComponent::OnServerPhaseChanged(EWildCardPhase NewPhase)
{
	if (GetOwnerRole() != ROLE_Authority) return;

	AInGameState* GameState = Cast<AInGameState>(GetOwner());
	if (!GameState) return;

	switch (NewPhase)
	{
	case EWildCardPhase::Action_Public:
		{
			UE_LOG(LogWildChain, Log, TEXT("[Server] Action_Public 진입: 메인 행동 순차 처리를 준비합니다."));

			// [수정] 대기열(Queue)과 보관소(Pending)가 '모두' 비어있을 때만 새로 수집합니다.
			if (MainActionQueue.Num() == 0 && PendingResolutionMainActions.Num() == 0)
			{
				int32 FirstPlayerID = GameState->GetCurrentPriorityPlayerID();
				// ... (기존과 동일하게 FirstPlayerID, SecondPlayerID 찾기) ...
				int32 SecondPlayerID = -1;
				for (APlayerState* State : GameState->PlayerArray)
				{
					if (State && State->GetPlayerId() != FirstPlayerID)
					{
						SecondPlayerID = State->GetPlayerId();
						break;
					}
				}

				AInGamePlayerState* FirstPS = nullptr;
				AInGamePlayerState* SecondPS = nullptr;
				for (APlayerState* State : GameState->PlayerArray)
				{
					AInGamePlayerState* WildPS = Cast<AInGamePlayerState>(State);
					if (WildPS)
					{
						if (WildPS->GetPlayerId() == FirstPlayerID) FirstPS = WildPS;
						else if (WildPS->GetPlayerId() == SecondPlayerID) SecondPS = WildPS;
					}
				}

				if (FirstPS && SecondPS)
				{
					const FWildActionData& FirstAction = FirstPS->GetPendingMainAction();
					const FWildActionData& SecondAction = SecondPS->GetPendingMainAction();

					if (FirstAction.ActionType != EWildActionType::None) MainActionQueue.Add(FirstAction);
					if (SecondAction.ActionType != EWildActionType::None) MainActionQueue.Add(SecondAction);
				}
			}

			// 델리게이트가 알아서 다음 행동을 꺼내도록 위임합니다.
			ProcessNextMainAction();
			break;
		}
	default:
		break;
	}
	
}

void UChainComponent::ExecuteAction(const FWildActionData& ActionData)
{
	AInGamePlayerState* ActivatorPS = nullptr;
	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	
	if (GameState)
	{
		for (APlayerState* State : GameState->PlayerArray)
		{
			AInGamePlayerState* PS = Cast<AInGamePlayerState>(State);
			if (PS && PS->GetPlayerId() == ActionData.PlayerID)
			{
				ActivatorPS = PS;
				break;
			}
		}
	}

	if (!ActivatorPS)
	{
		UE_LOG(LogWildChain, Error, TEXT("[Server] 시전자(PlayerID: %d)의 PlayerState를 찾을 수 없어 행동을 취소합니다."), ActionData.PlayerID);
		return;
	}

	if (ActionData.ActionType == EWildActionType::SummonUnit)
	{
		ACardBase* SummonCard = ActivatorPS->FindCardInstanceByID(ActionData.CardInstanceID);
		if (SummonCard)
		{
			// Ensure TargetGridPos is somewhat validated (e.g. not off-grid)
			UE_LOG(LogWildChain, Log, TEXT("[Server] %s 유닛을 %s 좌표에 소환합니다."), *SummonCard->CardID.ToString(), *ActionData.TargetGridPos.ToString());
		}
	}
	else if (ActionData.ActionType == EWildActionType::UnitMove)
	{
		AUnitBase* SourceUnit = ActionData.SourceUnit;
		AUnitBase* TargetUnit = ActionData.TargetUnit;

		if (IsValid(SourceUnit))
		{
			if (IsValid(TargetUnit))
			{
				UE_LOG(LogWildChain, Warning, TEXT("[Combat] 전투 발생!"));
			}
			else 
			{
				UE_LOG(LogWildChain, Log, TEXT("[Server] %s 유닛 이동 (목표 좌표: %s)"), *SourceUnit->GetName(), *ActionData.TargetGridPos.ToString());
			}
		}
	}
}