// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Game/InGameController.h"
#include "WildCard.h"
#include "InGame/Interface/WildInteractableInterface.h"
#include "InGameMode.h"
#include "PhaseComponent.h"
#include "InGameState.h"
#include "InGamePlayerState.h"
#include "View/MVVMView.h"
#include "Blueprint/UserWidget.h"
#include "WildCardGameInstance.h"
#include "WildDeckSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "WildHUDWidget.h"
#include "Tile.h"
#include "InGame/Map/TileMap.h"
#include "TileSelectComponent.h"
#include "UnitBase.h"
#include "InGameState.h"
#include "ArtifactSelectionViewModel.h"
#include "MulliganViewModel.h"
#include "WildHUDViewModel.h"
#include "WildCardViewModel.h"
#include "SelectActionWidget.h"
#include "WildHandWidget.h"
#include "PlayerStatComponent.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "InGame/Unit/PlayerUnit.h"
#include "InGame/Unit/TacticalSkillBase.h"
#include "InGame/Battle/ChainComponent.h"


AInGameController::AInGameController()
{
	TileSelectComp = CreateDefaultSubobject<UTileSelectComponent>(TEXT("TileSelectComponent"));
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	CurrentClickTraceChannel = ECC_MouseClick;
}

void AInGameController::Server_CompleteMulligan_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[Server] PlayerController에서 Server_CompleteMulligan RPC 수신 완료!"));
	
	if (AInGameMode* GameMode = Cast<AInGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GameMode->PhaseComponent->ProcessPlayerMulliganReady(this); // 여기도 this를 넘겨주도록 수정!
	}
	else
	{
		// [추가된 로그] GameMode 캐스팅 실패 예외 처리 확인
		UE_LOG(LogTemp, Error, TEXT("[Server] GameMode를 찾을 수 없어 ProcessPlayerMulliganReady를 호출하지 못했습니다."));
	}
}

bool AInGameController::Server_CompleteMulligan_Validate()
{
	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (GameState && GameState->GetCurrentMatchState() != EWildMatchState::Mulligan)
	{
		return false;
	}
	return true;
}

void AInGameController::CompleteMulligan()
{
	if (IsLocalController())
	{
		UE_LOG(LogTemp, Display, TEXT("[Client] 내가 멀리건 완료 버튼을 눌렀다. 대기 UI 띄우기!"));
	}
	
	Server_CompleteMulligan();
}

void AInGameController::PlayChainCard(const FWildActionData& ActionData)
{
	Server_SubmitAction(ActionData);
}

void AInGameController::PassChain()
{
	FWildActionData PassAction;
	PassAction.ActionCategory = EWildActionCategory::Sub;
	PassAction.ActionType = EWildActionType::None;
	Server_SubmitAction(PassAction);
}

void AInGameController::SkipCurrentAction()
{
	if (!IsLocalController()) return;

	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>();
	if (!GameState || !PS) return;

	EWildCardPhase CurrentPhase = GameState->GetCurrentPhase();
	if (CurrentPhase == EWildCardPhase::Action_Private)
	{
		// private phase 종료 전까지 수정 및 스킵 선택 가능

		FWildActionData SkipAction;
		SkipAction.PlayerID = PS->GetPlayerId();
		SkipAction.ActionCategory = EWildActionCategory::Main;
		SkipAction.ActionType = EWildActionType::None;

		UE_LOG(LogWildPhase, Log, TEXT("[Client] 메인 행동 스킵(Pass) 요청 전송"));
		CompletePrivateAction(SkipAction);

		if (HUDInstance)
		{
			HUDInstance->UpdateSkipButtonState();
		}
	}
	else if (CurrentPhase == EWildCardPhase::Action_Public)
	{
		if (GameState->GetCurrentChainPromptID() != PS->GetPlayerId())
		{
			UE_LOG(LogWildChain, Warning, TEXT("[Client] 현재 체인 응답 권한(Prompt)이 없습니다."));
			return;
		}

		UE_LOG(LogWildChain, Log, TEXT("[Client] 체인(서브 액션) 패스 요청 전송"));
		PassChain();

		if (HUDInstance)
		{
			HUDInstance->UpdateSkipButtonState();
		}
	}
	else
	{
		UE_LOG(LogWildPhase, Warning, TEXT("[Client] 현재 페이즈(%s)에서는 스킵할 수 없습니다."), *UEnum::GetValueAsString(CurrentPhase));
	}
}

void AInGameController::CompleteGameset()
{
	
	UE_LOG(LogTemp, Warning, TEXT("[디버그] 1. CompleteGameset 함수 진입 성공! (버튼 클릭됨)"));

	if (!GamesetWidgetInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("[디버그] 2. 실패: GamesetWidgetInstance가 NULL입니다!"));
		return;
	}

	UMVVMView* View = GamesetWidgetInstance->GetExtension<UMVVMView>();
	if (!View)
	{
		UE_LOG(LogTemp, Error, TEXT("[디버그] 2. 실패: 위젯에서 UMVVMView 익스텐션을 찾을 수 없습니다!"));
		return;
	}

	
	UArtifactSelectionViewModel* VM = Cast<UArtifactSelectionViewModel>(View->GetViewModel(FName("ArtifactSelectionViewModel")).GetObject());
	if (!VM)
	{
		UE_LOG(LogTemp, Error, TEXT("[디버그] 2. 실패: 뷰모델 캐스팅에 실패했습니다! (이름 불일치 또는 생성 안됨)"));
		return;
	}

	FName SelectedArtifactID = VM->GetSelectedArtifactID();
	UE_LOG(LogTemp, Warning, TEXT("[디버그] 3. 현재 뷰모델이 인식하고 있는 선택된 성유물 ID: %s"), *SelectedArtifactID.ToString());

	if (SelectedArtifactID != NAME_None)
	{
		if (AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>())
		{
			PS->Server_SubmitArtifact(SelectedArtifactID);
		}
		Server_CompleteGameset();
		VM->SetGamesetActive(false);
		UE_LOG(LogTemp, Warning, TEXT("[Client] 4. 성공: 성유물 [%s] 서버 제출 및 레디 완료!"), *SelectedArtifactID.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Client] 4. 실패: 진행하려면 성유물을 선택해야 합니다! (SelectedArtifactID == NAME_None)"));
	}
	
}

void AInGameController::CompletePrivateAction(const FWildActionData& ActionData)
{
	if (IsLocalController())
	{
		UE_LOG(LogTemp, Log, TEXT("[Client] 메인 행동 서버로 전송 시도... ActionType: %d"), (int32)ActionData.ActionType);
	}
	Server_SubmitAction(ActionData);
}

void AInGameController::RefreshMulliganUI(const TArray<FWildCardDisplayData>& NewCards)
{
	
	if (!MulliganWidgetInstance)
	{
		ShowMulliganUI(); 
	}

	if (MulliganWidgetInstance)
	{
		if (UMVVMView* View = MulliganWidgetInstance->GetExtension<UMVVMView>())
		{
			// UMG 블루프린트 내부에서 자동 생성된 ViewModel 인스턴스를 가져와서 데이터만 주입합니다.
			UMulliganViewModel* VM = Cast<UMulliganViewModel>(View->GetViewModel(FName("MulliganViewModel")).GetObject());

			if (VM)
			{
				VM->SetupMulliganCards(NewCards);
				UE_LOG(LogTemp, Warning, TEXT("[Client] 멀리건 UI에 %d개의 카드 데이터 주입 완료!"), NewCards.Num());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[Client] ViewModel 연결 실패! UMG 에디터(ViewModels 패널)의 이름이 'MulliganViewModel'인지 확인하세요."));
			}
		}
	}
	
}

void AInGameController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		bShowMouseCursor = true;
		bEnableClickEvents = true;
		bEnableMouseOverEvents = true;

		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);

		if (AInGameState* GameState = GetWorld()->GetGameState<AInGameState>())
		{
			GameState->OnMatchStateChangedDelegate.AddDynamic(this, &AInGameController::OnMatchStateChanged);
			GameState->OnPhaseChangedDelegate.AddDynamic(this, &AInGameController::OnGamePhaseChanged);
			GameState->OnTimerChangedDelegate.AddDynamic(this, &AInGameController::OnTimerUpdated);
			GameState->OnChainPromptChangedDelegate.AddDynamic(this, &AInGameController::OnChainPromptChanged);
			GameState->OnMatchEndedDelegate.AddDynamic(this, &AInGameController::OnMatchEnded);
		}

		// 클라이언트 로컬에 저장된 선택 덱을 서버로 전송하여 동기화
		if (UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(GetGameInstance()))
		{
			if (UWildDeckSubsystem* DeckSub = GI->GetSubsystem<UWildDeckSubsystem>())
			{
				if (DeckSub->HasSelectedDeck())
				{
					Server_SubmitSelectedDeck(DeckSub->GetSelectedDeck());
				}
			}
		}
	}
}

void AInGameController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (IsLocalController())
	{
		// 카드를 들고(드래그) 있을 때는 WildHandWidget::HandleCardDragMoved에서 ScreenPos로 직접 호버를 처리하므로 PlayerTick에서 덮어쓰지 않음
		if (HUDInstance && HUDInstance->GetHandWidget() && HUDInstance->GetHandWidget()->IsDraggingCard())
		{
			return;
		}

		UpdateTileHoverUnderCursor();
		UpdateActionMenuPosition();
	}
}

void AInGameController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (IsLocalController())
	{
		if (UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(GetGameInstance()))
		{
			if (UWildDeckSubsystem* DeckSub = GI->GetSubsystem<UWildDeckSubsystem>())
			{
				if (DeckSub->HasSelectedDeck())
				{
					Server_SubmitSelectedDeck(DeckSub->GetSelectedDeck());
				}
			}
		}
	}

	if (AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>())
	{
		if (UPlayerStatComponent* Stat = PS->GetStatComponent())
		{
			Stat->OnManaChanged.RemoveDynamic(this, &AInGameController::OnPlayerManaChanged);
			Stat->OnManaChanged.AddDynamic(this, &AInGameController::OnPlayerManaChanged);
			if (MainHUDViewModel || HUDInstance)
			{
				OnPlayerManaChanged(Stat->Mana);
			}
		}
	}
}

// 다양한 케이스 테스트용으로 불피요시 지울 수 있음
void AInGameController::TryBindGameState()
{
	if (AInGameState* GameState = GetWorld()->GetGameState<AInGameState>())
	{
		// GameState가 도착했으므로 안전하게 델리게이트를 바인딩합니다.
		GameState->OnPhaseChangedDelegate.AddDynamic(this, &AInGameController::OnGamePhaseChanged);
		GameState->OnTimerChangedDelegate.AddDynamic(this, &AInGameController::OnTimerUpdated);
		GameState->OnChainPromptChangedDelegate.AddDynamic(this, &AInGameController::OnChainPromptChanged);
		GameState->OnMatchEndedDelegate.AddDynamic(this, &AInGameController::OnMatchEnded);
		
		// 늦게 연결되었을 경우를 대비해 현재 페이즈 상태를 한 번 수동으로 갱신해줍니다.
		OnGamePhaseChanged(GameState->GetCurrentPhase());
		
		UE_LOG(LogTemp, Warning, TEXT("[Client] GameState 델리게이트 바인딩 성공!"));
	}
	else
	{
		// 아직 복제되지 않았다면 0.1초 뒤에 다시 시도합니다.
		GetWorld()->GetTimerManager().SetTimer(BindTimer, this, &AInGameController::TryBindGameState, 0.1f, false);
	}
}

void AInGameController::Server_SubmitAction_Implementation(FWildActionData ActionData)
{
	AInGameMode* GameMode = Cast<AInGameMode>(GetWorld()->GetAuthGameMode());
	AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>();
	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();

	if (!GameMode || !GameMode->PhaseComponent || !PS || !GameState) return;

	// [보안 핵심] 클라이언트가 변조해서 보냈을 가능성을 원천 차단. 
	// 서버가 알고 있는 확실한 현재 접속자의 ID로 덮어씌웁니다.
	ActionData.PlayerID = PS->GetPlayerId();

	// 게임 상태(Phase)에 따라 올바른 로직으로 라우팅 (분기 처리)
	EWildCardPhase CurrentPhase = GameState->GetCurrentPhase();

	if (CurrentPhase == EWildCardPhase::Action_Private)
	{
		GameMode->PhaseComponent->ProcessPrivateActionReady(this, ActionData);
	}
	else if (CurrentPhase == EWildCardPhase::Action_Public)
	{
		if (ActionData.ActionType == EWildActionType::None)
		{
			// 타입이 None인 액션은 Pass로 간주
			GameMode->PhaseComponent->ProcessPublicPass(this);
		}
		else
		{
			GameMode->PhaseComponent->ProcessPlayPublicAction(this, ActionData);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] 보안: 현재 액션을 처리할 수 없는 페이즈입니다. (PlayerID: %d)"), ActionData.PlayerID);
	}
}

bool AInGameController::Server_SubmitAction_Validate(FWildActionData ActionData)
{
	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (GameState)
	{
		EWildCardPhase Phase = GameState->GetCurrentPhase();
		if (Phase != EWildCardPhase::Action_Private && Phase != EWildCardPhase::Action_Public)
		{
			return false;
		}
	}
	return true;
}

void AInGameController::OnMatchStateChanged(EWildMatchState NewState)
{
	if (MainHUDViewModel)
	{
		MainHUDViewModel->SetMatchState(NewState);
	}
	if (HUDInstance)
	{
		HUDInstance->UpdateSkipButtonState();
	}

	switch (NewState)
	{
	case EWildMatchState::Gameset:
		ShowGamesetUI();
		ShowTimingNotice(EWildTimingNoticeType::Gameset);
		UE_LOG(LogTemp, Log, TEXT("[Client] 매치 상태: 성유물 선택(Gameset) 시작"));
		break;
	case EWildMatchState::Mulligan:
		// 멀리건 상태 진입 시 UI 표시
		HideGamesetUI();
		ShowMulliganUI();
		ShowTimingNotice(EWildTimingNoticeType::Mulligan);
		UE_LOG(LogTemp, Log, TEXT("[Client] 매치 상태: 멀리건 시작"));
		break;

	case EWildMatchState::Mulligan_End:
		
		// 멀리건 종료 시 (애니메이션 등 대기) UI 숨기기 준비
		if (MulliganWidgetInstance)
		{
			if (UMVVMView* View = MulliganWidgetInstance->GetExtension<UMVVMView>())
			{
				UMulliganViewModel* VM = Cast<UMulliganViewModel>(View->GetViewModel(FName("UMulliganViewModel")).GetObject());
				if (VM)
				{
					// "상대방 대기 중..." 혹은 "카드 교체 중..." 상태를 UI에 표시 (ViewModel 필드 활용)
					VM->SetMulliganActive(false); 
				}
			}
		}
		UE_LOG(LogTemp, Log, TEXT("[Client] 모든 플레이어 준비 완료. 카드 교체 연출 시작."));
		
		break;

	case EWildMatchState::MainGame:
		// 본 게임 진입 시 멀리건 UI 확실히 제거
		HideMulliganUI();
		UE_LOG(LogTemp, Log, TEXT("[Client] 매치 상태: 본 게임 시작!"));
		break;

	case EWildMatchState::Match_End:
		UE_LOG(LogTemp, Log, TEXT("[Client] 매치 상태: 게임 종료"));
		break;
	}
}

void AInGameController::OnGamePhaseChanged(EWildCardPhase NewPhase)
{
	if (MainHUDViewModel)
	{
		MainHUDViewModel->SetTurnPhase(NewPhase);
	}
	if (HUDInstance)
	{
		HUDInstance->UpdateSkipButtonState();
	}

	// 페이즈 전환에 맞춰 화면에 타이밍 알림 위젯 표시
	switch (NewPhase)
	{
	case EWildCardPhase::Turn_Start:
		UE_LOG(LogWildPhase, Log, TEXT("[Client] 턴 시작 페이즈"));
		ShowTimingNotice(EWildTimingNoticeType::TurnStart);
		break;

	case EWildCardPhase::Action_Private:
		// 메인 행동 선택 UI 활성화 등
		UE_LOG(LogWildPhase, Log, TEXT("[Client] 행동 선택 단계"));
		ShowTimingNotice(EWildTimingNoticeType::ActionPhase);
		break;

	case EWildCardPhase::Action_Public:
		// 체인 대응 버튼 활성화 등
		UE_LOG(LogWildPhase, Log, TEXT("[Client] 체인 공방 단계"));
		// 서버에서 MainActionReveal 후 순차적으로 ChainStart를 Multicast하므로 중복 호출 방지 위해 생략
		break;

	case EWildCardPhase::Action_Resolution:
		// 체인 처리 애니메이션 집중 모드
		UE_LOG(LogWildPhase, Log, TEXT("[Client] 효과 처리 중..."));
		// 서버에서 ChainResolution을 Multicast하므로 중복 호출 방지 위해 생략
		break;

	case EWildCardPhase::Turn_End:
		UE_LOG(LogWildPhase, Log, TEXT("[Client] 턴 종료 페이즈"));
		ShowTimingNotice(EWildTimingNoticeType::TurnEnd);
		break;

	default:
		break;
	}
}

void AInGameController::OnChainPromptChanged(int32 NewPromptPlayerID)
{
	if (HUDInstance)
	{
		HUDInstance->UpdateSkipButtonState();
	}
}

void AInGameController::OnActionReadyChanged(bool bIsReady)
{
	if (HUDInstance)
	{
		HUDInstance->UpdateSkipButtonState();
	}
}

void AInGameController::Debug_Ready()
{
	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>();
	
	// GameState와 PlayerState가 유효한지 먼저 확인합니다.
	if (!GameState || !PS) return;

	EWildMatchState CurrentMatchState = GameState->GetCurrentMatchState();
	int32 MyPlayerID = PS->GetPlayerId();

	// [계층적 상태 체크] 
	// 1단계: 먼저 전체 매치 흐름(MatchState)이 준비 조작이 필요한 단계인지 확인합니다.
	switch (CurrentMatchState)
	{
	case EWildMatchState::Gameset:
		UE_LOG(LogWildPhase, Log, TEXT("[Debug] Player %d - Gameset 선택 완료!"), MyPlayerID);
		CompleteGameset();
		break;

	case EWildMatchState::Mulligan:
		UE_LOG(LogWildPhase, Log, TEXT("[Debug] Player %d - 멀리건 완료!"), MyPlayerID);
		CompleteMulligan();
		break;

	case EWildMatchState::MainGame:
		// 2단계: 본 게임 중인 경우, 현재 턴의 세부 페이즈(CardPhase)를 확인합니다.
		{
			EWildCardPhase CurrentPhase = GameState->GetCurrentPhase();
			
			// 비공개 행동 결정 단계에서만 'Ready' 처리가 의미가 있습니다.
			if (CurrentPhase == EWildCardPhase::Action_Private)
			{
				UE_LOG(LogWildPhase, Log, TEXT("[Debug] Player %d - 메인 행동 결정 완료!"), MyPlayerID);

				FWildActionData DummyAction;
				DummyAction.PlayerID = MyPlayerID;
				DummyAction.ActionType = EWildActionType::UnitAttack; // 테스트용 더미 액션 설정
		
				CompletePrivateAction(DummyAction);
			}
			else
			{
				UE_LOG(LogWildPhase, Log, TEXT("[Debug] Player %d - 현재 턴 페이즈(%s)는 'Ready' 조작이 필요 없는 단계입니다."), 
					MyPlayerID, *UEnum::GetValueAsString(CurrentPhase));
			}
			break;
		}

	default:
		// 그 외 초기화(Init)나 종료(Match_End) 단계에서는 조작을 무시합니다.
		UE_LOG(LogWildPhase, Log, TEXT("[Debug] Player %d - 현재 매치 상태(%s)에서는 'Ready' 명령을 수행할 수 없습니다."), 
			MyPlayerID, *UEnum::GetValueAsString(CurrentMatchState));
		break;
	}
}

void AInGameController::Debug_Pass()
{
	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>();
	if (!GameState || !PS) return;

	if (GameState->GetCurrentPhase() == EWildCardPhase::Action_Public)
	{
		// 우선권 보유 여부와 관계없이 명령 시도 (로직에서 걸러짐)
		UE_LOG(LogWildChain, Log, TEXT("[Debug] Player %d - 체인 패스 시도"), PS->GetPlayerId());
		//Server_PassChain(); 
	}
	else
	{
		UE_LOG(LogWildChain, Log, TEXT("[Debug] Player %d - 현재 패스할 수 있는 단계가 아닙니다."), PS->GetPlayerId());
	}
}

void AInGameController::Debug_Chain()
{
	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>();
	if (!GameState || !PS) return;

	// 체인 공방 단계(Action_Public)에서만 작동
	if (GameState->GetCurrentPhase() == EWildCardPhase::Action_Public)
	{
		int32 MyID = PS->GetPlayerId();
		
		// 체인 데이터 생성 (메인 행동과 구분하기 위해 스펠 타입으로 설정)
		FWildActionData ChainAction;
		ChainAction.PlayerID = MyID;
		ChainAction.ActionType = EWildActionType::CastSpell; // 메인 행동(Attack)과 구분용
		
		UE_LOG(LogWildChain, Log, TEXT("[Debug] Player %d - 역체인 발동 시도!"), MyID);
		PlayChainCard(ChainAction); // 8단계에서 구현한 서버 RPC 호출
	}
}

void AInGameController::Dev_SetMana(int32 NewMana)
{
	Server_DevSetMana(NewMana);
}

void AInGameController::SetMana10()
{
	Server_DevSetMana(10);
}

bool AInGameController::Server_DevSetMana_Validate(int32 NewMana)
{
	return true;
}

void AInGameController::Server_DevSetMana_Implementation(int32 NewMana)
{
	if (AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>())
	{
		if (UPlayerStatComponent* Stat = PS->GetStatComponent())
		{
			Stat->SetMana(NewMana);
			UE_LOG(LogTemp, Log, TEXT("[Server][DevCheat] Player %d 의 마나를 %d 로 설정했습니다."), PS->GetPlayerId(), NewMana);
		}
	}
}

bool AInGameController::Server_SubmitSelectedDeck_Validate(const FWildDeckData& InDeckData)
{
	return true;
}

void AInGameController::Server_SubmitSelectedDeck_Implementation(const FWildDeckData& InDeckData)
{
	if (AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>())
	{
		if (InDeckData.IsValidDeck())
		{
			PS->ApplyDeckData(InDeckData);
			PS->InitDeckFromData(InDeckData);
			UE_LOG(LogTemp, Log, TEXT("[Server] Player %d 의 선택 덱 '%s' (%d장) 동기화 완료"),
				PS->GetPlayerId(), *InDeckData.DeckName, InDeckData.CardIDs.Num());
		}
	}
}

ATile* AInGameController::GetTileUnderScreenPosition(FVector2D ScreenPosition)
{
	// 1. 유효성 검사
	if (!IsLocalController()) return nullptr;

	auto TraceTileAtPos = [this](const FVector2D& Pos) -> ATile*
	{
		FVector WorldDirection;
		FVector WorldOrigin;

		if (DeprojectScreenPositionToWorld(Pos.X, Pos.Y, WorldOrigin, WorldDirection))
		{
			FHitResult HitResult;
			FVector TraceEnd = WorldOrigin + (WorldDirection * 10000.0f);

			if (GetWorld()->LineTraceSingleByChannel(HitResult, WorldOrigin, TraceEnd, ECC_MouseClick))
			{
				ATile* HittedTile = Cast<ATile>(HitResult.GetActor());
				if (!HittedTile)
				{
					if (AUnitBase* HittedUnit = Cast<AUnitBase>(HitResult.GetActor()))
					{
						HittedTile = HittedUnit->GetCurrentTile();
					}
				}

				if (IsValid(HittedTile))
				{
					return HittedTile;
				}
			}
		}
		return nullptr;
	};

	// 1차 시도: 넘겨받은 좌표로 바로 시도 (뷰포트 좌표 또는 이미 일치하는 좌표)
	ATile* HittedTile = TraceTileAtPos(ScreenPosition);
	if (HittedTile)
	{
		return HittedTile;
	}

	// 2차 시도: Slate 절대 스크린 좌표인 경우 뷰포트 좌표로 변환하여 시도
	FVector2D PixelPosition;
	FVector2D ViewportPosition;
	USlateBlueprintLibrary::AbsoluteToViewport(this, ScreenPosition, PixelPosition, ViewportPosition);
	if (PixelPosition != ScreenPosition)
	{
		HittedTile = TraceTileAtPos(PixelPosition);
		if (HittedTile)
		{
			return HittedTile;
		}
	}

	return nullptr;
}

void AInGameController::UpdateTileHover(FVector2D ScreenPosition)
{
	if (!IsLocalController()) return;

	ATile* NewTile = GetTileUnderScreenPosition(ScreenPosition);

	if (CurrentHoveredTile.Get() != NewTile)
	{
		// 이전 타일 호버 해제
		if (CurrentHoveredTile.IsValid())
		{
			CurrentHoveredTile->SetTileHovered(false);
		}

		// 새 타일 호버 적용
		if (IsValid(NewTile))
		{
			NewTile->SetTileHovered(true);
		}

		CurrentHoveredTile = NewTile;
	}
}

void AInGameController::UpdateTileHoverUnderCursor()
{
	if (!IsLocalController()) return;

	// 모달 UI(멀리건, 성유물 선택 등) 활성화 중일 때는 타일 호버 해제
	if (MulliganWidgetInstance || GamesetWidgetInstance)
	{
		ClearTileHover();
		return;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (GetMousePosition(MouseX, MouseY))
	{
		UpdateTileHover(FVector2D(MouseX, MouseY));
	}
	else
	{
		ClearTileHover();
	}
}

void AInGameController::ClearTileHover()
{
	if (CurrentHoveredTile.IsValid())
	{
		CurrentHoveredTile->SetTileHovered(false);
	}
	CurrentHoveredTile = nullptr;
}

void AInGameController::AttemptPlayCard(UWildCardViewModel* CardViewModel, FVector2D ScreenPosition)
{
	// 1. 유효성 검사
	if (!CardViewModel || !IsLocalController()) return;

	if (HUDInstance && HUDInstance->GetHandWidget() && HUDInstance->GetHandWidget()->IsCardInstanceDisabled(CardViewModel->InstanceID))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InGameController] 비활성화된 카드는 사용할 수 없습니다: ID %d"), CardViewModel->InstanceID);
		return;
	}

	// 드래그 종료 시 타일 호버 상태 해제
	ClearTileHover();

	// 2. 마우스 스크린 위치의 타일 감지
	ATile* HittedTile = GetTileUnderScreenPosition(ScreenPosition);
	if (!HittedTile)
	{
		float MouseX = 0.0f;
		float MouseY = 0.0f;
		if (GetMousePosition(MouseX, MouseY))
		{
			HittedTile = GetTileUnderScreenPosition(FVector2D(MouseX, MouseY));
		}
	}
	
	if (!IsValid(HittedTile))
	{
		UE_LOG(LogTemp, Warning, TEXT("드래그 드롭한 곳은 타일이 아닙니다."));
		return;
	}

	// (디버그용 시각화) 마우스가 닿은 3D 위치에 구체를 그립니다.
	DrawDebugSphere(GetWorld(), HittedTile->GetActorLocation(), 50.0f, 12, FColor::Orange, false, 2.0f);

	// 현재 게임의 페이즈 상태를 확인
	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (!GameState) return;

	EWildCardPhase CurrentPhase = GameState->GetCurrentPhase();

	// 4. 서버로 보낼 행동 구조체(FWildActionData) 조립
	FWildActionData ActionData;

	// PlayerID 세팅
	if (AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>())
	{
		ActionData.PlayerID = PS->GetPlayerId();
		if (UCardBase* Card = PS->FindCardInstanceByID(CardViewModel->InstanceID))
		{
			ActionData.CardID = Card->CardID;
		}
	}

	// 뷰모델에서 카드의 고유 InstanceID를 가져옴
	ActionData.CardInstanceID = CardViewModel->InstanceID;

	// 목표 타일 및 그리드 좌표(Axial) 설정
	ActionData.TargetTile = HittedTile;

	if (CurrentPhase == EWildCardPhase::Action_Private)
	{
		// 소환 가능한 타일인지 확인
		if (HittedTile->GetTileState() != ETileState::Spawnable)
		{
			UE_LOG(LogTemp, Warning, TEXT("소환 가능한 타일이 아닙니다: %s (현재 상태: %d)"), *HittedTile->GetName(), (int32)HittedTile->GetTileState());
			return;
		}

		if (HittedTile->HasUnit())
		{
			UE_LOG(LogTemp, Warning, TEXT("이미 유닛이 존재하는 타일입니다: %s"), *HittedTile->GetName());
			return;
		}

		if (!CardViewModel->CardType.HasTagExact(FGameplayTag::RequestGameplayTag(FName(TEXT("Unit")))))
		{
			UE_LOG(LogTemp, Warning, TEXT("유닛이 카드가 아닙니다 : %s"), *HittedTile->GetName());
			return;
		}

		// 비공개 메인 행동 결정 단계: 유닛 소환 등의 메인 액션 전송
		ActionData.ActionCategory = EWildActionCategory::Main;
		ActionData.ActionType = EWildActionType::SummonUnit;

		UE_LOG(LogWildPhase, Log, TEXT("[Client] 메인 행동(Action_Private)으로 카드 사용 시도: 타일 %s"), *HittedTile->GetName());
		CompletePrivateAction(ActionData);
		if (HUDInstance)
		{
			HUDInstance->UpdateSkipButtonState();
		}
	}
	else if (CurrentPhase == EWildCardPhase::Action_Public)
	{
		// 공개 체인 공방 단계: 스펠 등의 서브 액션 전송
		ActionData.ActionCategory = EWildActionCategory::Sub;
		ActionData.ActionType = EWildActionType::CastSpell;

		UE_LOG(LogWildPhase, Log, TEXT("[Client] 체인 서브 행동(Action_Public)으로 카드 사용 시도: 타일 %s"), *HittedTile->GetName());
		PlayChainCard(ActionData);
		if (HUDInstance)
		{
			HUDInstance->UpdateSkipButtonState();
		}
	}
	else
	{
		// 카드를 낼 수 없는 페이즈 (턴 시작, 처리 중, 게임 셋 등)
		UE_LOG(LogWildPhase, Warning, TEXT("[Client] 현재 카드 조작이 불가능한 페이즈(%s)입니다."), *UEnum::GetValueAsString(CurrentPhase));
	}
}

void AInGameController::TryBindEnemyState()
{
	if (AInGameState* GS = GetWorld()->GetGameState<AInGameState>())
	{
		for (APlayerState* PS : GS->PlayerArray)
		{
			AInGamePlayerState* EnemyPS = Cast<AInGamePlayerState>(PS);
			// 상대방을 찾았다면 (내 PlayerState가 아닌 경우)
			if (EnemyPS && EnemyPS != PlayerState) 
			{
				// 기존에 묶여있을 수 있는 이벤트 안전하게 해제 후 바인딩
				(EnemyPS->CardComponent)->OnHandCountChangedDelegate.RemoveDynamic(this, &AInGameController::OnEnemyHandCountUpdated);
				(EnemyPS->CardComponent)->OnHandCountChangedDelegate.AddDynamic(this, &AInGameController::OnEnemyHandCountUpdated);
				
				// 첫 동기화
				OnEnemyHandCountUpdated((EnemyPS->CardComponent)->HandCount);
				
				// 바인딩 성공했으므로 타이머 종료
				GetWorld()->GetTimerManager().ClearTimer(EnemyStateBindTimer);
				UE_LOG(LogTemp, Log, TEXT("[Client] 상대방 PlayerState(%s) 손패 이벤트 바인딩 성공!"), *EnemyPS->GetName());
				return;
			}
		}
	}
}

void AInGameController::OnEnemyHandCountUpdated(int32 NewHandCount)
{
	if (MainHUDViewModel)
	{
		MainHUDViewModel->SetEnemyHandCount(NewHandCount);
	}
}

void AInGameController::ShowMulliganUI()
{
	if (!MulliganWidgetClass || MulliganWidgetInstance) return;

	MulliganWidgetInstance = CreateWidget<UUserWidget>(this, MulliganWidgetClass);
	if (!MulliganWidgetInstance) return;

	MulliganWidgetInstance->AddToViewport();
    
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MulliganWidgetInstance->TakeWidget());
	
	SetInputMode(InputMode);
	bShowMouseCursor = true;
	
	if (AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>())
	{
		if ((PS->CardComponent)->CurrentHandDisplay.Num() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Client] 멀리건 UI가 생성되었습니다. 기존 %d장의 데이터를 강제 주입합니다."), (PS->CardComponent)->CurrentHandDisplay.Num());
			RefreshMulliganUI((PS->CardComponent)->CurrentHandDisplay);
		}
	}
}

void AInGameController::HideMulliganUI()
{
	if (MulliganWidgetInstance)
	{
		MulliganWidgetInstance->RemoveFromParent();
		MulliganWidgetInstance = nullptr;
		
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}

	CreateHUD();
}

UWildHUDWidget* AInGameController::GetOrCreateHUD()
{
	if (!IsLocalController() || HUDClass == nullptr)
	{
		return nullptr;
	}

	if (HUDInstance == nullptr)
	{
		HUDInstance = CreateWidget<UWildHUDWidget>(this, HUDClass);
		if (HUDInstance == nullptr)
		{
			return nullptr;
		}

		MainHUDViewModel = NewObject<UWildHUDViewModel>(this);
		HUDInstance->InitializeHUD(MainHUDViewModel);

		// Controller에 설정된 커스텀 타이밍 위젯 클래스가 있다면 HUDInstance로 전달 (Bridge)
		if (MainActionRevealWidgetClass && !HUDInstance->MainActionRevealWidgetClass) HUDInstance->MainActionRevealWidgetClass = MainActionRevealWidgetClass;
		if (TurnStartWidgetClass && !HUDInstance->TurnStartWidgetClass) HUDInstance->TurnStartWidgetClass = TurnStartWidgetClass;
		if (ChainStartWidgetClass && !HUDInstance->ChainStartWidgetClass) HUDInstance->ChainStartWidgetClass = ChainStartWidgetClass;
		if (ActionPhaseWidgetClass && !HUDInstance->ActionPhaseWidgetClass) HUDInstance->ActionPhaseWidgetClass = ActionPhaseWidgetClass;
		if (ChainResolutionWidgetClass && !HUDInstance->ChainResolutionWidgetClass) HUDInstance->ChainResolutionWidgetClass = ChainResolutionWidgetClass;
		if (TurnEndWidgetClass && !HUDInstance->TurnEndWidgetClass) HUDInstance->TurnEndWidgetClass = TurnEndWidgetClass;
		if (VictoryWidgetClass && !HUDInstance->VictoryWidgetClass) HUDInstance->VictoryWidgetClass = VictoryWidgetClass;
		if (DefeatWidgetClass && !HUDInstance->DefeatWidgetClass) HUDInstance->DefeatWidgetClass = DefeatWidgetClass;
		if (DefaultNoticeWidgetClass && !HUDInstance->DefaultNoticeWidgetClass) HUDInstance->DefaultNoticeWidgetClass = DefaultNoticeWidgetClass;

		for (const auto& Pair : TimingWidgetMap)
		{
			if (!HUDInstance->TimingWidgetMap.Contains(Pair.Key))
			{
				HUDInstance->TimingWidgetMap.Add(Pair.Key, Pair.Value);
			}
		}

		for (const auto& Pair : TimingNoticeDurations)
		{
			if (!HUDInstance->TimingNoticeDurations.Contains(Pair.Key))
			{
				HUDInstance->TimingNoticeDurations.Add(Pair.Key, Pair.Value);
			}
		}
	}

	return HUDInstance;
}

void AInGameController::CreateHUD()
{
	if (HUDClass == nullptr) return;

	UWildHUDWidget* HUD = GetOrCreateHUD();
	if (!HUD) return;

	if (!HUD->IsInViewport())
	{
		HUD->AddToViewport();
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
	
	if (AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>())
	{
		// [추가됨] 1단계: 게임 중 카드를 뽑거나 사용할 때 UI를 갱신하기 위해 이벤트 바인딩
		(PS->CardComponent)->OnMyHandUpdatedDelegate.AddDynamic(this, &AInGameController::UpdateHandFromState);
		PS->OnActionReadyChangedDelegate.RemoveDynamic(this, &AInGameController::OnActionReadyChanged);
		PS->OnActionReadyChangedDelegate.AddDynamic(this, &AInGameController::OnActionReadyChanged);

		if (UPlayerStatComponent* Stat = PS->GetStatComponent())
		{
			Stat->OnManaChanged.RemoveDynamic(this, &AInGameController::OnPlayerManaChanged);
			Stat->OnManaChanged.AddDynamic(this, &AInGameController::OnPlayerManaChanged);
			OnPlayerManaChanged(Stat->Mana);
		}

		if ((PS->CardComponent)->CurrentHandDisplay.Num() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Client] HUD 생성 완료. 즉시 동기화합니다."));
			
			// [수정됨] 1단계: 기존 중복 로직 대신 통합된 업데이트 함수 직접 호출
			UpdateHandFromState((PS->CardComponent)->CurrentHandDisplay); 
		}
	}

	HUD->UpdateSkipButtonState();

	if (AInGameState* GameState = GetWorld()->GetGameState<AInGameState>())
	{
		OnTimerUpdated(GameState->GetPhaseRemainTime());
	}

	// 즉시 바인딩 시도, 실패 시에만 타이머로 재시도
	TryBindEnemyState();
	if (!EnemyStateBindTimer.IsValid())
	{
		GetWorld()->GetTimerManager().SetTimer(EnemyStateBindTimer, this, &AInGameController::TryBindEnemyState, 0.5f, true);
	}

	ConnectedInHUD();
	ConnectDelegate();
}

void AInGameController::ShowGamesetUI()
{
	
	if (!GamesetWidgetClass || GamesetWidgetInstance) return;

	GamesetWidgetInstance = CreateWidget<UUserWidget>(this, GamesetWidgetClass);
	if (!GamesetWidgetInstance) return;

	GamesetWidgetInstance->AddToViewport();
    
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(GamesetWidgetInstance->TakeWidget());
	SetInputMode(InputMode);
	bShowMouseCursor = true;
	
	UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(GetGameInstance());
	if (GI && GI->ArtifactDataTable)
	{
		TArray<FArtifactData> ArtifactList;
		GI->ArtifactDataTable->ForeachRow<FArtifactData>(TEXT("ShowGamesetUI"), [&ArtifactList](const FName& Key, const FArtifactData& Value)
		{
			FArtifactData DataWithKey = Value;
			DataWithKey.ArtifactID = Key; 
			ArtifactList.Add(DataWithKey);
		});
		
		if (UMVVMView* View = GamesetWidgetInstance->GetExtension<UMVVMView>())
		{
			UArtifactSelectionViewModel* VM = Cast<UArtifactSelectionViewModel>(View->GetViewModel(FName("ArtifactSelectionViewModel")).GetObject());
			if (VM) 
			{
				UE_LOG(LogTemp, Warning, TEXT("[디버그] 뷰모델 연결 성공! %d개의 데이터를 UI로 보냅니다."), ArtifactList.Num());
				VM->SetupAvailableArtifacts(ArtifactList);
				VM->SetGamesetActive(true);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[디버그] 치명적 오류: 뷰모델이 NULL입니다! UMG의 Creation Type이나 이름을 확인하세요."));
			}
		}
	}
}

void AInGameController::HideGamesetUI()
{
	if (GamesetWidgetInstance)
	{
		GamesetWidgetInstance->RemoveFromParent();
		GamesetWidgetInstance = nullptr;
		// 마우스/인풋 모드는 MulliganUI나 MainHUD가 이어서 설정하게 둡니다.
	}
}

void AInGameController::ConnectDelegate()
{
	TileSelectComp->OnActionMenuClosed.AddDynamic(this, &AInGameController::HandleActionMenu);
	TileSelectComp->OnActionMenuRequested.AddDynamic(this, &AInGameController::HandleActionMenu);

	TileSelectComp->OnCombatableTilesFound.AddUObject(this, &AInGameController::FindCanCombatTiles);
	TileSelectComp->OnCombatableTilesCancled.AddUObject(this, &AInGameController::CancledCanCombatTiles);
	TileSelectComp->OnMoveableTilesFound.AddUObject(this, &AInGameController::FindCanMoveTiles);
	TileSelectComp->OnMoveableTilesCancled.AddUObject(this, &AInGameController::CancledCanMoveTiles);
	TileSelectComp->OnTacticalSkillTilesFound.AddUObject(this, &AInGameController::FindCanTacticalSkillTiles);
	TileSelectComp->OnTacticalSkillTilesCancled.AddUObject(this, &AInGameController::CancledCanTacticalSkillTiles);
	TileSelectComp->OnEffectableTilesFound.AddUObject(this, &AInGameController::FindCanEffectTiles);
	TileSelectComp->OnEffectableTilesCancled.AddUObject(this, &AInGameController::CancledCanEffectTiles);
}

void AInGameController::OnTimerUpdated(int32 RemainTime)
{
	if (MainHUDViewModel)
	{
		MainHUDViewModel->SetRemainTime(RemainTime);
	}

	if (HUDInstance)
	{
		HUDInstance->UpdateTimer(RemainTime);
	}
}

void AInGameController::OnPlayerManaChanged(int32 NewMana)
{
	if (MainHUDViewModel)
	{
		MainHUDViewModel->SetMyMana(NewMana);
	}

	if (HUDInstance)
	{
		HUDInstance->UpdateMana(NewMana);
	}
}

void AInGameController::UpdateHandFromState(const TArray<FWildCardDisplayData>& HandData)
{
	if (!MainHUDViewModel) return;

	// GameInstance에서 데이터 테이블 가져오기
	UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(GetGameInstance());
	UDataTable* CardTable = GI ? GI->CardDataTable : nullptr;

	TArray<UWildCardViewModel*> CardVMs;
	CardVMs.Reserve(HandData.Num()); 

	for (const FWildCardDisplayData& CardData : HandData)
	{
		UWildCardViewModel* NewCardVM = NewObject<UWildCardViewModel>(this);
		
		// DataTable에서 CardID(RowName)를 이용해 정적 데이터(이미지/이름) 찾기
		FCardData* StaticData = nullptr;
		if (CardTable)
		{
			StaticData = CardTable->FindRow<FCardData>(CardData.CardID, TEXT("UpdateHandFromState"));
		}

		// 찾은 데이터를 제대로 넘겨줌 (이제 이미지와 이름이 출력됩니다)
		NewCardVM->InitializeFromData(CardData, StaticData); 
		CardVMs.Add(NewCardVM);
	}

	MainHUDViewModel->UpdateMyHandCards(CardVMs);
}

//#. Select Action ///////////////////////////////////////
void AInGameController::HandlePlayerUnitActionMenu(APlayerUnit* InPlayerUnit)
{
	HandleActionMenu();
}

void AInGameController::CancelTileAction()
{
	if (TileSelectComp && TileSelectComp->GetCurrentState() == ETileInteractionState::ActionMenuOpen)
	{
		TileSelectComp->CancelAction();
	}
}

void AInGameController::SetActionMenuTestMode(bool bUseWidgetComp)
{
	bTestWidgetComponentMenu = bUseWidgetComp;
	UE_LOG(LogTemp, Warning, TEXT("[InGameController] 액션 메뉴 렌더링 모드 전환: %s"), bUseWidgetComp ? TEXT("WidgetComponent (머리 위 컴포넌트)") : TEXT("2D Screen HUD (현재 투영 방식)"));

	HandleActionMenu();
}

void AInGameController::HandleActionMenu()
{
	WILD_SIMPLE(Warning);

	if (!TileSelectComp || !HUDInstance)
	{
		return;
	}

	USelectActionWidget* GeneralWidget = SelectInstance ? SelectInstance.Get() : HUDInstance->GetSelectActionWidget();
	USelectActionWidget* PlayerWidget = PlayerSelectInstance ? PlayerSelectInstance.Get() : HUDInstance->GetPlayerSelectActionWidget();

	// PlayerSelectInstance가 늦게 로드되었을 경우를 대비한 바인딩 및 갱신
	if (!PlayerSelectInstance && PlayerWidget)
	{
		PlayerSelectInstance = PlayerWidget;
		PlayerSelectInstance->OnMoveButtonPressed.RemoveDynamic(TileSelectComp, &UTileSelectComponent::SelectMoveAction);
		PlayerSelectInstance->OnMoveButtonPressed.AddDynamic(TileSelectComp, &UTileSelectComponent::SelectMoveAction);
		PlayerSelectInstance->OnTacticalSkillButtonPressed.RemoveDynamic(this, &AInGameController::OnTacticalSkillActionSelected);
		PlayerSelectInstance->OnTacticalSkillButtonPressed.AddDynamic(this, &AInGameController::OnTacticalSkillActionSelected);
	}

	if (TileSelectComp->GetCurrentState() == ETileInteractionState::ActionMenuOpen)
	{
		if (AUnitBase* SelectedUnit = TileSelectComp->GetSelectedUnit())
		{
			if (bTestWidgetComponentMenu)
			{
				// [테스트 모드 B: 유닛 머리 위 WidgetComponent]
				if (GeneralWidget) GeneralWidget->SetVisibility(ESlateVisibility::Collapsed);
				if (PlayerWidget) PlayerWidget->SetVisibility(ESlateVisibility::Collapsed);

				AInGameState* GS = GetWorld()->GetGameState<AInGameState>();
				AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>();
				EWildCardPhase CurrentPhase = GS ? GS->GetCurrentPhase() : EWildCardPhase::None;
				bool bHasPriority = GS && PS && (GS->GetCurrentChainPromptID() == PS->GetPlayerId());

				SelectedUnit->ShowActionMenu(CurrentPhase, bHasPriority, TileSelectComp);
			}
			else
			{
				// [기본 모드 A: 2D HUD 투영 방식]
				SelectedUnit->HideActionMenu();

				AInGameState* GS = GetWorld()->GetGameState<AInGameState>();
				AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>();
				EWildCardPhase CurrentPhase = GS ? GS->GetCurrentPhase() : EWildCardPhase::None;
				bool bHasPriority = GS && PS && (GS->GetCurrentChainPromptID() == PS->GetPlayerId());

				if (APlayerUnit* PlayerUnit = Cast<APlayerUnit>(SelectedUnit))
				{
					if (GeneralWidget)
					{
						GeneralWidget->SetVisibility(ESlateVisibility::Collapsed);
					}

					USelectActionWidget* TargetWidget = PlayerWidget ? PlayerWidget : GeneralWidget;
					if (TargetWidget)
					{
						FVector TargetLocation = PlayerUnit->GetActorLocation() + FVector(0.f, 0.f, 140.f);
						TargetWidget->SetupForPlayerUnit(PlayerUnit, CurrentPhase, bHasPriority);
						TargetWidget->SetPositionFromWorldLocation(TargetLocation);
						TargetWidget->SetVisibility(ESlateVisibility::Visible);
					}
				}
				else
				{
					if (PlayerWidget)
					{
						PlayerWidget->SetVisibility(ESlateVisibility::Collapsed);
					}

					if (GeneralWidget)
					{
						FVector TargetLocation = SelectedUnit->GetActorLocation() + FVector(0.f, 0.f, 110.f);
						GeneralWidget->SetupForGeneralUnit(SelectedUnit, CurrentPhase, bHasPriority);
						GeneralWidget->SetPositionFromWorldLocation(TargetLocation);
						GeneralWidget->SetVisibility(ESlateVisibility::Visible);
					}
				}
			}
		}
		else
		{
			if (GeneralWidget) GeneralWidget->SetVisibility(ESlateVisibility::Collapsed);
			if (PlayerWidget) PlayerWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		if (GeneralWidget) GeneralWidget->SetVisibility(ESlateVisibility::Collapsed);
		if (PlayerWidget) PlayerWidget->SetVisibility(ESlateVisibility::Collapsed);

		if (AUnitBase* SelectedUnit = TileSelectComp->GetSelectedUnit())
		{
			SelectedUnit->HideActionMenu();
		}
	}
}

void AInGameController::UpdateActionMenuPosition()
{
	// WidgetComponent 테스트 모드인 경우 컴포넌트가 위치를 자체 관리하므로 PlayerTick 연산 생략
	if (bTestWidgetComponentMenu)
	{
		return;
	}

	if (!TileSelectComp || TileSelectComp->GetCurrentState() != ETileInteractionState::ActionMenuOpen)
	{
		return;
	}

	AUnitBase* SelectedUnit = TileSelectComp->GetSelectedUnit();
	if (!SelectedUnit)
	{
		return;
	}

	USelectActionWidget* TargetWidget = nullptr;
	FVector HeadLocation = SelectedUnit->GetActorLocation();

	if (APlayerUnit* PlayerUnit = Cast<APlayerUnit>(SelectedUnit))
	{
		TargetWidget = PlayerSelectInstance ? PlayerSelectInstance.Get() : (HUDInstance ? HUDInstance->GetPlayerSelectActionWidget() : nullptr);
		HeadLocation += FVector(0.f, 0.f, 140.f); // PlayerUnit 머리 위 높이
	}
	else
	{
		TargetWidget = SelectInstance ? SelectInstance.Get() : (HUDInstance ? HUDInstance->GetSelectActionWidget() : nullptr);
		HeadLocation += FVector(0.f, 0.f, 110.f); // 일반 Unit 카드 상단 높이
	}

	if (TargetWidget && TargetWidget->IsVisible())
	{
		TargetWidget->SetPositionFromWorldLocation(HeadLocation);
	}
}

void AInGameController::FindCanSpawnTiles()
{
	WILD_SIMPLE(Warning);
	AInGameState* GS = Cast<AInGameState>(GetWorld()->GetGameState());
	TArray<TWeakObjectPtr<ATile>> UnitTiles = GS->GetTilesArray();

	TArray<ATile*> CanSpawnTiles = GetPlayerState<AInGamePlayerState>()->MySpawnableTiles;

	TSet<TWeakObjectPtr<ATile>> TileSet(UnitTiles);

	CanSpawnTiles.RemoveAll([&](ATile* Tile) {
		return TileSet.Contains(Tile);
		});

	for (ATile* InTile : CanSpawnTiles)
	{
		InTile->SetTileStateEnum(ETileState::Spawnable);
	}
}

void AInGameController::CancledCanSpawnTiles()
{
	WILD_SIMPLE(Warning);
	AInGameState* GS = Cast<AInGameState>(GetWorld()->GetGameState());
	TArray<TWeakObjectPtr<ATile>> UnitTiles = GS->GetTilesArray();

	if (UnitTiles.IsEmpty())
		WILD_UELOG(Warning, TEXT("Empty"));

	for (TWeakObjectPtr<ATile> T : UnitTiles)
	{
		WILD_UELOG(Warning, TEXT("%s"), *T->GetName());
	}

	TArray<ATile*> CanSpawnTiles = GetPlayerState<AInGamePlayerState>()->MySpawnableTiles;

	TSet<TWeakObjectPtr<ATile>> TileSet(UnitTiles);

	CanSpawnTiles.RemoveAll([&](ATile* Tile) {
		return TileSet.Contains(Tile);
		});

	for (ATile* InTile : CanSpawnTiles)
	{
		InTile->SetTileStateEnum(ETileState::Idle);
	}
}

void AInGameController::FindCanMoveTiles(TWeakObjectPtr<ATile> InTile)
{
	WILD_SIMPLE(Warning);

	if (!InTile.IsValid()) return;

	AInGameState* GS = GetWorld()->GetGameState<AInGameState>();
	if (!GS || !GS->GetMap())
	{
		Server_CalltoFindMoveableTiles(InTile.Get(), this);
		return;
	}

	AUnitBase* SelectedUnit = InTile->GetUnit();
	int32 MaxMoveDistance = SelectedUnit ? SelectedUnit->GetMaxMoveDistance() : 1;

	TArray<ATile*> MoveCandidateTiles = GS->GetMap()->GetTilesWithinDistance(InTile, MaxMoveDistance);
	TArray<ATile*> ValidMoveTiles;

	for (ATile* Tile : MoveCandidateTiles)
	{
		if (IsValid(Tile) && !Tile->HasUnit())
		{
			ValidMoveTiles.Add(Tile);
		}
	}

	Client_ChangeMoveableTiles(ValidMoveTiles);
}

void AInGameController::CancledCanMoveTiles(TWeakObjectPtr<ATile> InTile)
{
	WILD_SIMPLE(Warning);

	TArray<ATile*> AT = GetPlayerState<AInGamePlayerState>()->GetOldTiles();

	for (ATile* Tile : AT)
	{
		WILD_SIMPLE(Warning);
		Tile->SetTileStateEnum(ETileState::Idle);
	}

	GetPlayerState<AInGamePlayerState>()->ClearOldTiles();

	if (HUDInstance)
	{
		HUDInstance->HideTargetingFocus();
	}
}

void AInGameController::FindCanCombatTiles(TWeakObjectPtr<ATile> InTile)
{
	WILD_SIMPLE(Warning);

	if (!InTile.IsValid()) return;

	AInGameState* GS = GetWorld()->GetGameState<AInGameState>();
	AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>();
	if (!GS || !GS->GetMap() || !PS)
	{
		Server_CalltoFindCombatableTiles(InTile.Get(), this);
		return;
	}

	int32 MyPlayerID = PS->GetPlayerId();
	TArray<ATile*> AdjacentTiles = GS->GetMap()->GetTilesAtDistance(InTile, 1);
	TArray<ATile*> ValidCombatTiles;

	for (ATile* Tile : AdjacentTiles)
	{
		if (IsValid(Tile) && Tile->HasUnit())
		{
			AUnitBase* TargetUnit = Tile->GetUnit();
			if (IsValid(TargetUnit) && TargetUnit->GetOwnerId() != MyPlayerID)
			{
				ValidCombatTiles.Add(Tile);
			}
		}
	}

	Client_ChangeCombatableTiles(ValidCombatTiles);
}

void AInGameController::CancledCanCombatTiles(TWeakObjectPtr<ATile> InTile)
{
	WILD_SIMPLE(Warning);

	TArray<ATile*> AT = GetPlayerState<AInGamePlayerState>()->GetOldTiles();

	for (ATile* Tile : AT)
	{
		WILD_SIMPLE(Warning);
		Tile->SetTileStateEnum(ETileState::Idle);
	}

	GetPlayerState<AInGamePlayerState>()->ClearOldTiles();

	if (HUDInstance)
	{
		HUDInstance->HideTargetingFocus();
	}
}

void AInGameController::FindCanEffectTiles(TWeakObjectPtr<ATile> InTile)
{
	WILD_SIMPLE(Warning);

	if (!InTile.IsValid() || !InTile->GetUnit()) return;

	AInGameState* GS = GetWorld()->GetGameState<AInGameState>();
	AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>();
	if (!GS || !GS->GetMap() || !PS)
	{
		Server_CalltoFindEffectableTiles(InTile.Get(), this);
		return;
	}

	TArray<ATile*> CanEffectableTiles;
	const FCardData& CardData = InTile->GetUnit()->GetCardData();

	switch (CardData.EffectData.Type)
	{
	case EEffectType::DRAW:
		break;
	case EEffectType::MOVE:
	{
		TArray<ATile*> AdjacentTiles = GS->GetMap()->GetTilesAtDistance(InTile, 1);
		for (ATile* Tile : AdjacentTiles)
		{
			if (IsValid(Tile) && !Tile->HasUnit())
			{
				CanEffectableTiles.Add(Tile);
			}
		}
		break;
	}
	default:
	{
		int32 MyPlayerID = PS->GetPlayerId();
		TArray<ATile*> AdjacentTiles = GS->GetMap()->GetTilesAtDistance(InTile, 1);
		for (ATile* Tile : AdjacentTiles)
		{
			if (IsValid(Tile) && Tile->HasUnit() && Tile->GetUnit()->GetOwnerId() != MyPlayerID)
			{
				CanEffectableTiles.Add(Tile);
			}
		}
		break;
	}
	}

	Client_ChangeEffectableTiles(CanEffectableTiles);
}

void AInGameController::CancledCanEffectTiles(TWeakObjectPtr<ATile> InTile)
{
	WILD_SIMPLE(Warning);

	TArray<ATile*> AT = GetPlayerState<AInGamePlayerState>()->GetOldTiles();

	for (ATile* Tile : AT)
	{
		WILD_SIMPLE(Warning);
		Tile->SetTileStateEnum(ETileState::Idle);
	}

	GetPlayerState<AInGamePlayerState>()->ClearOldTiles();

	if (HUDInstance)
	{
		HUDInstance->HideTargetingFocus();
	}
}

void AInGameController::OnTileSelected()
{
	if (IsInTacticalSkillCardSelection())
	{
		CancelTacticalSkillCardSelection();
		return;
	}

	// 카드 외 화면(월드 타일/배경)을 클릭했을 때 손패가 펼쳐져 있다면 안쪽으로 넣음
	if (HUDInstance && HUDInstance->GetHandWidget() && HUDInstance->GetHandWidget()->IsHandExpanded())
	{
		HUDInstance->GetHandWidget()->SetHandExpanded(false);
	}

	//TODO : CurrentState()에 따라 처리 분기 필요
	WILD_SIMPLE(Warning);
	WILD_CHECK(IsValid(TileSelectComp))
	float X = 0.0f;
	float Y = 0.0f;
	if (!GetMousePosition(X, Y))
	{
		return;
	}

	FVector WorldPosition;
	FVector WorldDirection;
	if (!DeprojectScreenPositionToWorld(X, Y, WorldPosition, WorldDirection))
	{
		return;
	}

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetPawn());

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		WorldPosition,
		WorldPosition + WorldDirection * 10000.0f,
		ECC_MouseClick,
		Params
	);

	// [범용 타일 타겟팅 활성 상태] 처리
	if (bIsTileTargetingActive)
	{
		AActor* HitActor = (bHit && HitResult.GetActor()) ? HitResult.GetActor() : nullptr;
		ATile* ClickedTarget = Cast<ATile>(HitActor);
		if (!ClickedTarget && HitActor)
		{
			if (AUnitBase* HitUnit = Cast<AUnitBase>(HitActor))
			{
				ClickedTarget = HitUnit->GetCurrentTile();
			}
		}
		if (!ClickedTarget && CurrentHoveredTile.IsValid())
		{
			ClickedTarget = CurrentHoveredTile.Get();
		}

		if (ClickedTarget)
		{
			bool bIsCandidate = false;
			for (TWeakObjectPtr<ATile> CandidatePtr : ActiveTargetingTiles)
			{
				if (CandidatePtr.Get() == ClickedTarget)
				{
					bIsCandidate = true;
					break;
				}
			}

			if (bIsCandidate)
			{
				FOnTileTargetSelected ConfirmCallback = ActiveTileSelectedCallback;

				// 하이라이트 원복 및 HUD 포커스 해제
				for (TWeakObjectPtr<ATile> TilePtr : ActiveTargetingTiles)
				{
					if (TilePtr.IsValid())
					{
						TilePtr->SetTileStateEnum(ETileState::Idle);
					}
				}
				ActiveTargetingTiles.Empty();
				bIsTileTargetingActive = false;

				if (HUDInstance)
				{
					HUDInstance->HideTargetingFocus();
				}

				ActiveTileSelectedCallback.Unbind();
				ActiveTileCancelledCallback.Unbind();

				ConfirmCallback.ExecuteIfBound(ClickedTarget);
				return;
			}
		}

		// 비유효 타일 또는 허공 클릭 시 타겟팅 취소
		CancelTileTargeting();
		return;
	}

	if (bHit && HitResult.GetActor())
	{
		AActor* HitActor = HitResult.GetActor();
		ATile* ClickedTile = Cast<ATile>(HitActor);

		// 혹시라도 유닛(일반 유닛 또는 PlayerUnit)이 히트된 경우, 해당 유닛이 위치한 타일로 변환
		if (!ClickedTile)
		{
			if (AUnitBase* HitUnit = Cast<AUnitBase>(HitActor))
			{
				ClickedTile = HitUnit->GetCurrentTile();
			}
		}

		// 2차 폴백: 현재 마우스 호버링 중인 타일이 있다면 우선 사용
		if (!ClickedTile && CurrentHoveredTile.IsValid())
		{
			ClickedTile = CurrentHoveredTile.Get();
		}

		if (ClickedTile)
		{
			WILD_SIMPLE(Warning);
			WILD_UELOG(Warning, TEXT("Tile Name : %s"), *ClickedTile->GetName());

			if ((ClickedTile->GetUnit() != nullptr) &&
				(ClickedTile->GetUnit()->GetOwnerId()) != (GetPlayerState<AInGamePlayerState>()->GetPlayerId()) &&
				TileSelectComp->GetCurrentState() != ETileInteractionState::AwaitingAttackTarget)
			{
				if (TileSelectComp->GetCurrentState() == ETileInteractionState::ActionMenuOpen)
				{
					TileSelectComp->CancelAction();
				}
				WILD_UELOG(Warning, TEXT("Not Combating now And Not Your Unit. So Cancle it!"));
				return;
			}

			TileSelectComp->ProcessTileClick(ClickedTile);
		}
		else
		{
			// 타일 및 유닛 외의 인터랙션 액터(재미 요소 등) 클릭 처리
			if (TileSelectComp->GetCurrentState() == ETileInteractionState::ActionMenuOpen)
			{
				TileSelectComp->CancelAction();
			}

			if (HitActor->Implements<UWildInteractableInterface>())
			{
				IWildInteractableInterface::Execute_OnInteract(HitActor, this);
			}
			HitActor->NotifyActorOnClicked(EKeys::LeftMouseButton);
		}
	}
	else
	{
		// 트레이스가 빗나갔더라도 현재 호버 중인 타일이 유효하다면 즉시 타일 클릭 처리
		if (CurrentHoveredTile.IsValid())
		{
			ATile* HoveredTile = CurrentHoveredTile.Get();
			if ((HoveredTile->GetUnit() != nullptr) &&
				(HoveredTile->GetUnit()->GetOwnerId()) != (GetPlayerState<AInGamePlayerState>()->GetPlayerId()) &&
				TileSelectComp->GetCurrentState() != ETileInteractionState::AwaitingAttackTarget)
			{
				if (TileSelectComp->GetCurrentState() == ETileInteractionState::ActionMenuOpen)
				{
					TileSelectComp->CancelAction();
				}
				return;
			}

			TileSelectComp->ProcessTileClick(HoveredTile);
			return;
		}

		// 빈 공간(허공) 클릭 시 열려 있던 액션 메뉴 취소
		if (TileSelectComp->GetCurrentState() == ETileInteractionState::ActionMenuOpen)
		{
			TileSelectComp->CancelAction();
		}
	}
}

void AInGameController::ConnectedInHUD()
{
	if (HUDInstance == nullptr)
	{
		WILD_UELOG(Warning, TEXT("HUDInstance is nullptr. Cannot connect delegates."));
		return;
	}

	SelectInstance = HUDInstance->GetSelectActionWidget();
	if (SelectInstance)
	{
		SelectInstance->OnMoveButtonPressed.RemoveDynamic(TileSelectComp, &UTileSelectComponent::SelectMoveAction);
		SelectInstance->OnMoveButtonPressed.AddDynamic(TileSelectComp, &UTileSelectComponent::SelectMoveAction);

		SelectInstance->OnCombatButtonPressed.RemoveDynamic(TileSelectComp, &UTileSelectComponent::SelectAttackAction);
		SelectInstance->OnCombatButtonPressed.AddDynamic(TileSelectComp, &UTileSelectComponent::SelectAttackAction);

		SelectInstance->OnEffectButtonPressed.RemoveDynamic(TileSelectComp, &UTileSelectComponent::SelectEffectAction);
		SelectInstance->OnEffectButtonPressed.AddDynamic(TileSelectComp, &UTileSelectComponent::SelectEffectAction);

		SelectInstance->OnTacticalSkillButtonPressed.RemoveDynamic(this, &AInGameController::OnTacticalSkillActionSelected);
		SelectInstance->OnTacticalSkillButtonPressed.AddDynamic(this, &AInGameController::OnTacticalSkillActionSelected);
	}

	PlayerSelectInstance = HUDInstance->GetPlayerSelectActionWidget();
	if (PlayerSelectInstance)
	{
		PlayerSelectInstance->OnMoveButtonPressed.RemoveDynamic(TileSelectComp, &UTileSelectComponent::SelectMoveAction);
		PlayerSelectInstance->OnMoveButtonPressed.AddDynamic(TileSelectComp, &UTileSelectComponent::SelectMoveAction);

		PlayerSelectInstance->OnTacticalSkillButtonPressed.RemoveDynamic(this, &AInGameController::OnTacticalSkillActionSelected);
		PlayerSelectInstance->OnTacticalSkillButtonPressed.AddDynamic(this, &AInGameController::OnTacticalSkillActionSelected);
	}
}



void AInGameController::Server_CalltoFindMoveableTiles_Implementation(ATile* InTile, AInGameController* InPlayer)
{
	AInGameState* GS = Cast<AInGameState>(GetWorld()->GetGameState());
	GS->Server_FindCanMoveUnitTiles(InTile, this);
}

bool AInGameController::Server_CalltoFindMoveableTiles_Validate(ATile* InTile, AInGameController* InPlayer)
{
	return true;
}

void AInGameController::Server_CalltoFindCombatableTiles_Implementation(ATile* InTile, AInGameController* InPlayer)
{
	AInGameState* GS = Cast<AInGameState>(GetWorld()->GetGameState());
	GS->Server_FindCanCombatUnitTiles(InTile, this);
}

bool AInGameController::Server_CalltoFindCombatableTiles_Validate(ATile* InTile, AInGameController* InPlayer)
{
	return true;
}

void AInGameController::Server_CalltoFindEffectableTiles_Implementation(ATile* InTile, AInGameController* InPlayer)
{
	AInGameState* GS = Cast<AInGameState>(GetWorld()->GetGameState());
	GS->Server_FindCanEffectableUnitTiles(InTile, this);
}

bool AInGameController::Server_CalltoFindEffectableTiles_Validate(ATile* InTile, AInGameController* InPlayer)
{
	return true;
}

void AInGameController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AInGameController::OnTileSelected);
		InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AInGameController::OnCancelButtonPressed);
	}
}



void AInGameController::Client_ChangeEffectableTiles_Implementation(const TArray<ATile*>& InTiles)
{
	WILD_SIMPLE(Warning);
	if (InTiles.Num() == 0)
	{
		//주변에 싸울 적이 없음
		//로그 출력 및 component에 전달
		WILD_UELOG(Warning, TEXT("Can't fight. No Enemy"));
		TileSelectComp->SetCurrentState(ETileInteractionState::Idle);
		if (HUDInstance)
		{
			HUDInstance->HideTargetingFocus();
		}
	}
	else
	{
		GetPlayerState<AInGamePlayerState>()->SetOldTiles(InTiles);
		for (ATile* Tile : InTiles)
		{
			Tile->SetTileStateEnum(ETileState::Effectable);
		}
		if (HUDInstance)
		{
			HUDInstance->ShowTargetingFocus(FText::FromString(TEXT("효과를 적용할 대상 타일을 선택하세요")));
		}
	}
}

void AInGameController::Client_ChangeCombatableTiles_Implementation(const TArray<ATile*>& InTiles)
{
	WILD_SIMPLE(Warning);
	if (InTiles.Num() == 0)
	{
		//주변에 싸울 적이 없음
		//로그 출력 및 component에 전달
		WILD_UELOG(Warning, TEXT("Can't fight. No Enemy"));
		TileSelectComp->SetCurrentState(ETileInteractionState::Idle);
		if (HUDInstance)
		{
			HUDInstance->HideTargetingFocus();
		}
	}
	else
	{
		GetPlayerState<AInGamePlayerState>()->SetOldTiles(InTiles);
		for (ATile* Tile : InTiles)
		{
			Tile->SetTileStateEnum(ETileState::Combatable);
		}
		if (HUDInstance)
		{
			HUDInstance->ShowTargetingFocus(FText::FromString(TEXT("공격할 적 유닛 타일을 선택하세요")));
		}
	}
}

void AInGameController::Client_ChangeMoveableTiles_Implementation(const TArray<ATile*>& InTiles)
{
	WILD_SIMPLE(Warning);
	if (InTiles.Num() == 0)
	{
		//주변에 움직일 곳이 없음
		//로그 출력 및 component에 전달
		WILD_UELOG(Warning, TEXT("Can't Move. No Tile"));
		TileSelectComp->SetCurrentState(ETileInteractionState::Idle);
		if (HUDInstance)
		{
			HUDInstance->HideTargetingFocus();
		}
	}
	else
	{
		GetPlayerState<AInGamePlayerState>()->SetOldTiles(InTiles);
		for (ATile* Tile : InTiles)
		{
			Tile->SetTileStateEnum(ETileState::Moveable);
		}
		if (HUDInstance)
		{
			HUDInstance->ShowTargetingFocus(FText::FromString(TEXT("이동할 타일을 선택하세요")));
		}
	}
	
}

void AInGameController::FindCanTacticalSkillTiles(TWeakObjectPtr<ATile> InTile)
{
	if (InTile.IsValid())
	{
		APlayerUnit* PlayerUnit = Cast<APlayerUnit>(InTile->GetUnit());
		if (PlayerUnit && PlayerUnit->GetTacticalSkill())
		{
			TArray<ATile*> TargetTiles;
			PlayerUnit->GetTacticalSkill()->GetValidTargetTiles(PlayerUnit, TargetTiles);
			Client_ChangeTacticalSkillTiles(TargetTiles);
		}
	}
}

void AInGameController::CancledCanTacticalSkillTiles(TWeakObjectPtr<ATile> InTile)
{
	TArray<ATile*> AT = GetPlayerState<AInGamePlayerState>()->GetOldTiles();
	for (ATile* Tile : AT)
	{
		if (Tile)
		{
			Tile->SetTileStateEnum(ETileState::Idle);
		}
	}
	GetPlayerState<AInGamePlayerState>()->ClearOldTiles();

	if (HUDInstance)
	{
		HUDInstance->HideTargetingFocus();
	}
}



void AInGameController::Client_ChangeTacticalSkillTiles_Implementation(const TArray<ATile*>& InTiles)
{
	if (InTiles.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Client] 유효한 전술 스킬 대상 타일이 없습니다."));
		TileSelectComp->SetCurrentState(ETileInteractionState::Idle);
		if (HUDInstance)
		{
			HUDInstance->HideTargetingFocus();
		}
	}
	else
	{
		GetPlayerState<AInGamePlayerState>()->SetOldTiles(InTiles);
		for (ATile* Tile : InTiles)
		{
			if (Tile)
			{
				Tile->SetTileStateEnum(ETileState::Combatable);
			}
		}
		if (HUDInstance)
		{
			HUDInstance->ShowTargetingFocus(FText::FromString(TEXT("전술 스킬을 적용할 대상 타일을 선택하세요")));
		}
	}
}
//////////////////////////////////////////////////////

void AInGameController::Server_CompleteGameset_Implementation()
{
	if (AInGameMode* GameMode = Cast<AInGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GameMode->PhaseComponent->ProcessPlayerGamesetReady(this); // this(자신)를 넘겨줌!
	}
}

bool AInGameController::Server_CompleteGameset_Validate()
{
	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (GameState && GameState->GetCurrentMatchState() != EWildMatchState::Gameset)
	{
		return false;
	}
	return true;
}

void AInGameController::OnMatchEnded(int32 WinnerPlayerID, int32 DefeatedPlayerID)
{
	AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>();
	if (!PS) return;

	if (PS->GetPlayerId() == WinnerPlayerID)
	{
		ShowTimingNotice(EWildTimingNoticeType::Victory, FText::FromString(TEXT("승리!")));
	}
	else if (WinnerPlayerID != -1)
	{
		ShowTimingNotice(EWildTimingNoticeType::Defeat, FText::FromString(TEXT("패배...")));
	}
}

void AInGameController::ShowTimingNotice(EWildTimingNoticeType NoticeType, const FText& CustomMessage, float InDuration)
{
	if (!IsLocalController() || NoticeType == EWildTimingNoticeType::None)
	{
		return;
	}

	if (UWildHUDWidget* HUD = GetOrCreateHUD())
	{
		HUD->ShowTimingNotice(NoticeType, CustomMessage, InDuration);
	}
}

void AInGameController::HideTimingNotice()
{
	if (HUDInstance)
	{
		HUDInstance->HideCurrentNotice();
	}
}

void AInGameController::OnTacticalSkillActionSelected()
{
	APlayerUnit* PlayerUnit = TileSelectComp ? TileSelectComp->GetSelectedPlayerUnit() : nullptr;
	if (!PlayerUnit)
	{
		if (AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>())
		{
			PlayerUnit = PS->GetPlayerUnit();
		}
	}

	if (!PlayerUnit)
	{
		return;
	}

	UTacticalSkillBase* Skill = PlayerUnit->GetTacticalSkill();
	if (!Skill)
	{
		return;
	}

	Skill->InitiateSkill(this, PlayerUnit);
}

void AInGameController::RequestCardSelection(const FWildCardSelectionContext& Context)
{
	if (!IsLocalController()) return;

	CancelTileAction();

	if (HUDInstance)
	{
		HUDInstance->RequestCardSelection(Context);
	}
}

void AInGameController::CancelCardSelection()
{
	if (HUDInstance)
	{
		HUDInstance->CloseCardSelection();
	}
}

bool AInGameController::IsInCardSelection() const
{
	return HUDInstance && HUDInstance->IsCardSelectionActive();
}

void AInGameController::StartTacticalSkillCardSelection(APlayerUnit* SourceUnit, UTacticalSkillBase* Skill)
{
	if (!IsLocalController()) return;

	PendingTacticalSkillUnit = SourceUnit;
	PendingTacticalSkill = Skill;

	CancelTileAction();

	if (HUDInstance)
	{
		HUDInstance->ShowTacticalSkillCardSelection(Skill);
	}
}

void AInGameController::CancelTacticalSkillCardSelection()
{
	PendingTacticalSkillUnit = nullptr;
	PendingTacticalSkill = nullptr;

	if (HUDInstance)
	{
		HUDInstance->HideTacticalSkillCardSelection();
	}
}

void AInGameController::ConfirmTacticalSkillCardSelection(int32 SelectedCardInstanceID)
{
	if (!IsLocalController()) return;

	Server_ConfirmSearchLibrarySelection(SelectedCardInstanceID);
	CancelTacticalSkillCardSelection();
}



void AInGameController::Client_PromptSearchLibrarySelection_Implementation()
{
	if (!IsLocalController()) return;

	FWildCardSelectionContext Context;
	Context.PromptText = FText::FromString(TEXT("덱으로 되돌릴 카드를 1장 선택하세요"));
	Context.ConfirmButtonText = FText::FromString(TEXT("확인"));
	Context.MinSelectionCount = 1;
	Context.MaxSelectionCount = 1;
	Context.bAllowCancel = false;

	TWeakObjectPtr<AInGameController> WeakThis = this;
	Context.OnConfirmedDelegate.BindLambda([WeakThis](const TArray<int32>& SelectedIDs)
	{
		if (WeakThis.IsValid() && SelectedIDs.Num() > 0)
		{
			WeakThis->Server_ConfirmSearchLibrarySelection(SelectedIDs[0]);
		}
	});

	Context.OnCancelledDelegate.BindLambda([WeakThis]()
	{
		if (WeakThis.IsValid())
		{
			WeakThis->Server_ConfirmSearchLibrarySelection(-1);
		}
	});

	RequestCardSelection(Context);
}



void AInGameController::Client_CancelCardSelection_Implementation()
{
	CancelCardSelection();
}

bool AInGameController::Server_ConfirmSearchLibrarySelection_Validate(int32 SelectedCardInstanceID)
{
	return true;
}

void AInGameController::Server_ConfirmSearchLibrarySelection_Implementation(int32 SelectedCardInstanceID)
{
	AInGameState* GS = GetWorld() ? GetWorld()->GetGameState<AInGameState>() : nullptr;
	if (!GS) return;

	UChainComponent* ChainComp = GS->FindComponentByClass<UChainComponent>();
	if (!ChainComp) return;

	AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>();
	int32 CallerID = PS ? PS->GetPlayerId() : -1;

	ChainComp->OnSearchLibrarySelectionConfirmed(CallerID, SelectedCardInstanceID);
}

bool AInGameController::IsInTacticalSkillCardSelection() const
{
	return IsInCardSelection();
}

void AInGameController::StartTileTargeting(
	const TArray<ATile*>& InCandidateTiles,
	const FText& InPromptText,
	int32 InTileHighlightState,
	FOnTileTargetSelected OnSelected,
	FOnTileTargetCancelled OnCancelled)
{
	if (!IsLocalController()) return;

	// 기존 타겟팅 및 타일 액션 취소
	CancelTileTargeting();
	CancelTileAction();

	bIsTileTargetingActive = true;
	ActiveTileSelectedCallback = OnSelected;
	ActiveTileCancelledCallback = OnCancelled;
	ActiveTargetingTiles.Empty();

	for (ATile* Tile : InCandidateTiles)
	{
		if (IsValid(Tile))
		{
			ActiveTargetingTiles.Add(Tile);
			Tile->SetTileState(InTileHighlightState);
		}
	}

	if (HUDInstance)
	{
		HUDInstance->ShowTargetingFocus(InPromptText);
	}
}

void AInGameController::CancelTileTargeting()
{
	if (!bIsTileTargetingActive) return;

	bIsTileTargetingActive = false;

	for (TWeakObjectPtr<ATile> TilePtr : ActiveTargetingTiles)
	{
		if (TilePtr.IsValid())
		{
			TilePtr->SetTileStateEnum(ETileState::Idle);
		}
	}
	ActiveTargetingTiles.Empty();

	if (HUDInstance)
	{
		HUDInstance->HideTargetingFocus();
	}

	FOnTileTargetCancelled CancelCallback = ActiveTileCancelledCallback;
	ActiveTileSelectedCallback.Unbind();
	ActiveTileCancelledCallback.Unbind();
	CancelCallback.ExecuteIfBound();
}

void AInGameController::OnCancelButtonPressed()
{
	if (bIsTileTargetingActive)
	{
		CancelTileTargeting();
		return;
	}

	CancelTileAction();
}
