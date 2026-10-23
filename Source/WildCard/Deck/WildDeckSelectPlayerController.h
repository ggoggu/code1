// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BattleData.h"
#include "WildMatchmakingTypes.h"
#include "WildDeckSelectPlayerController.generated.h"

class UUserWidget;
class SWidget;
class SWildDeckSelectFallbackWidget;

/**
 * 덱 선택 레벨 전용 PlayerController
 * 마우스 커서 표시, UI 입력 모드 설정, 덱 선택 이벤트 및 대전 시작(인게임 전환)을 관리합니다.
 * UMG 위젯 클래스(DeckSelectWidgetClass)가 비어있을 경우 개발용 Slate Fallback UI를 제공합니다.
 */
UCLASS()
class WILDCARD_API AWildDeckSelectPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AWildDeckSelectPlayerController();

	/** 덱 선택 처리 (Fallback UI 및 UMG 공용) */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Deck")
	void SelectDeck(const FWildDeckData& InDeck);

	/** 대전 시작 요청 (선택된 덱 검증 후 NextLevelName으로 이동) */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Deck")
	void RequestStartMatch();

	/** 로그인 화면으로 돌아가기 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Deck")
	void RequestReturnToLogin();

	/** 현재 로그인한 유저명 반환 */
	UFUNCTION(BlueprintPure, Category = "WildCard|Deck")
	FString GetActiveUsername() const;

	/** 폴백 UI 상태 텍스트 갱신 */
	void SetFallbackStatusText(const FText& InText, const FLinearColor& InColor = FLinearColor::White);

	/** 대전 시작 시 이동할 레벨 이름 반환 */
	UFUNCTION(BlueprintPure, Category = "WildCard|Deck")
	FName GetNextLevelName() const { return NextLevelName; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 화면에 생성할 덱 선택 UI 위젯 클래스 (WBP_DeckSelect 등) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> DeckSelectWidgetClass;

	/** 생성된 덱 선택 UI 위젯 인스턴스 */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> DeckSelectWidgetInstance;

	/** 대전 시작 시 이동할 레벨 이름 (기본값: start_test) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WildCard|Deck")
	FName NextLevelName = FName(TEXT("start_test"));

	/** 뒤로가기 시 이동할 로그인 레벨 이름 (기본값: L_Login) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WildCard|Deck")
	FName LoginLevelName = FName(TEXT("L_Login"));

	/** 원격 전용 서버 기본 접속 주소 (기본값: 127.0.0.1:7777) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WildCard|Network")
	FString DefaultServerAddress = TEXT("127.0.0.1:7777");

	/** 매칭 모달 UMG 위젯 클래스 (WBP_MatchmakingDialog 등) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Matchmaking")
	TSubclassOf<UUserWidget> MatchmakingWidgetClass;

	/** 생성된 매칭 모달 UMG 위젯 인스턴스 */
	UPROPERTY(BlueprintReadOnly, Category = "UI|Matchmaking")
	TObjectPtr<UUserWidget> MatchmakingWidgetInstance;

private:
	/** DeckSelectWidgetClass가 비어있을 때 표시되는 폴백 Slate 위젯 */
	TSharedPtr<SWidget> FallbackDeckSelectWidget;
	TWeakPtr<SWildDeckSelectFallbackWidget> FallbackDeckSelectWidgetRef;

	/** MatchmakingWidgetClass가 비어있을 때 표시되는 매칭 모달 Slate 위젯 */
	TSharedPtr<SWidget> FallbackMatchmakingWidget;
	TWeakPtr<class SWildMatchmakingDialogWidget> FallbackMatchmakingWidgetRef;

	void ShowMatchmakingUI();
	void HideMatchmakingUI();

	UFUNCTION()
	void HandleMatchmakingStateChanged(EWildMatchmakingState NewState);

	UFUNCTION()
	void HandleMatchmakingTimerTick(int32 ElapsedSeconds, int32 RemainingSecondsUntilBot);

	UFUNCTION()
	void HandleMatchFound(const FWildMatchResult& MatchResult);

	void ExecuteTravelToMatch();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_NotifyMatchReady(const FString& MatchId);

	FTimerHandle TravelTimerHandle;
	FWildMatchResult CachedMatchResult;

	void RefreshFallbackUI();
};
