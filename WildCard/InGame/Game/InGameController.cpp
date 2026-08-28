// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Game/InGameController.h"
#include "InGameMode.h"
#include "PhaseComponent.h"
#include "InGameState.h"
#include "InGamePlayerState.h"
#include "View/MVVMView.h"
#include "Blueprint/UserWidget.h"
#include "WildCardGameInstance.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "WildHUDWidget.h"
#include "ArtifactSelectionViewModel.h"
#include "MulliganViewModel.h"
#include "WildHUDViewModel.h"
#include "WildCardViewModel.h"

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
	PassAction.ActionType = EWildActionType::None;
	Server_SubmitAction(PassAction);
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
				UE_LOG(LogTemp, Error, TEXT("[Client] ViewModel 연결 실패! UMG 에디터(ViewModels 패널)의 이름이 'WildProtoMulliganViewModel'인지 확인하세요."));
			}
		}
	}
	
}

void AInGameController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		if (AInGameState* GameState = GetWorld()->GetGameState<AInGameState>())
		{
			GameState->OnMatchStateChangedDelegate.AddDynamic(this, &AInGameController::OnMatchStateChanged);
			GameState->OnPhaseChangedDelegate.AddDynamic(this, &AInGameController::OnGamePhaseChanged);
			GameState->OnTimerChangedDelegate.AddDynamic(this, &AInGameController::OnTimerUpdated);
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
	switch (NewState)
	{
	case EWildMatchState::Gameset:
		ShowGamesetUI();
		UE_LOG(LogTemp, Log, TEXT("[Client] 매치 상태: 성유물 선택(Gameset) 시작"));
		break;
	case EWildMatchState::Mulligan:
		// 멀리건 상태 진입 시 UI 표시
		HideGamesetUI();
		ShowMulliganUI();
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
	// 이제 여기서는 Mulligan 관련 로직을 제거하고 턴 내부 공방만 신경 씁니다.
	switch (NewPhase)
	{
	case EWildCardPhase::Turn_Start:
		UE_LOG(LogWildPhase, Log, TEXT("[Client] 턴 시작 페이즈"));
		break;

	case EWildCardPhase::Action_Private:
		// 메인 행동 선택 UI 활성화 등
		UE_LOG(LogWildPhase, Log, TEXT("[Client] 행동 선택 단계"));
		break;

	case EWildCardPhase::Action_Public:
		// 체인 대응 버튼 활성화 등
		UE_LOG(LogWildPhase, Log, TEXT("[Client] 체인 공방 단계"));
		break;

	case EWildCardPhase::Action_Resolution:
		// 체인 처리 애니메이션 집중 모드
		UE_LOG(LogWildPhase, Log, TEXT("[Client] 효과 처리 중..."));
		break;
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

void AInGameController::AttemptPlayCard(UWildCardViewModel* CardViewModel, FVector2D ScreenPosition)
{
	// 1. 유효성 검사
	if (!CardViewModel || !IsLocalController()) return;

	FVector WorldOrigin;
	FVector WorldDirection;

	// 2. 2D 화면 좌표를 3D 공간의 시작점과 방향(Ray)으로 변환
	if (DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, WorldOrigin, WorldDirection))
	{
		FHitResult HitResult;
		FVector TraceEnd = WorldOrigin + (WorldDirection * 10000.0f); 

		// 3. 월드 레이캐스트 수행 (바닥이나 타일 충돌 감지)
		if (GetWorld()->LineTraceSingleByChannel(HitResult, WorldOrigin, TraceEnd, ECC_Visibility))
		{
			// (디버그용 시각화) 마우스가 닿은 3D 위치에 구체를 그립니다.
			DrawDebugSphere(GetWorld(), HitResult.Location, 50.0f, 12, FColor::Green, false, 2.0f);

			// 4. 서버로 보낼 행동 구조체(FWildActionData) 조립
			FWildActionData ActionData;
			
			// PlayerID 세팅[cite: 3]
			if (AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>())
			{
				ActionData.PlayerID = PS->GetPlayerId();
			}
			
			// 뷰모델에서 카드의 고유 InstanceID를 가져온다고 가정 (로직에 맞게 수정 필요)
			ActionData.CardInstanceID = CardViewModel->InstanceID;
			
			// 레이캐스트로 맞은 타일/바닥의 위치를 목표 좌표로 설정[cite: 8]
			//ActionData.TargetGridPos = HitResult.Location;

			// 5. 현재 게임의 페이즈 상태를 확인하여 알맞은 RPC 호출[cite: 5, 6]
			AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
			if (GameState)
			{
				EWildCardPhase CurrentPhase = GameState->GetCurrentPhase();

				if (CurrentPhase == EWildCardPhase::Action_Private)
				{
					// 비공개 메인 행동 결정 단계: 유닛 소환 등의 메인 액션 전송
					ActionData.ActionCategory = EWildActionCategory::Main;
					ActionData.ActionType = EWildActionType::SummonUnit; // (뷰모델 데이터에 따라 동적 할당 필요)[cite: 8]
					
					UE_LOG(LogWildPhase, Log, TEXT("[Client] 메인 행동(Action_Private)으로 카드 사용 시도"));
					CompletePrivateAction(ActionData);
				}
				else if (CurrentPhase == EWildCardPhase::Action_Public)
				{
					// 공개 체인 공방 단계: 스펠 등의 서브 액션 전송[cite: 7, 8]
					ActionData.ActionCategory = EWildActionCategory::Sub;
					ActionData.ActionType = EWildActionType::CastSpell; // (뷰모델 데이터에 따라 동적 할당 필요)[cite: 8]
					
					UE_LOG(LogWildPhase, Log, TEXT("[Client] 체인 서브 행동(Action_Public)으로 카드 사용 시도"));
					PlayChainCard(ActionData);
				}
				else
				{
					// 카드를 낼 수 없는 페이즈 (턴 시작, 처리 중, 게임 셋 등)
					UE_LOG(LogWildPhase, Warning, TEXT("[Client] 현재 카드 조작이 불가능한 페이즈(%s)입니다."), *UEnum::GetValueAsString(CurrentPhase));
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("레이캐스트 실패: 허공에 카드를 놓았습니다."));
		}
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
		
		SetInputMode(FInputModeGameOnly());
	}

	CreateHUD();
}

void AInGameController::CreateHUD()
{
	
	if (HUDClass == nullptr || HUDInstance != nullptr) return;

	HUDInstance = CreateWidget<UWildHUDWidget>(GetWorld(), HUDClass);
	if (HUDInstance == nullptr) return;

	MainHUDViewModel = NewObject<UWildHUDViewModel>(this);
	HUDInstance->AddToViewport();
	
	// [추가됨] 1단계: 생성된 HUD에 뷰모델 주입
	HUDInstance->InitializeHUD(MainHUDViewModel);
	
	if (AInGamePlayerState* PS = GetPlayerState<AInGamePlayerState>())
	{
		// [추가됨] 1단계: 게임 중 카드를 뽑거나 사용할 때 UI를 갱신하기 위해 이벤트 바인딩
		(PS->CardComponent)->OnMyHandUpdatedDelegate.AddDynamic(this, &AInGameController::UpdateHandFromState);

		if ((PS->CardComponent)->CurrentHandDisplay.Num() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Client] HUD 생성 완료. 즉시 동기화합니다."));
			
			// [수정됨] 1단계: 기존 중복 로직 대신 통합된 업데이트 함수 직접 호출
			UpdateHandFromState((PS->CardComponent)->CurrentHandDisplay); 
		}
	}

	if (AInGameState* GameState = GetWorld()->GetGameState<AInGameState>())
	{
		OnTimerUpdated(GameState->GetPhaseRemainTime());
	}

	GetWorld()->GetTimerManager().SetTimer(EnemyStateBindTimer, this, &AInGameController::TryBindEnemyState, 0.5f, true);
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

void AInGameController::OnTimerUpdated(int32 RemainTime)
{
	/*
	if (HUDInstance && HUDInstance->GetTimer())
	{
		FText TimeText = FText::AsNumber(RemainTime);
		HUDInstance->GetTimer()->SetTimerText(TimeText);
	}
	*/
	
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
