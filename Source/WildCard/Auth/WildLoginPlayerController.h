// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WildAuthTypes.h"
#include "WildLoginPlayerController.generated.h"

class UUserWidget;
class SWidget;

/**
 * 로그인/회원가입 화면 전용 PlayerController
 * 마우스 커서 활성화 및 UI 전용 입력 모드를 설정하고 로그인 위젯을 뷰포트에 생성합니다.
 * UMG 위젯 클래스(LoginWidgetClass)가 지정되지 않은 경우 개발용 폴백 Slate UI를 제공합니다.
 */
UCLASS()
class WILDCARD_API AWildLoginPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AWildLoginPlayerController();

	/** 로그인 성공 시 이동할 기본 레벨 이름 반환 */
	UFUNCTION(BlueprintPure, Category = "WildCard|Auth")
	FName GetNextLevelName() const { return NextLevelName; }

	/** 폴백 UI 등에서 직접 로그인 요청 */
	void RequestSignInFromFallback(const FString& Username, const FString& Password);

	/** 폴백 UI 등에서 직접 회원가입 요청 */
	void RequestSignUpFromFallback(const FString& Username, const FString& Password);

	/** 폴백 UI에서 바로 시작(테스트 레벨 진입) 요청 */
	void RequestQuickStartFromFallback();

	/** 폴백 UI 상태 텍스트 갱신 */
	void SetFallbackStatusText(const FText& InText, const FLinearColor& InColor = FLinearColor::White);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 화면에 생성할 로그인 UI 위젯 클래스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> LoginWidgetClass;

	/** 생성된 로그인 UI 위젯 인스턴스 */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> LoginWidgetInstance;

	/** 로그인 성공 시 이동할 다음 레벨 이름 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WildCard|Auth")
	FName NextLevelName = FName(TEXT("L_DeckSelect"));

private:
	/** LoginWidgetClass가 비어있을 때 표시되는 폴백 Slate 위젯 */
	TSharedPtr<SWidget> FallbackLoginWidget;
	TWeakPtr<class SWildLoginFallbackWidget> FallbackLoginWidgetRef;

	void ShowFallbackLoginUI();
	void RemoveFallbackLoginUI();

	UFUNCTION()
	void HandleSignInCompleted(EWildAuthResult Result, const FWildUserSession& Session, const FString& ErrorMessage);

	UFUNCTION()
	void HandleSignUpCompleted(EWildAuthResult Result, const FWildUserSession& Session, const FString& ErrorMessage);
};
