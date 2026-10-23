// Fill out your copyright notice in the Description page of Project Settings.

#include "WildDeckSelectPlayerController.h"
#include "WildDeckSelectGameMode.h"
#include "WildDeckSubsystem.h"
#include "WildAuthSubsystem.h"
#include "WildCardGameInstance.h"
#include "WildMatchmakingSubsystem.h"
#include "SWildMatchmakingDialogWidget.h"
#include "Deck/UI/SWildDeckSelectFallbackWidget.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"

AWildDeckSelectPlayerController::AWildDeckSelectPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AWildDeckSelectPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// UI 및 게임 입력 모드 설정
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	// 기본 유저 확인 및 스타터 덱 시딩
	const FString Username = GetActiveUsername();
	if (UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(GetGameInstance()))
	{
		if (UWildDeckSubsystem* DeckSub = GI->GetSubsystem<UWildDeckSubsystem>())
		{
			DeckSub->EnsureStarterDecks(Username);

			// L_DeckSelect Scene 기본 선택: 마법사 덱을 우선적으로 기본 선택
			TArray<FWildDeckData> Decks = DeckSub->GetUserDecks(Username);
			int32 DefaultDeckIdx = INDEX_NONE;
			for (int32 i = 0; i < Decks.Num(); ++i)
			{
				if (Decks[i].DeckClass == EWildPlayerClass::Mage || Decks[i].DeckName.Contains(TEXT("마법사")))
				{
					DefaultDeckIdx = i;
					break;
				}
			}

			if (DefaultDeckIdx != INDEX_NONE)
			{
				DeckSub->SetSelectedDeck(Decks[DefaultDeckIdx]);
			}
			else if (!DeckSub->HasSelectedDeck() && Decks.Num() > 0)
			{
				DeckSub->SetSelectedDeck(Decks[0]);
			}
		}

		// 매칭 서브시스템 이벤트 바인딩
		if (UWildMatchmakingSubsystem* MatchSub = GI->GetSubsystem<UWildMatchmakingSubsystem>())
		{
			MatchSub->OnStateChanged.AddDynamic(this, &AWildDeckSelectPlayerController::HandleMatchmakingStateChanged);
			MatchSub->OnTimerTick.AddDynamic(this, &AWildDeckSelectPlayerController::HandleMatchmakingTimerTick);
			MatchSub->OnMatchFound.AddDynamic(this, &AWildDeckSelectPlayerController::HandleMatchFound);
		}
	}

	// UMG 위젯 클래스가 지정되어 있으면 UMG 위젯 생성
	if (DeckSelectWidgetClass)
	{
		DeckSelectWidgetInstance = CreateWidget<UUserWidget>(this, DeckSelectWidgetClass);
		if (DeckSelectWidgetInstance)
		{
			DeckSelectWidgetInstance->AddToViewport(0);
			UE_LOG(LogTemp, Log, TEXT("[WildDeckSelect] DeckSelectWidgetClass(UMG) 인스턴스 생성 완료"));
			return;
		}
	}

	// UMG가 없으면 폴백 Slate 위젯 표시
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (LocalPlayer->ViewportClient)
		{
			TSharedRef<SWildDeckSelectFallbackWidget> FallbackWidget =
				SNew(SWildDeckSelectFallbackWidget)
				.PlayerController(this);

			FallbackDeckSelectWidget = FallbackWidget;
			FallbackDeckSelectWidgetRef = FallbackWidget;

			LocalPlayer->ViewportClient->AddViewportWidgetContent(FallbackWidget, 100);
			UE_LOG(LogTemp, Log, TEXT("[WildDeckSelect] UMG 미지정으로 인해 폴백 Slate 위젯을 뷰포트에 등록했습니다."));
		}
	}
}

void AWildDeckSelectPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TravelTimerHandle);
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWildMatchmakingSubsystem* MatchSub = GI->GetSubsystem<UWildMatchmakingSubsystem>())
		{
			MatchSub->OnStateChanged.RemoveDynamic(this, &AWildDeckSelectPlayerController::HandleMatchmakingStateChanged);
			MatchSub->OnTimerTick.RemoveDynamic(this, &AWildDeckSelectPlayerController::HandleMatchmakingTimerTick);
			MatchSub->OnMatchFound.RemoveDynamic(this, &AWildDeckSelectPlayerController::HandleMatchFound);
		}
	}

	HideMatchmakingUI();

	if (FallbackDeckSelectWidget.IsValid())
	{
		if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
		{
			if (LocalPlayer->ViewportClient)
			{
				LocalPlayer->ViewportClient->RemoveViewportWidgetContent(FallbackDeckSelectWidget.ToSharedRef());
			}
		}
		FallbackDeckSelectWidget.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void AWildDeckSelectPlayerController::SelectDeck(const FWildDeckData& InDeck)
{
	if (UWildDeckSubsystem* DeckSub = GetGameInstance()->GetSubsystem<UWildDeckSubsystem>())
	{
		DeckSub->SetSelectedDeck(InDeck);
	}
}

void AWildDeckSelectPlayerController::RequestStartMatch()
{
	UWildDeckSubsystem* DeckSub = GetGameInstance()->GetSubsystem<UWildDeckSubsystem>();
	if (!DeckSub || !DeckSub->HasSelectedDeck())
	{
		SetFallbackStatusText(FText::FromString(TEXT("대전할 덱이 선택되지 않았습니다.")), FLinearColor::Red);
		return;
	}

	const FWildDeckData& SelectedDeck = DeckSub->GetSelectedDeck();
	FText ErrorMsg;
	if (!DeckSub->ValidateDeck(SelectedDeck, ErrorMsg))
	{
		SetFallbackStatusText(ErrorMsg, FLinearColor::Red);
		return;
	}

	UWildMatchmakingSubsystem* MatchSub = GetGameInstance()->GetSubsystem<UWildMatchmakingSubsystem>();
	if (!MatchSub)
	{
		SetFallbackStatusText(FText::FromString(TEXT("매칭 서브시스템을 찾을 수 없습니다.")), FLinearColor::Red);
		return;
	}

	// 1. 매칭 모달 UI 팝업
	ShowMatchmakingUI();

	// 2. 매칭 큐 탐색 시작 (10초 타임아웃 후 봇 폴백)
	MatchSub->StartMatchmaking(SelectedDeck, 10.0f);
}

void AWildDeckSelectPlayerController::ShowMatchmakingUI()
{
	// UMG 위젯 클래스가 지정되어 있으면 UMG 생성
	if (MatchmakingWidgetClass)
	{
		MatchmakingWidgetInstance = CreateWidget<UUserWidget>(this, MatchmakingWidgetClass);
		if (MatchmakingWidgetInstance)
		{
			MatchmakingWidgetInstance->AddToViewport(200);
			return;
		}
	}

	// UMG가 없으면 Slate Fallback 모달 생성
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (LocalPlayer->ViewportClient && !FallbackMatchmakingWidget.IsValid())
		{
			TSharedRef<SWildMatchmakingDialogWidget> Dialog =
				SNew(SWildMatchmakingDialogWidget)
				.PlayerController(this);

			FallbackMatchmakingWidget = Dialog;
			FallbackMatchmakingWidgetRef = Dialog;

			LocalPlayer->ViewportClient->AddViewportWidgetContent(Dialog, 200);
		}
	}
}

void AWildDeckSelectPlayerController::HideMatchmakingUI()
{
	if (MatchmakingWidgetInstance)
	{
		MatchmakingWidgetInstance->RemoveFromParent();
		MatchmakingWidgetInstance = nullptr;
	}

	if (FallbackMatchmakingWidget.IsValid())
	{
		if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
		{
			if (LocalPlayer->ViewportClient)
			{
				LocalPlayer->ViewportClient->RemoveViewportWidgetContent(FallbackMatchmakingWidget.ToSharedRef());
			}
		}
		FallbackMatchmakingWidget.Reset();
	}
}

void AWildDeckSelectPlayerController::HandleMatchmakingStateChanged(EWildMatchmakingState NewState)
{
	if (TSharedPtr<SWildMatchmakingDialogWidget> Widget = FallbackMatchmakingWidgetRef.Pin())
	{
		Widget->OnMatchmakingStateChanged(NewState);
	}

	if (NewState == EWildMatchmakingState::Cancelled || NewState == EWildMatchmakingState::Idle)
	{
		HideMatchmakingUI();
		SetFallbackStatusText(FText::FromString(TEXT("매칭이 취소되었습니다. 덱을 선택해주세요.")), FLinearColor(0.8f, 0.8f, 0.8f, 1.0f));
	}
	else if (NewState == EWildMatchmakingState::Error)
	{
		HideMatchmakingUI();
	}
}

void AWildDeckSelectPlayerController::HandleMatchmakingTimerTick(int32 ElapsedSeconds, int32 RemainingSecondsUntilBot)
{
	if (TSharedPtr<SWildMatchmakingDialogWidget> Widget = FallbackMatchmakingWidgetRef.Pin())
	{
		Widget->OnTimerTick(ElapsedSeconds, RemainingSecondsUntilBot);
	}
}

void AWildDeckSelectPlayerController::HandleMatchFound(const FWildMatchResult& MatchResult)
{
	CachedMatchResult = MatchResult;

	if (TSharedPtr<SWildMatchmakingDialogWidget> Widget = FallbackMatchmakingWidgetRef.Pin())
	{
		Widget->OnMatchFound(MatchResult);
	}

	// 호스트는 1.5초 후 Listen Server를 열고, 클라이언트는 2.5초 후 호스트에 접속 (서버 소켓 개방 여유 확보)
	const float TravelDelay = (MatchResult.bIsHost || MatchResult.Opponent.bIsBot) ? 1.5f : 2.5f;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TravelTimerHandle,
			this,
			&AWildDeckSelectPlayerController::ExecuteTravelToMatch,
			TravelDelay,
			false
		);
	}
}

void AWildDeckSelectPlayerController::ExecuteTravelToMatch()
{
	HideMatchmakingUI();

	UE_LOG(LogTemp, Log, TEXT("[WildDeckSelect] 매칭 성사 트래블 실행. MatchID: %s, ServerAddress: %s, IsHost: %d, NetMode: %d"),
		*CachedMatchResult.MatchId, *CachedMatchResult.ServerAddress, CachedMatchResult.bIsHost, (int32)GetNetMode());

	// 1. [Dedicated Server 클라이언트] 이미 전용 서버에 접속 중인 경우, 서버에 준비 완료 통보 (서버가 ServerTravel 주도)
	if (GetNetMode() == NM_Client)
	{
		Server_NotifyMatchReady(CachedMatchResult.MatchId);
		return;
	}

	// 2. [Dedicated Server 본체] 전용 서버 프로세스 내부 실행인 경우
	if (IsRunningDedicatedServer() && HasAuthority())
	{
		if (AWildDeckSelectGameMode* GM = GetWorld()->GetAuthGameMode<AWildDeckSelectGameMode>())
		{
			GM->HandlePlayerMatchReady(this, CachedMatchResult.MatchId);
		}
		return;
	}

	// 3. [1인 봇 매칭] 로컬 인게임 레벨 진입
	if (CachedMatchResult.Opponent.bIsBot)
	{
		FName TargetLevel = NextLevelName.IsNone() ? FName(TEXT("start_test")) : NextLevelName;
		UGameplayStatics::OpenLevel(this, TargetLevel);
		return;
	}

	// 4. [2인 매칭 호스트] Listen Server로 맵을 열어 상대 접속 수신 대기
	if (CachedMatchResult.bIsHost)
	{
		FName TargetLevel = NextLevelName.IsNone() ? FName(TEXT("start_test")) : NextLevelName;
		UGameplayStatics::OpenLevel(this, TargetLevel, true, TEXT("listen"));
		return;
	}

	// 5. [2인 매칭 클라이언트] 호스트의 서버 주소로 원격 접속
	FString ConnectAddress = CachedMatchResult.ServerAddress;
	if (ConnectAddress.IsEmpty() || ConnectAddress.Contains(TEXT("listen")))
	{
		ConnectAddress = DefaultServerAddress;
	}
	UE_LOG(LogTemp, Log, TEXT("[WildDeckSelect] 클라이언트 ClientTravel 실행: %s"), *ConnectAddress);
	ClientTravel(ConnectAddress, TRAVEL_Absolute);
}

bool AWildDeckSelectPlayerController::Server_NotifyMatchReady_Validate(const FString& MatchId)
{
	return true;
}

void AWildDeckSelectPlayerController::Server_NotifyMatchReady_Implementation(const FString& MatchId)
{
	UE_LOG(LogTemp, Log, TEXT("[WildDeckSelect] Server_NotifyMatchReady 수신: Player %s, MatchID: %s"),
		*GetName(), *MatchId);

	if (AWildDeckSelectGameMode* GM = GetWorld()->GetAuthGameMode<AWildDeckSelectGameMode>())
	{
		GM->HandlePlayerMatchReady(this, MatchId);
	}
}

void AWildDeckSelectPlayerController::RequestReturnToLogin()
{
	UE_LOG(LogTemp, Log, TEXT("[WildDeckSelect] 로그인 화면으로 복귀: %s"), *LoginLevelName.ToString());
	if (!LoginLevelName.IsNone())
	{
		UGameplayStatics::OpenLevel(this, LoginLevelName);
	}
}

FString AWildDeckSelectPlayerController::GetActiveUsername() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWildAuthSubsystem* AuthSub = GI->GetSubsystem<UWildAuthSubsystem>())
		{
			if (AuthSub->IsLoggedIn())
			{
				return AuthSub->GetCurrentSession().Username;
			}
		}
	}
	return TEXT("Guest");
}

void AWildDeckSelectPlayerController::SetFallbackStatusText(const FText& InText, const FLinearColor& InColor)
{
	if (TSharedPtr<SWildDeckSelectFallbackWidget> Widget = FallbackDeckSelectWidgetRef.Pin())
	{
		Widget->SetStatusText(InText, InColor);
	}
}
