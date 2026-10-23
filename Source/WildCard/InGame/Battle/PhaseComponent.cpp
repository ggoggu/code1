// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Battle/PhaseComponent.h"
#include "InGameState.h"
#include "InGamePlayerState.h"
#include "ChainComponent.h"
#include "WildCard.h"
#include "PlayerStatComponent.h"
#include "CardBase.h"
#include "Tile.h"
#include "TileMap.h"
#include "InGame/Unit/PlayerUnit.h"
#include "InGame/Unit/TacticalSkillBase.h"
#include "InGame/Battle/WildCombatLogComponent.h"


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

			if (UWildCombatLogComponent* LogComp = GameState->GetCombatLogComponent())
			{
				LogComp->LogTurnPriority(GameState->GetCurrentPriorityPlayerID());
				if (GameState->PlayerArray.Num() >= 2)
				{
					AInGamePlayerState* P1 = Cast<AInGamePlayerState>(GameState->PlayerArray[0]);
					AInGamePlayerState* P2 = Cast<AInGamePlayerState>(GameState->PlayerArray[1]);
					if (P1 && P2)
					{
						LogComp->LogGameset(P1->GetPlayerId(), P1->GetPlayerClass(), P2->GetPlayerId(), P2->GetPlayerClass());
					}
				}
				LogComp->LogArtifactSelect();
			}
		}
		
		StartPhaseCountdown(GamesetTime);

		//TEST
		GameState->GetMap()->TESTUnitSpawn();

		break;

	case EWildMatchState::Mulligan:
		UE_LOG(LogWildPhase, Log, TEXT("[Phase] Mulligan 진입"));
		// 모든 플레이어에게 성유물 동기화 및 더미 카드 5장 지급
		for (APlayerState* State : GameState->PlayerArray)
		{
			if (AInGamePlayerState* PS = Cast<AInGamePlayerState>(State))
			{
				PS->SyncArtifactToDisplay();
				PS->Debug_GiveDummyStartingHand();
			}
		}

		if (UWildCombatLogComponent* LogComp = GameState->GetCombatLogComponent())
		{
			if (GameState->PlayerArray.Num() >= 2)
			{
				AInGamePlayerState* P1 = Cast<AInGamePlayerState>(GameState->PlayerArray[0]);
				AInGamePlayerState* P2 = Cast<AInGamePlayerState>(GameState->PlayerArray[1]);
				if (P1 && P2)
				{
					FString P1Art = (P1->ArtifactDisplay.ArtifactID != NAME_None) ? UWildCombatLogComponent::GetArtifactDisplayName(GetWorld(), P1->ArtifactDisplay.ArtifactID) : TEXT("없음");
					FString P1Skill = (P1->GetSelectedTacticalSkillID() != NAME_None) ? UWildCombatLogComponent::GetTacticalSkillDisplayName(GetWorld(), P1->GetSelectedTacticalSkillID()) : TEXT("없음");
					FString P2Art = (P2->ArtifactDisplay.ArtifactID != NAME_None) ? UWildCombatLogComponent::GetArtifactDisplayName(GetWorld(), P2->ArtifactDisplay.ArtifactID) : TEXT("없음");
					FString P2Skill = (P2->GetSelectedTacticalSkillID() != NAME_None) ? UWildCombatLogComponent::GetTacticalSkillDisplayName(GetWorld(), P2->GetSelectedTacticalSkillID()) : TEXT("없음");

					LogComp->LogArtifactAndSkillReveal(P1->GetPlayerId(), P1Art, P1Skill, P2->GetPlayerId(), P2Art, P2Skill);
				}
			}
		}

		StartPhaseCountdown(MulliganTime);
		break;

	case EWildMatchState::Mulligan_End:
		{
			UE_LOG(LogWildPhase, Log, TEXT("[Phase] Mulligan_End: 서버에서 카드 교체 및 최종 패 확정 중..."));
			GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);

			if (UWildCombatLogComponent* LogComp = GameState->GetCombatLogComponent())
			{
				LogComp->LogMulligan();
			}

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
		if (UWildCombatLogComponent* LogComp = GameState->GetCombatLogComponent())
		{
			LogComp->LogPhasePrivate();
		}
		StartPhaseCountdown(ActionPrivateTime);
		break;

	case EWildCardPhase::Action_Public:
		UE_LOG(LogWildPhase, Log, TEXT("[Phase] Action_Public 진입"));
		ConsecutivePassCount = 0; 
		StartPhaseCountdown(ActionPublicTime);
		break;

	case EWildCardPhase::Action_Resolution:
		GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);
		if (UChainComponent* ChainComp = GameState->FindComponentByClass<UChainComponent>())
		{
			GameState->Multicast_NotifyTiming(EWildTimingNoticeType::ChainResolution, FText::FromString(TEXT("체인 처리")), 1.8f);
			GetWorld()->GetTimerManager().SetTimer(
				PhaseTimerHandle,
				FTimerDelegate::CreateUObject(ChainComp, &UChainComponent::ResolveChain),
				1.8f,
				false
			);
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

	if (ActionData.ActionType != EWildActionType::None && ActionData.ActionType != EWildActionType::CastTacticalSkill && ActionData.CardInstanceID != -1)
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
	else if (ActionData.ActionType == EWildActionType::CastTacticalSkill)
	{
		APlayerUnit* PlayerUnit = Cast<APlayerUnit>(ActionData.SourceTile ? ActionData.SourceTile->GetUnit() : nullptr);
		if (!PlayerUnit)
		{
			PlayerUnit = PS->GetPlayerUnit();
		}

		if (!PlayerUnit)
		{
			UE_LOG(LogWildPhase, Warning, TEXT("[보안] Player %d 의 PlayerUnit을 찾을 수 없어 전술 스킬을 시전할 수 없습니다."), CallerID);
			return;
		}

		if (!PlayerUnit->CanUseTacticalSkillAction(ActionData))
		{
			UE_LOG(LogWildPhase, Warning, TEXT("[보안] Player %d 의 전술 스킬 사용 조건이 충족되지 않았습니다."), CallerID);
			return;
		}

		if (UTacticalSkillBase* Skill = PlayerUnit->GetTacticalSkill())
		{
			if (UPlayerStatComponent* Stat = PS->GetStatComponent())
			{
				if (Stat->Mana < Skill->ManaCost)
				{
					UE_LOG(LogWildPhase, Warning, TEXT("[보안] Player %d 의 마나가 부족합니다. (필요: %d, 현재: %d)"), CallerID, Skill->ManaCost, Stat->Mana);
					return;
				}
				Stat->ConsumeMana(Skill->ManaCost);
			}
		}
	}

	// 체인 컴포넌트에 액션 추가
	if (UChainComponent* ChainComp = GameState->FindComponentByClass<UChainComponent>())
	{
		// DeclareAction을 호출하면 내부에서 스택 추가 + 상대방에게 우선권/프롬프트 이전이 자동으로 처리됩니다.
		ChainComp->DeclareAction(ActionData);

		if (UWildCombatLogComponent* LogComp = GameState->GetCombatLogComponent())
		{
			FString CardName = TEXT("");
			int32 ActionCardCost = 1;
			if (ActionData.ActionType == EWildActionType::CastTacticalSkill)
			{
				APlayerUnit* PlayerUnit = Cast<APlayerUnit>(ActionData.SourceTile ? ActionData.SourceTile->GetUnit() : nullptr);
				if (!PlayerUnit)
				{
					PlayerUnit = PS->GetPlayerUnit();
				}
				if (PlayerUnit && PlayerUnit->GetTacticalSkill())
				{
					CardName = PlayerUnit->GetTacticalSkill()->SkillName.ToString();
					ActionCardCost = PlayerUnit->GetTacticalSkill()->ManaCost;
				}
			}
			else if (ActionData.CardInstanceID != -1 && PS)
			{
				if (UCardBase* Card = PS->FindCardInstanceByID(ActionData.CardInstanceID))
				{
					CardName = UWildCombatLogComponent::GetCardDisplayName(GetWorld(), Card->CardID);
					ActionCardCost = Card->CurrentForce > 0 ? Card->CurrentForce : 1;
				}
			}
			int32 CurForce = PS->GetStatComponent() ? PS->GetStatComponent()->CurrentForce : 0;
			int32 TargetQ = ActionData.TargetTile ? ActionData.TargetTile->Axial.Q : -1;
			int32 TargetR = ActionData.TargetTile ? ActionData.TargetTile->Axial.R : -1;

			FString TargetUnitName = TEXT("");
			int32 TargetUnitOwner = -1;
			if (ActionData.TargetTile)
			{
				if (AUnitBase* PlacedUnit = ActionData.TargetTile->GetUnit())
				{
					TargetUnitName = PlacedUnit->GetUnitCardData().CardName;
					if (TargetUnitName.IsEmpty())
					{
						TargetUnitName = PlacedUnit->GetName();
					}
					TargetUnitOwner = PlacedUnit->GetOwnerId();
				}
			}

			int32 StackCount = FMath::Max(1, ChainComp->GetActionStackNum());
			LogComp->LogChainCast(StackCount, CallerID, CardName, CurForce + ActionCardCost, CurForce, TargetQ, TargetR, TargetUnitName, TargetUnitOwner, true);
		}
	}
	else
	{
		UE_LOG(LogWildPhase, Error, TEXT("[오류] GameState에서 ChainComponent를 찾을 수 없습니다!"));
		return;
	}

	// 누군가 액션을 했으므로 연속 패스 카운트 초기화
	ConsecutivePassCount = 0;
	StartPhaseCountdown(ActionPublicTime);
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
		int32 OpponentID = GetOpponentPlayerID(CallerID);
		if (OpponentID != -1)
		{
			// [핵심] 패스(행동 미공개) 시: 선공권(Priority)은 유지하고, 체인 권한(Prompt)만 상대에게 넘김
			GameState->SetCurrentChainPromptID(OpponentID);
			
			UE_LOG(LogWildPhase, Log, TEXT("[Server] 패스 확인: 선공권은 현재 보유자가 유지하며, 체인 대응 여부만 Player %d 에게 묻습니다."), OpponentID);
			StartPhaseCountdown(ActionPublicTime);
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
	
	// 인원수에 맞춰 완료 판정 (솔로/봇 1인, 멀티 2인)
	int32 RequiredPlayers = 2;
	if (AInGameState* GameState = GetWorld()->GetGameState<AInGameState>())
	{
		RequiredPlayers = FMath::Clamp(GameState->PlayerArray.Num(), 1, 2);
	}
	if (GamesetReadyCount >= RequiredPlayers)
	{
		UE_LOG(LogWildPhase, Log, TEXT("[Server] 성유물 선택 완료 (%d/%d). Mulligan 페이즈로 진입합니다."), GamesetReadyCount, RequiredPlayers);
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

			UE_LOG(LogTemp, Warning, TEXT("[Server] Player %d 멀리건 완료 처리됨! 현재 레디 카운트: %d"), PS->GetPlayerID(), MulliganReadyCount);
		}
	}
	
	// 인원수에 맞춰 완료 판정 (솔로/봇 1인, 멀티 2인)
	int32 RequiredPlayers = 2;
	if (AInGameState* GameState = GetWorld()->GetGameState<AInGameState>())
	{
		RequiredPlayers = FMath::Clamp(GameState->PlayerArray.Num(), 1, 2);
	}
	if (MulliganReadyCount >= RequiredPlayers)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] 멀리건 완료 (%d/%d)! Turn_Start 페이즈로 전환합니다."), MulliganReadyCount, RequiredPlayers);
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

	// [가독성 개선] 액션 무결성 및 치트 검증을 함수화
	const FWildActionData ValidatedAction = ValidatePrivateAction(PS, ActionData);
	UPlayerStatComponent* Stat = PS->GetStatComponent();
	int32 NewCost = GetActionForceCost(PS, ValidatedAction);

	if (PS->bIsActionReady)
	{
		// 이미 행동을 제출했으나 private phase 종료 전 수정하는 경우
		int32 PrevCost = GetActionForceCost(PS, PS->GetPendingMainAction());

		if (Stat)
		{
			Stat->AddForce(PrevCost);
			if (Stat->CurrentForce < NewCost)
			{
				Stat->ConsumeForce(PrevCost);
				UE_LOG(LogWildPhase, Warning, TEXT("[Action] Player %d 수정 실패: 포스 부족 (필요: %d, 보유: %d)"),
					PS->GetPlayerId(), NewCost, Stat->CurrentForce);
				return;
			}
			Stat->ConsumeForce(NewCost);
		}

		PS->SetPendingMainAction(ValidatedAction);
		UE_LOG(LogWildPhase, Log, TEXT("[Server] Player %d 메인 행동 수정 완료. (Type: %d, 소모 포스: %d)"),
			PS->GetPlayerId(), (int32)ValidatedAction.ActionType, NewCost);
	}
	else
	{
		// 최초 행동 제출
		if (Stat && NewCost > 0)
		{
			if (Stat->CurrentForce < NewCost)
			{
				UE_LOG(LogWildPhase, Warning, TEXT("[Action] Player %d 제출 실패: 포스 부족 (필요: %d, 보유: %d)"),
					PS->GetPlayerId(), NewCost, Stat->CurrentForce);
				return;
			}
			Stat->ConsumeForce(NewCost);
		}

		PS->SetPendingMainAction(ValidatedAction);
		ActionReadyCount++;
		UE_LOG(LogWildPhase, Log, TEXT("[Server] Player %d 메인 행동 결정 완료. (Type: %d, 현재 %d/2)"), 
			PS->GetPlayerId(), (int32)ValidatedAction.ActionType, ActionReadyCount);
	}

	// 양쪽 모두 메인 행동 결정을 완료한 시점
	if (ActionReadyCount >= 2)
	{
		AdvanceFromPrivateActionPhase();
	}
}

// =========================================================================
// 액션 검증 헬퍼 함수군 (Action Validation Helpers)
// =========================================================================

bool UPhaseComponent::ValidateCardAction(AInGamePlayerState* PS, int32 CardInstanceID)
{
	if (!PS || CardInstanceID == -1) return false;

	if (!PS->IsCardInHand(CardInstanceID))
	{
		UE_LOG(LogWildPhase, Warning, TEXT("[보안] Player %d 가 손패에 없는 카드(InstanceID: %d)로 행동을 시도했습니다."), PS->GetPlayerId(), CardInstanceID);
		return false;
	}

	class UCardBase* Card = PS->FindCardInstanceByID(CardInstanceID);
	UPlayerStatComponent* Stat = PS->GetStatComponent();
	if (!Card || (Stat && Stat->Mana < Card->CurrentManaCost))
	{
		UE_LOG(LogWildPhase, Warning, TEXT("[보안] Player %d 의 마나가 부족하여 카드를 사용할 수 없습니다."), PS->GetPlayerId());
		return false;
	}

	return true;
}

bool UPhaseComponent::ValidateUnitMoveAction(AInGamePlayerState* PS, const FWildActionData& ActionData)
{
	if (!PS || !ActionData.SourceTile || !ActionData.TargetTile) return false;

	AUnitBase* MovingUnit = ActionData.SourceTile->GetUnit();
	if (!IsValid(MovingUnit) || MovingUnit->GetOwnerId() != PS->GetPlayerId())
	{
		UE_LOG(LogWildPhase, Error, TEXT("[보안 치트 감지] Player %d 의 유닛 이동 실패: 유닛이 유효하지 않거나 본인 유닛이 아닙니다."), PS->GetPlayerId());
		return false;
	}

	if (ActionData.TargetTile->HasUnit())
	{
		UE_LOG(LogWildPhase, Warning, TEXT("[규칙 위반] 이동 대상 타일에 이미 유닛이 점유 중입니다."));
		return false;
	}

	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (GameState && GameState->GetMap())
	{
		int32 MaxMoveDist = MovingUnit->GetMaxMoveDistance();
		TArray<ATile*> AllowedTiles = GameState->GetMap()->GetTilesWithinDistance(ActionData.SourceTile, MaxMoveDist);
		if (!AllowedTiles.Contains(ActionData.TargetTile))
		{
			UE_LOG(LogWildPhase, Error, TEXT("[보안 치트 감지] Player %d 가 허용된 이동 거리(%d칸)를 초과하여 이동하려 했습니다!"), PS->GetPlayerId(), MaxMoveDist);
			return false;
		}
	}

	return true;
}

bool UPhaseComponent::ValidateUnitAttackAction(AInGamePlayerState* PS, const FWildActionData& ActionData)
{
	if (!PS || !ActionData.SourceTile || !ActionData.TargetTile) return false;

	AUnitBase* Attacker = ActionData.SourceTile->GetUnit();
	AUnitBase* Defender = ActionData.TargetTile->GetUnit();
	if (!IsValid(Attacker) || !IsValid(Defender)) return false;

	if (Attacker->GetOwnerId() != PS->GetPlayerId() || Defender->GetOwnerId() == PS->GetPlayerId())
	{
		UE_LOG(LogWildPhase, Error, TEXT("[보안 치트 감지] 공격자/방어자 소유권 오류: 본인 유닛으로 적 유닛을 공격해야 합니다."));
		return false;
	}

	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (GameState && GameState->GetMap())
	{
		TArray<ATile*> Neighbors = GameState->GetMap()->GetTilesAtDistance(ActionData.SourceTile, 1);
		if (!Neighbors.Contains(ActionData.TargetTile))
		{
			UE_LOG(LogWildPhase, Error, TEXT("[보안 치트 감지] 공격 대상 타일이 사거리(1칸)를 벗어났습니다."));
			return false;
		}
	}

	return true;
}

FWildActionData UPhaseComponent::ValidatePrivateAction(AInGamePlayerState* PS, const FWildActionData& InAction)
{
	FWildActionData ValidatedAction = InAction;

	// 카드 기반 액션(소환/주문 등) 검증
	if (ValidatedAction.ActionType != EWildActionType::None && ValidatedAction.CardInstanceID != -1)
	{
		if (!ValidateCardAction(PS, ValidatedAction.CardInstanceID))
		{
			ValidatedAction.ActionType = EWildActionType::None;
			ValidatedAction.CardInstanceID = -1;
			ValidatedAction.CardID = NAME_None;
		}
		else if (PS)
		{
			if (class UCardBase* Card = PS->FindCardInstanceByID(ValidatedAction.CardInstanceID))
			{
				ValidatedAction.CardID = Card->CardID;
			}
		}
	}
	// 유닛 이동 검증 (비행 유닛 등 가변 거리 검증 포함)
	else if (ValidatedAction.ActionType == EWildActionType::UnitMove)
	{
		if (!ValidateUnitMoveAction(PS, ValidatedAction))
		{
			ValidatedAction.ActionType = EWildActionType::None;
			ValidatedAction.SourceTile = nullptr;
			ValidatedAction.TargetTile = nullptr;
		}
	}
	// 유닛 공격 검증
	else if (ValidatedAction.ActionType == EWildActionType::UnitAttack)
	{
		if (!ValidateUnitAttackAction(PS, ValidatedAction))
		{
			ValidatedAction.ActionType = EWildActionType::None;
			ValidatedAction.SourceTile = nullptr;
			ValidatedAction.TargetTile = nullptr;
		}
	}

	return ValidatedAction;
}

// =========================================================================
// 페이즈 공통 유틸리티 함수군 (Common Phase Helpers)
// =========================================================================

void UPhaseComponent::ClearAllPendingActions()
{
	if (AInGameState* GameState = GetWorld()->GetGameState<AInGameState>())
	{
		for (APlayerState* State : GameState->PlayerArray)
		{
			if (AInGamePlayerState* WildPS = Cast<AInGamePlayerState>(State))
			{
				WildPS->ClearPendingMainAction();
			}
		}

		if (UChainComponent* ChainComp = GameState->FindComponentByClass<UChainComponent>())
		{
			ChainComp->ClearPublicMainActions();
		}
	}
}

bool UPhaseComponent::HasAnyPlayerCommittedMainAction() const
{
	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (!GameState) return false;

	for (APlayerState* State : GameState->PlayerArray)
	{
		if (AInGamePlayerState* WildPS = Cast<AInGamePlayerState>(State))
		{
			const FWildActionData& PendingAction = WildPS->GetPendingMainAction();
			if (PendingAction.ActionCategory == EWildActionCategory::Main && PendingAction.ActionType != EWildActionType::None)
			{
				return true;
			}
		}
	}
	return false;
}

void UPhaseComponent::ResetCycleCounters()
{
	ActionReadyCount = 0;
	ConsecutivePassCount = 0;
}

void UPhaseComponent::StartPhaseCountdown(int32 DurationSeconds)
{
	if (AInGameState* GameState = GetWorld()->GetGameState<AInGameState>())
	{
		GameState->SetPhaseRemainTime(DurationSeconds);
		GetWorld()->GetTimerManager().SetTimer(PhaseTimerHandle, this, &UPhaseComponent::OnGlobalTimerTick, 1.0f, true);
	}
}

int32 UPhaseComponent::GetOpponentPlayerID(int32 PlayerID) const
{
	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (!GameState) return -1;

	for (APlayerState* State : GameState->PlayerArray)
	{
		if (State && State->GetPlayerId() != PlayerID)
		{
			return State->GetPlayerId();
		}
	}
	return -1;
}

void UPhaseComponent::AdvanceFromPrivateActionPhase()
{
	ActionReadyCount = 0;
	GetWorld()->GetTimerManager().ClearTimer(PhaseTimerHandle);

	if (HasAnyPlayerCommittedMainAction())
	{
		UE_LOG(LogWildPhase, Log, TEXT("[Server] 최소 1명이 행동을 결정했습니다. Action_Public(체인 대기) 페이즈로 전환합니다."));
		SetGamePhase(EWildCardPhase::Action_Public);
	}
	else
	{
		UE_LOG(LogWildPhase, Warning, TEXT("[Server] 규칙 7: 두 플레이어 모두 행동을 건너뛰었습니다. 턴 종료(Turn_End)로 진입합니다."));
		ClearAllPendingActions();
		SetGamePhase(EWildCardPhase::Turn_End);
	}
}

int32 UPhaseComponent::GetActionForceCost(AInGamePlayerState* PS, const FWildActionData& ActionData)
{
	if (!PS || ActionData.ActionType == EWildActionType::None)
	{
		return 0;
	}

	if (ActionData.ActionType == EWildActionType::SummonUnit || ActionData.ActionType == EWildActionType::CastSpell)
	{
		if (UCardBase* Card = PS->FindCardInstanceByID(ActionData.CardInstanceID))
		{
			return Card->CurrentForce > 0 ? Card->CurrentForce : (Card->CurrentManaCost > 0 ? Card->CurrentManaCost : 1);
		}
		return 1;
	}
	else if (ActionData.ActionType == EWildActionType::UnitMove || ActionData.ActionType == EWildActionType::UnitAttack)
	{
		return 1;
	}
	else if (ActionData.ActionType == EWildActionType::CastTacticalSkill)
	{
		if (APlayerUnit* PlayerUnit = PS->GetPlayerUnit())
		{
			if (UTacticalSkillBase* Skill = PlayerUnit->GetTacticalSkill())
			{
				return Skill->ManaCost > 0 ? Skill->ManaCost : 1;
			}
		}
		return 1;
	}

	return 0;
}

bool UPhaseComponent::CanPlayerAct(AInGamePlayerState* PS) const
{
	if (!PS) return false;

	UPlayerStatComponent* Stat = PS->GetStatComponent();
	if (!Stat) return false;

	int32 CurForce = Stat->CurrentForce;

	// 1. 패에서 낼 수 있는 카드가 있는지 검사
	if (CurForce > 0 && PS->CardComponent)
	{
		for (const FWildCardDisplayData& CardDisplay : PS->CardComponent->CurrentHandDisplay)
		{
			if (UCardBase* Card = PS->FindCardInstanceByID(CardDisplay.InstanceID))
			{
				int32 CardCost = Card->CurrentForce > 0 ? Card->CurrentForce : (Card->CurrentManaCost > 0 ? Card->CurrentManaCost : 1);
				if (CurForce >= CardCost)
				{
					return true;
				}
			}
			else
			{
				int32 CardCost = CardDisplay.Force > 0 ? CardDisplay.Force : (CardDisplay.CurrentManaCost > 0 ? CardDisplay.CurrentManaCost : 1);
				if (CurForce >= CardCost)
				{
					return true;
				}
			}
		}
	}

	// 2. 필드 유닛이 이동/공격할 수 있는지 검사
	if (CurForce >= 1)
	{
		AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
		if (GameState && GameState->GetMap())
		{
			for (const TWeakObjectPtr<ATile>& TilePtr : GameState->GetTilesArray())
			{
				if (TilePtr.IsValid())
				{
					if (AUnitBase* Unit = TilePtr->GetUnit())
					{
						if (Unit->GetOwnerId() == PS->GetPlayerId())
						{
							return true;
						}
					}
				}
			}
		}
	}

	// 3. 전술 스킬 검사
	if (APlayerUnit* PlayerUnit = PS->GetPlayerUnit())
	{
		if (UTacticalSkillBase* Skill = PlayerUnit->GetTacticalSkill())
		{
			if (!PlayerUnit->IsTacticalSkillUsedThisTurn())
			{
				if (Stat->Mana >= Skill->ManaCost || CurForce >= Skill->ManaCost)
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool UPhaseComponent::CanAnyPlayerAct() const
{
	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (!GameState) return false;

	for (APlayerState* State : GameState->PlayerArray)
	{
		if (AInGamePlayerState* PS = Cast<AInGamePlayerState>(State))
		{
			if (CanPlayerAct(PS))
			{
				return true;
			}
		}
	}
	return false;
}

void UPhaseComponent::CheckActionPhaseEnd(bool bActionOccurred)
{
	if (GetOwnerRole() != ROLE_Authority) return;

	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (!GameState) return;
	
	ClearAllPendingActions();
	ResetCycleCounters();

	// 선공과 후공 둘 다 행동하지 않았다면 턴 종료
	if (!bActionOccurred)
	{
		UE_LOG(LogWildPhase, Log, TEXT("[Phase] 선공과 후공 모두 행동하지 않았으므로 턴을 종료합니다."));
		SetGamePhase(EWildCardPhase::Turn_End);
		return;
	}

	// 단, 두 플레이어 모두 행동할 수 없을 경우, 시스템이 계산하여 자동으로 Action phase를 재시작하지 않고 End phase로 이동
	if (!CanAnyPlayerAct())
	{
		UE_LOG(LogWildPhase, Log, TEXT("[Phase] 두 플레이어 모두 행동 불가(자원 고갈). 자동으로 Action phase를 재시작하지 않고 End phase(Turn_End)로 이동합니다."));
		SetGamePhase(EWildCardPhase::Turn_End);
		return;
	}

	// 선공과 후공 중 하나라도 행동했다면 Action phase를 새로 시작
	UE_LOG(LogWildPhase, Log, TEXT("[Phase] 규칙 6: 메인 행동 처리 종료. 액션 페이즈(Action_Private)를 새로 시작합니다."));

	// 2초의 유예 시간을 둔 뒤 재시작
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
	
	ClearAllPendingActions();

	TMap<int32, FString> DrawnCardNames;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AInGamePlayerState* PS = It->Get()->GetPlayerState<AInGamePlayerState>())
		{
			UPlayerStatComponent* PlayerStat = PS->GetStatComponent();
			
			if (PlayerStat)
			{
				PlayerStat->RefreshForce();
			}

			UCardBase* DrawnCard = PS->DrawCard(TEXT("TurnStart_Draw"), 1);
			FString DrawnName = TEXT("");
			if (DrawnCard)
			{
				DrawnName = UWildCombatLogComponent::GetCardDisplayName(GetWorld(), DrawnCard->CardID);
			}

			DrawnCardNames.Add(PS->GetPlayerId(), DrawnName);

			UE_LOG(LogWildPhase, Log, TEXT("[Server] Player %d 가 턴 시작 드로우를 수행했습니다. (카드: %s)"), PS->GetPlayerId(), *DrawnName);
		}
	}

	if (GameState)
	{
		// 턴 시작시 시전되는 효과 시전
		GameState->OnTurnStartedDelegate.Broadcast();

		if (UWildCombatLogComponent* LogComp = GameState->GetCombatLogComponent())
		{
			if (GameState->PlayerArray.Num() >= 2)
			{
				int32 P1ID = GameState->PlayerArray[0]->GetPlayerId();
				int32 P2ID = GameState->PlayerArray[1]->GetPlayerId();

				FString P1Card = DrawnCardNames.Contains(P1ID) ? DrawnCardNames[P1ID] : TEXT("");
				FString P2Card = DrawnCardNames.Contains(P2ID) ? DrawnCardNames[P2ID] : TEXT("");

				LogComp->LogTurnStart(P1ID, P1Card, P2ID, P2Card);
				LogComp->LogForceCharge(P1ID, 1);
			}
		}
	}

	ResetCycleCounters();

	// 턴 시작 연출(위젯 표시 및 드로우) 시간 부여 후 비공개 액션 페이즈로 전환
	StartPhaseCountdown(TurnStartTime);
	GetWorld()->GetTimerManager().SetTimer(
		PhaseTimerHandle,
		FTimerDelegate::CreateUObject(this, &UPhaseComponent::SetGamePhase, EWildCardPhase::Action_Private),
		TurnStartTime,
		false
	);
}

void UPhaseComponent::HandleTurnEnd()
{
	if (GetOwnerRole() != ROLE_Authority) return;

	UE_LOG(LogWildPhase, Log, TEXT("[Server] 턴 종료 페이즈 진입. 양측 플레이어의 패 제한을 검사합니다."));

	if (AInGameState* GameState = GetWorld()->GetGameState<AInGameState>())
	{
		// 턴이 끝날 때 시전하는 효과 시전
		GameState->OnTurnEndedDelegate.Broadcast();

		if (UWildCombatLogComponent* LogComp = GameState->GetCombatLogComponent())
		{
			LogComp->LogTurnEnd();
		}
	}
	
	ClearAllPendingActions();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AInGamePlayerState* PS = It->Get()->GetPlayerState<AInGamePlayerState>())
		{
			if (PS->CardComponent)
			{
				while (PS->CardComponent->CurrentHandDisplay.Num() > 10)
				{
					const int32 CardIDToExile = PS->CardComponent->CurrentHandDisplay.Last().InstanceID;
					if (!PS->RemoveCardFromHand(CardIDToExile, ECardZone::EXILE))
					{
						UE_LOG(LogWildPhase, Error, TEXT("[Server] Player %d 초과 카드 추방 실패 (InstanceID: %d)"), PS->GetPlayerId(), CardIDToExile);
						break;
					}
					UE_LOG(LogWildPhase, Log, TEXT("[Server] Player %d 초과 패 제한(10장 초과)으로 카드 추방 (InstanceID: %d)"), PS->GetPlayerId(), CardIDToExile);
				}
			}
		}
	}

	ResetCycleCounters();

	// 턴 종료 연출 시간 부여 후 다음 턴 시작으로 전환
	StartPhaseCountdown(TurnEndTime);
	GetWorld()->GetTimerManager().SetTimer(
		PhaseTimerHandle,
		FTimerDelegate::CreateUObject(this, &UPhaseComponent::SetGamePhase, EWildCardPhase::Turn_Start),
		TurnEndTime,
		false
	);
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
		UE_LOG(LogTemp, Warning, TEXT("[System] 비공개 행동 결정 시간 초과! 강제로 페이즈를 전환합니다."));
		
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

		AdvanceFromPrivateActionPhase();
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

		ClearAllPendingActions();
		ResetCycleCounters();

		SetGamePhase(EWildCardPhase::Action_Private);
	}
}