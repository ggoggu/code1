// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Battle/ChainComponent.h"
#include "Net/UnrealNetwork.h"
#include "UnitBase.h"
#include "InGameState.h"
#include "PhaseComponent.h"
#include "InGamePlayerState.h"
#include "InGameMode.h"
#include "InGame/Unit/PlayerUnit.h"
#include "InGame/Unit/TacticalSkillBase.h"
#include "InGame/Game/InGameController.h"
#include "InGame/Game/PlayerCardComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "WildCard.h"
#include "Tile.h"
#include "CardBase.h"
#include "BattleData.h"
#include "PlayerStatComponent.h"
#include "WildCardGameInstance.h"
#include "Engine/DataTable.h"
#include "EffectBase.h"
#include "InGame/Battle/WildCombatLogComponent.h"
#include "InGame/Map/WildActionPreviewArrow.h"
#include "InGame/Map/WildPreviewGhostUnit.h"


// Sets default values for this component's properties
UChainComponent::UChainComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	ActionCount = 0;
	UnitClass = AUnitBase::StaticClass();
	PlayerUnitClass = APlayerUnit::StaticClass();
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

void UChainComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ChainTimerHandle);
		World->GetTimerManager().ClearTimer(SearchLibraryTimeoutTimerHandle);
	}
	ClearPublicPreviews();

	for (AWildActionPreviewArrow* Arrow : PooledPreviewArrows)
	{
		if (IsValid(Arrow))
		{
			Arrow->Destroy();
		}
	}
	PooledPreviewArrows.Empty();

	for (AWildPreviewGhostUnit* Ghost : PooledGhostUnits)
	{
		if (IsValid(Ghost))
		{
			Ghost->Destroy();
		}
	}
	PooledGhostUnits.Empty();

	Super::EndPlay(EndPlayReason);
}


void UChainComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UChainComponent, ActionStack);
	DOREPLIFETIME(UChainComponent, ActionCount);
	DOREPLIFETIME(UChainComponent, PublicMainActions);
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

	ResolveNextChainAction();
}

void UChainComponent::ResolveNextChainAction()
{
	if (GetOwnerRole() != ROLE_Authority) return;

	if (bAwaitingSearchLibrarySelection)
	{
		return;
	}

	while (ActionStack.Num() > 0)
	{
		FWildActionData CurrentAction = ActionStack.Pop();
		ActionCount = ActionStack.Num();
		OnChainUpdatedDelegate.Broadcast(ActionCount);

		if (IsPromptingSkillAction(CurrentAction))
		{
			ResolvePromptingSkillAction(CurrentAction);
			return;
		}
		else
		{
			ExecuteAction(CurrentAction);
		}
	}

	ActionCount = 0;
	OnChainUpdatedDelegate.Broadcast(ActionCount); 

	UE_LOG(LogWildChain, Warning, TEXT("[Server] === 체인 스택 처리 완료. 미들 페이즈 확인 진입 ==="));

	// 2. 체인 처리가 끝났으므로 유발 효과(미들 페이즈)가 있는지 확인
	ProcessMiddlePhase();
}

bool UChainComponent::IsPromptingSkillAction(const FWildActionData& ActionData) const
{
	if (ActionData.ActionType != EWildActionType::CastTacticalSkill)
	{
		return false;
	}

	AInGameState* GS = GetWorld() ? GetWorld()->GetGameState<AInGameState>() : nullptr;
	if (!GS) return false;

	for (APlayerState* State : GS->PlayerArray)
	{
		if (AInGamePlayerState* PS = Cast<AInGamePlayerState>(State))
		{
			if (PS->GetPlayerId() == ActionData.PlayerID)
			{
				if (APlayerUnit* PlayerUnit = PS->GetPlayerUnit())
				{
					if (UTacticalSkillBase* Skill = PlayerUnit->GetTacticalSkill())
					{
						return Skill->RequiresPlayerPromptOnResolve(ActionData);
					}
				}
			}
		}
	}

	return false;
}

void UChainComponent::ResolvePromptingSkillAction(const FWildActionData& ActionData)
{
	AInGameState* GS = GetWorld() ? GetWorld()->GetGameState<AInGameState>() : nullptr;
	if (!GS)
	{
		ResolveNextChainAction();
		return;
	}

	AInGamePlayerState* ActivatorPS = nullptr;
	for (APlayerState* State : GS->PlayerArray)
	{
		if (AInGamePlayerState* PS = Cast<AInGamePlayerState>(State))
		{
			if (PS->GetPlayerId() == ActionData.PlayerID)
			{
				ActivatorPS = PS;
				break;
			}
		}
	}

	if (!ActivatorPS)
	{
		UE_LOG(LogWildChain, Error, TEXT("[Chain] 전술 스킬 시전자 PlayerState를 찾을 수 없습니다."));
		ResolveNextChainAction();
		return;
	}

	APlayerUnit* PlayerUnit = ActivatorPS->GetPlayerUnit();
	UTacticalSkillBase* Skill = PlayerUnit ? PlayerUnit->GetTacticalSkill() : nullptr;
	if (!Skill)
	{
		ResolveNextChainAction();
		return;
	}

	UPlayerCardComponent* CardComp = ActivatorPS->GetCardComponent();
	if (!CardComp || CardComp->ServerHand.Num() == 0)
	{
		UE_LOG(LogWildChain, Warning, TEXT("[Chain] Player %d 전술 스킬: 패에 카드가 없어 효과가 불발됩니다."), ActionData.PlayerID);
		if (UWildCombatLogComponent* LogComp = GS->GetCombatLogComponent())
		{
			LogComp->LogActionResolve(LogComp->GetLastPublicPhaseLogIndex(), EWildActionType::CastTacticalSkill, -1, -1, TEXT("{전술 스킬} 불발 (패 없음)"));
		}
		ResolveNextChainAction();
		return;
	}

	AInGameController* PC = Cast<AInGameController>(ActivatorPS->GetPlayerController());
	if (!PC)
	{
		// PC가 없을 경우 (AI 등) 기본 첫 번째 카드로 즉시 교환
		ExecuteSearchLibrarySwap(ActivatorPS, CardComp->ServerHand[0]->InstanceID);
		ResolveNextChainAction();
		return;
	}

	// 비동기 카드 선택 대기 상태 설정
	bAwaitingSearchLibrarySelection = true;
	CurrentSearchLibraryAction = ActionData;

	// 10초 타임아웃 타이머 설정
	GetWorld()->GetTimerManager().SetTimer(
		SearchLibraryTimeoutTimerHandle,
		this,
		&UChainComponent::OnSearchLibraryTimeout,
		10.0f,
		false
	);

	// 스킬 다형성 프롬프트 위임 호출!
	Skill->PromptPlayerOnResolve(PC, ActionData);
}

void UChainComponent::ExecuteSearchLibrarySwap(AInGamePlayerState* PS, int32 SelectedCardInstanceID)
{
	if (!PS) return;

	APlayerUnit* CasterUnit = PS->GetPlayerUnit();
	if (!CasterUnit) return;

	FWildActionData ResolvedAction = CurrentSearchLibraryAction;
	ResolvedAction.CardInstanceID = SelectedCardInstanceID;

	ExecuteAction(ResolvedAction);
}

void UChainComponent::OnSearchLibrarySelectionConfirmed(int32 PlayerID, int32 SelectedCardInstanceID)
{
	if (GetOwnerRole() != ROLE_Authority) return;
	if (!bAwaitingSearchLibrarySelection || CurrentSearchLibraryAction.PlayerID != PlayerID)
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(SearchLibraryTimeoutTimerHandle);
	bAwaitingSearchLibrarySelection = false;

	AInGameState* GS = GetWorld() ? GetWorld()->GetGameState<AInGameState>() : nullptr;
	if (!GS) return;

	AInGamePlayerState* PS = nullptr;
	for (APlayerState* State : GS->PlayerArray)
	{
		if (AInGamePlayerState* WildPS = Cast<AInGamePlayerState>(State))
		{
			if (WildPS->GetPlayerId() == PlayerID)
			{
				PS = WildPS;
				break;
			}
		}
	}

	if (PS)
	{
		ExecuteSearchLibrarySwap(PS, SelectedCardInstanceID);
	}

	ResolveNextChainAction();
}

void UChainComponent::OnSearchLibraryTimeout()
{
	if (GetOwnerRole() != ROLE_Authority) return;
	if (!bAwaitingSearchLibrarySelection) return;

	UE_LOG(LogWildChain, Warning, TEXT("[Chain] Player %d 서고 탐색 선택 시간 초과. 첫 번째 카드로 자동 처리합니다."), CurrentSearchLibraryAction.PlayerID);

	AInGameState* GS = GetWorld() ? GetWorld()->GetGameState<AInGameState>() : nullptr;
	if (GS)
	{
		for (APlayerState* State : GS->PlayerArray)
		{
			if (AInGamePlayerState* WildPS = Cast<AInGamePlayerState>(State))
			{
				if (WildPS->GetPlayerId() == CurrentSearchLibraryAction.PlayerID)
				{
					if (AInGameController* PC = Cast<AInGameController>(WildPS->GetPlayerController()))
					{
						PC->Client_CancelCardSelection();
					}
					break;
				}
			}
		}
	}

	OnSearchLibrarySelectionConfirmed(CurrentSearchLibraryAction.PlayerID, -1);
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

void UChainComponent::StartChainWindowForCurrentAction(int32 PromptPlayerID)
{
	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (!GameState) return;

	// 선공권과 체인 권한 이전
	GameState->SetCurrentPriorityPlayerID(PromptPlayerID);
	GameState->SetCurrentChainPromptID(PromptPlayerID);

	// 2단계 & 6단계: 체인 시작 위젯 알림 브로드캐스트
	GameState->Multicast_NotifyTiming(EWildTimingNoticeType::ChainStart, FText::FromString(TEXT("체인 시작")), 1.5f);

	// 체인 대기 카운트다운 타이머 시작 (15초)
	if (AInGameMode* GameMode = Cast<AInGameMode>(GetWorld()->GetAuthGameMode()))
	{
		if (GameMode->PhaseComponent)
		{
			GameMode->PhaseComponent->ResetCycleCounters();
			GameMode->PhaseComponent->StartPhaseCountdown(15);
		}
	}
}

void UChainComponent::InitiateMainActionResolution()
{
	UE_LOG(LogWildChain, Warning, TEXT("[Server] ★★★ 모든 체인 공방 종료. 메인 행동 처리(판정) 단계로 진입합니다! ★★★"));

	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (!GameState) return;

	// 8단계: 메인 행동 처리 위젯 알림 브로드캐스트
	GameState->Multicast_NotifyTiming(EWildTimingNoticeType::MainActionResolution, FText::FromString(TEXT("메인 행동 처리")), 2.0f);

	// 메인 행동 처리 위젯 연출 시간(2.0초) 후 동시 처리 실행
	GetWorld()->GetTimerManager().SetTimer(
		ChainTimerHandle,
		FTimerDelegate::CreateUObject(this, &UChainComponent::ExecuteMainActionsSimultaneously),
		2.0f,
		false
	);
}

void UChainComponent::CheckAndResolveMainActionConflicts(FWildActionData& FirstAction, FWildActionData& SecondAction, AInGamePlayerState* FirstPS, AInGamePlayerState* SecondPS)
{
	if (!FirstPS || !SecondPS) return;
	if (FirstAction.ActionType == EWildActionType::None || SecondAction.ActionType == EWildActionType::None) return;

	bool bConflict = false;

	// 동일 타일 점유 충돌 검사 (소환-소환, 이동-이동, 소환-이동 등)
	if (FirstAction.TargetTile != nullptr && FirstAction.TargetTile == SecondAction.TargetTile)
	{
		bool bFirstIsTilePlacement = (FirstAction.ActionType == EWildActionType::SummonUnit || FirstAction.ActionType == EWildActionType::UnitMove);
		bool bSecondIsTilePlacement = (SecondAction.ActionType == EWildActionType::SummonUnit || SecondAction.ActionType == EWildActionType::UnitMove);

		if (bFirstIsTilePlacement && bSecondIsTilePlacement)
		{
			bConflict = true;
		}
	}

	if (bConflict)
	{
		UE_LOG(LogWildChain, Warning, TEXT("[Server] 규칙 1 경합 감지: 선공(Player %d)과 후공(Player %d)의 행동이 동일 타일을 대상으로 충돌합니다!"),
			FirstPS->GetPlayerId(), SecondPS->GetPlayerId());

		// 선공권이 없는 플레이어(후공)의 행동을 [시전 취소] 처리
		// 시전 취소된 플레이어는 시전에 소모된 타이머 시간을 제외한 모든 재화(포스)를 돌려받음
		int32 RefundForce = 0;
		if (SecondAction.ActionType == EWildActionType::SummonUnit || SecondAction.ActionType == EWildActionType::CastSpell)
		{
			if (UCardBase* Card = SecondPS->FindCardInstanceByID(SecondAction.CardInstanceID))
			{
				RefundForce = Card->CurrentForce > 0 ? Card->CurrentForce : (Card->CurrentManaCost > 0 ? Card->CurrentManaCost : 1);
			}
			else
			{
				RefundForce = 1;
			}
		}
		else if (SecondAction.ActionType == EWildActionType::UnitMove || SecondAction.ActionType == EWildActionType::UnitAttack)
		{
			RefundForce = 1;
		}

		if (RefundForce > 0 && SecondPS->GetStatComponent())
		{
			SecondPS->GetStatComponent()->AddForce(RefundForce);
			UE_LOG(LogWildChain, Log, TEXT("[Server] 후공 Player %d 시전 취소로 포스 %d 반환 완료. (현재 포스: %d)"),
				SecondPS->GetPlayerId(), RefundForce, SecondPS->GetStatComponent()->CurrentForce);
		}

		// 행동 초기화
		SecondAction.ActionType = EWildActionType::None;
		SecondAction.CardInstanceID = -1;
		SecondAction.TargetTile = nullptr;
		SecondAction.SourceTile = nullptr;
		SecondPS->SetPendingMainAction(SecondAction);

		if (AInGameState* GameState = Cast<AInGameState>(GetOwner()))
		{
			if (UWildCombatLogComponent* LogComp = GameState->GetCombatLogComponent())
			{
				LogComp->LogActionFailed(LogComp->GetLastPublicPhaseLogIndex(),
					FString::Printf(TEXT("[시전 취소] 타일 경합으로 인해 후공(Player %d)의 메인 행동이 시전 취소되었습니다. (포스 %d 환불)"), SecondPS->GetPlayerId(), RefundForce));
			}

			// 시전 취소 안내 알림 토스트
			GameState->Multicast_NotifyTiming(EWildTimingNoticeType::Custom,
				FText::FromString(TEXT("[시전 취소] 후공 플레이어 행동 시전 취소 (포스 환불)")), 2.0f);
		}
	}
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

			// 플레이어 표시명 및 카드/액션 정보 추출
			FString PlayerDisplayName = FString::Printf(TEXT("Player %d"), ActivatorID);
			FString ActionDetailName = TEXT("");

			for (APlayerState* State : GameState->PlayerArray)
			{
				if (State && State->GetPlayerId() == ActivatorID)
				{
					if (!State->GetPlayerName().IsEmpty())
					{
						PlayerDisplayName = State->GetPlayerName();
					}
					if (AInGamePlayerState* WildPS = Cast<AInGamePlayerState>(State))
					{
						if (NextMainAction.CardInstanceID != -1)
						{
							if (UCardBase* Card = WildPS->FindCardInstanceByID(NextMainAction.CardInstanceID))
							{
								ActionDetailName = UWildCombatLogComponent::GetCardDisplayName(GetWorld(), Card->CardID);
							}
						}
					}
					break;
				}
			}

			bool bIsFirstAction = (PendingResolutionMainActions.Num() == 1);
			FString TurnOrderStr = bIsFirstAction ? TEXT("선공") : TEXT("후공");
			FString NoticeStr;
			if (!ActionDetailName.IsEmpty())
			{
				NoticeStr = FString::Printf(TEXT("%s [%s] 행동 시전: %s"), *TurnOrderStr, *PlayerDisplayName, *ActionDetailName);
			}
			else
			{
				NoticeStr = FString::Printf(TEXT("%s [%s] 메인 행동 시전!"), *TurnOrderStr, *PlayerDisplayName);
			}

			// 1단계 & 5단계: 메인 행동 공개 위젯 알림 브로드캐스트 (기획서 2단계 ① 및 ⑤)
			GameState->Multicast_NotifyTiming(EWildTimingNoticeType::MainActionReveal, FText::FromString(NoticeStr), 1.8f);

			// 메인 행동 공개 위젯 연출 시간(1.8초) 후 체인 시작 위젯 표시 및 타이머 가동
			GetWorld()->GetTimerManager().SetTimer(
				ChainTimerHandle,
				FTimerDelegate::CreateUObject(this, &UChainComponent::StartChainWindowForCurrentAction, OpponentID),
				1.8f,
				false
			);
		}
	}
	else
	{
		// 모든 메인 행동의 공방과 미들 페이즈가 끝났다면 최종 메인 행동 처리(판정) 단계로 이동
		InitiateMainActionResolution();
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
			InitiateMainActionResolution();
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
	ClearPublicMainActions();

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

void UChainComponent::OnRep_PublicMainActions()
{
	UpdatePublicPreviews();
}

void UChainComponent::ClearPublicMainActions()
{
	PublicMainActions.Empty();
	ClearPublicPreviews();
}

void UChainComponent::ClearPublicPreviews()
{
	for (AWildActionPreviewArrow* Arrow : PooledPreviewArrows)
	{
		if (IsValid(Arrow))
		{
			Arrow->ClearArrow();
			Arrow->SetActorHiddenInGame(true);
		}
	}

	for (AWildPreviewGhostUnit* Ghost : PooledGhostUnits)
	{
		if (IsValid(Ghost))
		{
			Ghost->SetActorHiddenInGame(true);
			Ghost->SetActorTickEnabled(false);
		}
	}

	for (AActor* Actor : ActivePreviewActors)
	{
		if (IsValid(Actor) && !PooledPreviewArrows.Contains(Actor) && !PooledGhostUnits.Contains(Actor))
		{
			Actor->Destroy();
		}
	}
	ActivePreviewActors.Empty();
}

AWildActionPreviewArrow* UChainComponent::GetOrCreatePooledArrow()
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	for (AWildActionPreviewArrow* Arrow : PooledPreviewArrows)
	{
		if (IsValid(Arrow) && Arrow->IsHidden())
		{
			Arrow->SetActorHiddenInGame(false);
			return Arrow;
		}
	}

	AWildActionPreviewArrow* NewArrow = World->SpawnActor<AWildActionPreviewArrow>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (NewArrow)
	{
		PooledPreviewArrows.Add(NewArrow);
	}
	return NewArrow;
}

AWildPreviewGhostUnit* UChainComponent::GetOrCreatePooledGhostUnit()
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;

	for (AWildPreviewGhostUnit* Ghost : PooledGhostUnits)
	{
		if (IsValid(Ghost) && Ghost->IsHidden())
		{
			Ghost->SetActorHiddenInGame(false);
			Ghost->SetActorTickEnabled(true);
			return Ghost;
		}
	}

	AWildPreviewGhostUnit* NewGhost = World->SpawnActor<AWildPreviewGhostUnit>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (NewGhost)
	{
		PooledGhostUnits.Add(NewGhost);
	}
	return NewGhost;
}

void UChainComponent::UpdatePublicPreviews()
{
	ClearPublicPreviews();

	UWorld* World = GetWorld();
	if (!World) return;

	UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(World->GetGameInstance());

	for (const FWildActionData& Action : PublicMainActions)
	{
		if (Action.ActionType == EWildActionType::SummonUnit)
		{
			ATile* TargetTile = Action.TargetTile;
			if (!IsValid(TargetTile)) continue;

			// 소환될 대상 타일에 반투명 유닛 고스트 표시 (스케일 3, Z축 높이 +50)
			const FVector SpawnLoc = TargetTile->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
			AWildPreviewGhostUnit* GhostUnit = GetOrCreatePooledGhostUnit();

			if (GhostUnit)
			{
				GhostUnit->SetActorLocation(SpawnLoc);
				GhostUnit->SetActorScale3D(FVector(3.0f, 3.0f, 3.0f));
				GhostUnit->SetActorHiddenInGame(false);
				GhostUnit->SetActorTickEnabled(true);
				ActivePreviewActors.Add(GhostUnit);

				UTexture2D* CardTexture = nullptr;
				UTexture2D* FrameTexture = nullptr;

				if (GI && GI->CardDataTable && Action.CardID != NAME_None)
				{
					if (FCardData* FoundCard = GI->CardDataTable->FindRow<FCardData>(Action.CardID, TEXT("PreviewSummonUnit")))
					{
						CardTexture = FoundCard->GetCardImage();
						FrameTexture = FoundCard->GetCardFrameImage();
					}
				}

				GhostUnit->SetupGhostUnit(CardTexture, FrameTexture, PreviewGhostTintColor, PreviewGhostOpacity, PreviewGhostDesaturation);
			}
		}
		else if (Action.ActionType == EWildActionType::UnitMove)
		{
			ATile* SourceTile = Action.SourceTile;
			ATile* TargetTile = Action.TargetTile;
			if (!IsValid(TargetTile)) continue;

			// 1. 유닛 위치(상단)에서 목표 위치(상단)로 둥근 곡선의 호(Arc) 화살표 표시
			if (IsValid(SourceTile))
			{
				const FVector StartLoc = SourceTile->GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);
				const FVector EndLoc = TargetTile->GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);

				AWildActionPreviewArrow* PreviewArrow = GetOrCreatePooledArrow();

				if (PreviewArrow)
				{
					PreviewArrow->SetActorHiddenInGame(false);
					PreviewArrow->SetupArrow(
						StartLoc,
						EndLoc,
						FLinearColor(0.15f, 0.75f, 1.0f, 1.0f)
					);
					ActivePreviewActors.Add(PreviewArrow);
				}
			}

			// 2. 이동할 목표 위치에 반투명색으로 유닛 위치 (스케일 3, Z축 높이 +50)
			const FVector SpawnLoc = TargetTile->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
			AWildPreviewGhostUnit* GhostUnit = GetOrCreatePooledGhostUnit();

			if (GhostUnit)
			{
				GhostUnit->SetActorLocation(SpawnLoc);
				GhostUnit->SetActorScale3D(FVector(3.0f, 3.0f, 3.0f));
				GhostUnit->SetActorHiddenInGame(false);
				GhostUnit->SetActorTickEnabled(true);
				ActivePreviewActors.Add(GhostUnit);

				UTexture2D* CardTexture = nullptr;
				UTexture2D* FrameTexture = nullptr;

				if (IsValid(SourceTile) && SourceTile->GetUnit())
				{
					CardTexture = SourceTile->GetUnit()->GetCardIllustrationTexture();
					FrameTexture = SourceTile->GetUnit()->GetCardData().GetCardFrameImage();
				}

				GhostUnit->SetupGhostUnit(CardTexture, FrameTexture, PreviewGhostTintColor, PreviewGhostOpacity, PreviewGhostDesaturation);
			}
		}
	}
}

void UChainComponent::OnServerPhaseChanged(EWildCardPhase NewPhase)
{

	if (NewPhase != EWildCardPhase::Action_Resolution && bAwaitingSearchLibrarySelection)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(SearchLibraryTimeoutTimerHandle);
		}
		bAwaitingSearchLibrarySelection = false;
	}
	if (GetOwnerRole() != ROLE_Authority) return;

	AInGameState* GameState = Cast<AInGameState>(GetOwner());
	if (!GameState) return;

	switch (NewPhase)
	{
	case EWildCardPhase::Action_Public:
		{
			UE_LOG(LogWildChain, Log, TEXT("[Server] Action_Public 진입: 메인 행동 순차 처리를 준비합니다."));

			// [수정] 대기열(Queue)과 보관소(Pending)가 '모두' 비어있을 때만 새로 수집합니다.
			// TODO : 물어보기
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
					FWildActionData FirstAction = FirstPS->GetPendingMainAction();
					FWildActionData SecondAction = SecondPS->GetPendingMainAction();

					// [규칙 1 적용] 선공과 후공의 메인 행동 충돌 검사 및 [시전 취소] 처리
					CheckAndResolveMainActionConflicts(FirstAction, SecondAction, FirstPS, SecondPS);

					if (FirstAction.ActionType != EWildActionType::None) MainActionQueue.Add(FirstAction);
					if (SecondAction.ActionType != EWildActionType::None) MainActionQueue.Add(SecondAction);

					PublicMainActions.Empty();
					if (FirstAction.ActionType != EWildActionType::None)
					{
						FWildActionData CopyFirst = FirstAction;
						if (CopyFirst.CardInstanceID != -1 && FirstPS)
						{
							if (UCardBase* Card = FirstPS->FindCardInstanceByID(CopyFirst.CardInstanceID))
							{
								CopyFirst.CardID = Card->CardID;
							}
						}
						PublicMainActions.Add(CopyFirst);
					}
					if (SecondAction.ActionType != EWildActionType::None)
					{
						FWildActionData CopySecond = SecondAction;
						if (CopySecond.CardInstanceID != -1 && SecondPS)
						{
							if (UCardBase* Card = SecondPS->FindCardInstanceByID(CopySecond.CardInstanceID))
							{
								CopySecond.CardID = Card->CardID;
							}
						}
						PublicMainActions.Add(CopySecond);
					}

					if (!IsRunningDedicatedServer())
					{
						UpdatePublicPreviews();
					}

					if (UWildCombatLogComponent* LogComp = GameState->GetCombatLogComponent())
					{
						bool bP1NoAction = (FirstAction.ActionType == EWildActionType::None);
						bool bP2NoAction = (SecondAction.ActionType == EWildActionType::None);

						FString P1CardName = TEXT("");
						if (FirstAction.CardInstanceID != -1)
						{
							if (UCardBase* Card = FirstPS->FindCardInstanceByID(FirstAction.CardInstanceID))
							{
								P1CardName = UWildCombatLogComponent::GetCardDisplayName(GetWorld(), Card->CardID);
							}
						}

						FString P2CardName = TEXT("");
						if (SecondAction.CardInstanceID != -1)
						{
							if (UCardBase* Card = SecondPS->FindCardInstanceByID(SecondAction.CardInstanceID))
							{
								P2CardName = UWildCombatLogComponent::GetCardDisplayName(GetWorld(), Card->CardID);
							}
						}

						int32 P1Force = FirstPS->GetStatComponent() ? FirstPS->GetStatComponent()->CurrentForce : 0;
						int32 P2Force = SecondPS->GetStatComponent() ? SecondPS->GetStatComponent()->CurrentForce : 0;

						int32 P1Cost = 1;
						if (FirstAction.CardInstanceID != -1)
						{
							if (UCardBase* Card = FirstPS->FindCardInstanceByID(FirstAction.CardInstanceID))
							{
								P1Cost = Card->CurrentForce > 0 ? Card->CurrentForce : 1;
							}
						}

						int32 P2Cost = 1;
						if (SecondAction.CardInstanceID != -1)
						{
							if (UCardBase* Card = SecondPS->FindCardInstanceByID(SecondAction.CardInstanceID))
							{
								P2Cost = Card->CurrentForce > 0 ? Card->CurrentForce : 1;
							}
						}

						int32 P1TargetQ = FirstAction.TargetTile ? FirstAction.TargetTile->Axial.Q : -1;
						int32 P1TargetR = FirstAction.TargetTile ? FirstAction.TargetTile->Axial.R : -1;
						int32 P2TargetQ = SecondAction.TargetTile ? SecondAction.TargetTile->Axial.Q : -1;
						int32 P2TargetR = SecondAction.TargetTile ? SecondAction.TargetTile->Axial.R : -1;

						LogComp->LogPhasePublic(FirstPlayerID, bP1NoAction, P1CardName, FirstAction.ActionType, P1Force, FMath::Max(0, P1Force - P1Cost), P1TargetQ, P1TargetR,
						                        SecondPlayerID, bP2NoAction, P2CardName, SecondAction.ActionType, P2Force, FMath::Max(0, P2Force - P2Cost), P2TargetQ, P2TargetR);
					}
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

	// 처리되는 메인 액션은 공개 대기열(PublicMainActions)에서 제거하여 프리뷰(반투명/화살표)를 종료
	for (int32 i = PublicMainActions.Num() - 1; i >= 0; --i)
	{
		if (PublicMainActions[i].PlayerID == ActionData.PlayerID && PublicMainActions[i].ActionType == ActionData.ActionType)
		{
			PublicMainActions.RemoveAt(i);
			break;
		}
	}
	if (!IsRunningDedicatedServer())
	{
		UpdatePublicPreviews();
	}

	if (ActionData.ActionType == EWildActionType::SummonUnit)
	{
		UCardBase* SummonCard = ActivatorPS->FindCardInstanceByID(ActionData.CardInstanceID);
		if (SummonCard)
		{
			// 손패에서 제거 (구역 이동: Hand -> Battlefield/Exile)
			// (포스/마나는 Private Phase 행동 결정 시 이미 소모되었으므로 중복 소모 방지)
			if (ActivatorPS->IsCardInHand(ActionData.CardInstanceID))
			{
				ActivatorPS->RemoveCardFromHand(ActionData.CardInstanceID, ECardZone::BATTLEFIELD);
			}

			UE_LOG(LogWildChain, Log, TEXT("[Server] %s 유닛을 소환합니다. (CardInstanceID: %d)"), 
				*SummonCard->CardID.ToString(), ActionData.CardInstanceID);

			//TODO : FCardData에 있는 unitclass 가져오기
			TSubclassOf<AUnitBase> SpawnUnitClass = UnitClass;
			
			FCardData NewUnitCardData;

			UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(GetWorld()->GetGameInstance());
			if (GI && GI->CardDataTable)
			{
				if (FCardData* FoundCardData = GI->CardDataTable->FindRow<FCardData>(SummonCard->CardID, TEXT("SummonUnitAction")))
				{
					NewUnitCardData = *FoundCardData;

					if (!FoundCardData->UnitActorClass.IsNull())
					{
						if (UClass* LoadedClass = FoundCardData->UnitActorClass.LoadSynchronous())
						{
							SpawnUnitClass = LoadedClass;
						}
					}
				}
			}

			// 첫 소환 시 플레이어 유닛이 아직 없다면 Player BP로 대체 소환
			//TODO : 유닛 소환 로직 확인으로 잠시 멈춰놓음
			bool bIsPlayerUnitSpawn = false;
			TSubclassOf<APlayerUnit> PlayerBPToSpawn = nullptr;
			if (bSpawnPlayerUnitOnFirstSummon && ActivatorPS->GetPlayerUnit() == nullptr)
			{
				if (PlayerUnitClass)
				{
					PlayerBPToSpawn = PlayerUnitClass;
				}
				else
				{
					PlayerBPToSpawn = ActivatorPS->GetPlayerUnitClass();
				}

				if (PlayerBPToSpawn)
				{
					bIsPlayerUnitSpawn = false;
				}
			}

			if (IsValid(ActionData.TargetTile))
			{
				if (bIsPlayerUnitSpawn && PlayerBPToSpawn)
				{
					ActionData.TargetTile->SpawnPlayerUnit(PlayerBPToSpawn, ActivatorPS, ActivatorPS->GetPlayerClass());
					GameState->HandleUnitSpawned(ActionData.TargetTile->GetUnit(), ActionData.TargetTile);
					UE_LOG(LogWildChain, Log, TEXT("[Server] First unit summoned as Player BP (%s) for Player %d!"),
						*PlayerBPToSpawn->GetName(), ActionData.PlayerID);
				}
				else
				{
					ActionData.TargetTile->SpawnUnit(NewUnitCardData, SpawnUnitClass, ActionData.PlayerID, ActionData.CardInstanceID);
					GameState->HandleUnitSpawned(ActionData.TargetTile->GetUnit(), ActionData.TargetTile);
				}

				if (UWildCombatLogComponent* LogComp = GameState->GetCombatLogComponent())
				{
					int32 TargetQ = ActionData.TargetTile->Axial.Q;
					int32 TargetR = ActionData.TargetTile->Axial.R;
					FString CoordStr = UWildCombatLogComponent::FormatTileCoord(TargetQ, TargetR, -1, -1);
					LogComp->LogActionResolve(LogComp->GetLastPublicPhaseLogIndex(), EWildActionType::SummonUnit, TargetQ, TargetR, FString::Printf(TEXT("%s에 소환됨"), *CoordStr));
				}
			}
			else
			{
				UE_LOG(LogWildChain, Error, TEXT("[Server] 소환 대상 타일(TargetTile)이 유효하지 않습니다!"));
				if (UWildCombatLogComponent* LogComp = GameState->GetCombatLogComponent())
				{
					LogComp->LogActionFailed(LogComp->GetLastPublicPhaseLogIndex(), TEXT("소환 대상 타일이 유효하지 않습니다."));
				}
			}

		}
	}
	else if (ActionData.ActionType == EWildActionType::CastSpell)
	{
		UCardBase* SpellCard = ActivatorPS->FindCardInstanceByID(ActionData.CardInstanceID);
		if (SpellCard)
		{
			if (ActivatorPS->IsCardInHand(ActionData.CardInstanceID))
			{
				ActivatorPS->RemoveCardFromHand(ActionData.CardInstanceID, ECardZone::EXILE);
			}
			UE_LOG(LogWildChain, Log, TEXT("[Server] %s 주문을 시전합니다. (CardInstanceID: %d)"), 
				*SpellCard->CardID.ToString(), ActionData.CardInstanceID);

			FCardData FoundCardData;
			bool bFoundData = false;
			UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(GetWorld()->GetGameInstance());
			if (GI && GI->CardDataTable)
			{
				if (FCardData* RowData = GI->CardDataTable->FindRow<FCardData>(SpellCard->CardID, TEXT("CastSpellAction")))
				{
					FoundCardData = *RowData;
					bFoundData = true;
				}
			}

			if (bFoundData && GameState)
			{
				if (FoundCardData.VisualCueTag.IsValid())
				{
					FWildVisualCueData CueData;
					CueData.CueTag = FoundCardData.VisualCueTag;
					CueData.InstigatorPlayerID = ActionData.PlayerID;
					CueData.SourceID = SpellCard->CardID;
					CueData.CueMessage = FText::FromString(FoundCardData.CardName);
					if (ActionData.TargetTile)
					{
						CueData.TargetQ = ActionData.TargetTile->Axial.Q;
						CueData.TargetR = ActionData.TargetTile->Axial.R;
					}
					GameState->Multicast_BroadcastVisualCue(CueData);
				}

				for (const TSubclassOf<UEffectBase>& EffectClass : FoundCardData.EffectClasses)
				{
					if (!EffectClass) continue;
					UEffectBase* Effect = NewObject<UEffectBase>(this, EffectClass);
					Effect->ExecuteEffect(ActionData, FoundCardData.EffectData);
				}
			}

			if (UWildCombatLogComponent* LogComp = GameState->GetCombatLogComponent())
			{
				FString SpellName = UWildCombatLogComponent::GetCardDisplayName(GetWorld(), SpellCard->CardID);
				LogComp->LogActionResolve(LogComp->GetLastPublicPhaseLogIndex(), EWildActionType::CastSpell, -1, -1, FString::Printf(TEXT("{%s} 주문 발동"), *SpellName));
			}
		}
	}
	else if (ActionData.ActionType == EWildActionType::UnitMove)
	{
		ATile* SourceTile = ActionData.SourceTile;
		ATile* TargetTile = ActionData.TargetTile;
		if (IsValid(SourceTile) && IsValid(TargetTile) && !TargetTile->HasUnit())
		{
			AUnitBase* MovingUnit = SourceTile->GetUnit();
			if (IsValid(MovingUnit))
			{
				UE_LOG(LogWildChain, Log, TEXT("[Server] %s 유닛을 이동합니다."), *MovingUnit->GetName());

				MovingUnit->SetActorLocation(TargetTile->GetActorLocation());
				TargetTile->SetPlacedUnit(MovingUnit);
				SourceTile->SetPlacedUnit(nullptr);
				MovingUnit->SetCurrentTile(TargetTile);

				SourceTile->FlushNetDormancy();
				TargetTile->FlushNetDormancy();

				GameState->HandleUnitMoved(ActionData.SourceTile, ActionData.TargetTile);

				// [시각/UI 연출] 가이드라인 단일 NetMulticast 이벤트 버스(Multicast_BroadcastVisualCue) 브로드캐스트
				FWildVisualCueData MoveCueData;
				MoveCueData.CueTag = FGameplayTag::RequestGameplayTag(FName("Visual.Action.Move"), false);
				MoveCueData.InstigatorPlayerID = ActionData.PlayerID;
				MoveCueData.SourceActor = MovingUnit;
				MoveCueData.TargetActor = TargetTile;
				MoveCueData.SourceQ = SourceTile->Axial.Q;
				MoveCueData.SourceR = SourceTile->Axial.R;
				MoveCueData.TargetQ = TargetTile->Axial.Q;
				MoveCueData.TargetR = TargetTile->Axial.R;
				MoveCueData.SourceID = FName(*FString::FromInt(MovingUnit->GetUnitCardData().CardID));
				MoveCueData.CueMessage = FText::FromString(TEXT("이동"));
				GameState->Multicast_BroadcastVisualCue(MoveCueData);

				if (UWildCombatLogComponent* LogComp = GameState->GetCombatLogComponent())
				{
					FString SrcStr = UWildCombatLogComponent::FormatTileCoord(SourceTile->Axial.Q, SourceTile->Axial.R, -1, -1);
					FString DstStr = UWildCombatLogComponent::FormatTileCoord(TargetTile->Axial.Q, TargetTile->Axial.R, -1, -1);
					LogComp->LogActionResolve(LogComp->GetLastPublicPhaseLogIndex(), EWildActionType::UnitMove, TargetTile->Axial.Q, TargetTile->Axial.R, FString::Printf(TEXT("%s → %s 이동"), *SrcStr, *DstStr));
				}
			}
		}
	}
	else if (ActionData.ActionType == EWildActionType::UnitAttack)
	{
		//TODO : 공격 당시 유닛 대신 실행 당시 타일 점유자를 공격해서 바꿔야한다는데...?
		AUnitBase* Attacker = ActionData.SourceTile->GetUnit();
		AUnitBase* Defender = ActionData.TargetTile->GetUnit();
		if (IsValid(Attacker) && IsValid(Defender))
		{
			UE_LOG(LogWildChain, Warning, TEXT("[Combat] %s 유닛이 %s 유닛을 공격합니다!"), *Attacker->GetName(), *Defender->GetName());

			Attacker->SetCardHealth(Attacker->GetCardHealth() - Defender->GetCardAttack());
			Defender->SetCardHealth(Defender->GetCardHealth() - Attacker->GetCardAttack());

			// [시각/UI 연출] 가이드라인 핵심 원칙 3 준수: 개별 NetMulticast RPC 제거 및 단일 Multicast_BroadcastVisualCue 이벤트 버스 통일
			FWildVisualCueData AttackCueData;
			AttackCueData.CueTag = FGameplayTag::RequestGameplayTag(FName("Visual.Action.Attack"), false);
			AttackCueData.InstigatorPlayerID = ActionData.PlayerID;
			AttackCueData.SourceActor = Attacker;
			AttackCueData.TargetActor = Defender;
			AttackCueData.SourceQ = ActionData.SourceTile ? ActionData.SourceTile->Axial.Q : -1;
			AttackCueData.SourceR = ActionData.SourceTile ? ActionData.SourceTile->Axial.R : -1;
			AttackCueData.TargetQ = ActionData.TargetTile ? ActionData.TargetTile->Axial.Q : -1;
			AttackCueData.TargetR = ActionData.TargetTile ? ActionData.TargetTile->Axial.R : -1;
			AttackCueData.NumericValue = Attacker->GetCardAttack();
			AttackCueData.SourceID = FName(*FString::FromInt(Attacker->GetUnitCardData().CardID));
			AttackCueData.CueMessage = FText::FromString(TEXT("공격"));
			GameState->Multicast_BroadcastVisualCue(AttackCueData);

			if (UWildCombatLogComponent* LogComp = GameState->GetCombatLogComponent())
			{
				int32 AtkQ = ActionData.SourceTile ? ActionData.SourceTile->Axial.Q : -1;
				int32 AtkR = ActionData.SourceTile ? ActionData.SourceTile->Axial.R : -1;
				int32 DefQ = ActionData.TargetTile ? ActionData.TargetTile->Axial.Q : -1;
				int32 DefR = ActionData.TargetTile ? ActionData.TargetTile->Axial.R : -1;
				int32 BattleQ = ActionData.TargetTile ? ActionData.TargetTile->Axial.Q : -1;
				int32 BattleR = ActionData.TargetTile ? ActionData.TargetTile->Axial.R : -1;

				FString AttackerName = Attacker->GetUnitCardData().CardName.IsEmpty() ? Attacker->GetName() : Attacker->GetUnitCardData().CardName;
				FString DefenderName = Defender->GetUnitCardData().CardName.IsEmpty() ? Defender->GetName() : Defender->GetUnitCardData().CardName;

				LogComp->LogCombat(AttackerName, AtkQ, AtkR, Attacker->GetOwnerId(),
				                   DefenderName, DefQ, DefR, Defender->GetOwnerId(),
				                   BattleQ, BattleR);
			}
		}
	}
	else if (ActionData.ActionType == EWildActionType::CastTacticalSkill)
	{
		APlayerUnit* CasterUnit = Cast<APlayerUnit>(ActionData.SourceTile ? ActionData.SourceTile->GetUnit() : nullptr);
		if (!CasterUnit && ActivatorPS)
		{
			CasterUnit = ActivatorPS->GetPlayerUnit();
		}

		if (IsValid(CasterUnit))
		{
			UE_LOG(LogWildChain, Log, TEXT("[Server] %s 전술 스킬 시전을 해결합니다."), *CasterUnit->GetName());
			CasterUnit->ExecuteTacticalSkillAction(ActionData, false);

			if (GameState)
			{
				if (UWildCombatLogComponent* LogComp = GameState->GetCombatLogComponent())
				{
					FString SkillName = CasterUnit->GetTacticalSkill() ? CasterUnit->GetTacticalSkill()->SkillName.ToString() : TEXT("전술 스킬");
					LogComp->LogActionResolve(LogComp->GetLastPublicPhaseLogIndex(), EWildActionType::CastTacticalSkill, -1, -1, FString::Printf(TEXT("{%s} 발동"), *SkillName));
				}
			}
		}
		else
		{
			UE_LOG(LogWildChain, Error, TEXT("[Server] CastTacticalSkill 실패: 시전 PlayerUnit을 찾을 수 없습니다."));
		}
	}
	else if (ActionData.ActionType == EWildActionType::TriggerTrap)
	{
		if (ActionData.SourceTile && ActionData.SourceTile->GetUnit())
		{
			ActionData.SourceTile->GetUnit()->Multicast_PlayEffectTriggerVisual();
		}

		const FCardData& CardData = ActionData.SourceTile->GetUnit()->GetCardData();
		for (const TSubclassOf<UEffectBase>& EffectClass : CardData.EffectClasses)
		{
			if (!EffectClass)
				continue;

			UEffectBase* Effect = NewObject<UEffectBase>(ActionData.SourceTile->GetUnit(), EffectClass);
			Effect->ExecuteEffect(ActionData, CardData.EffectData);
		}
}
	
}