// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WildAuthTypes.h"
#include "WildLoginWidget.generated.h"

class UWildAuthSubsystem;

/**
 * 로그인 및 회원가입 UI 위젯 C++ 베이스 클래스
 * UWildAuthSubsystem의 비동기 이벤트를 자동으로 구독하며 간편한 요청 함수를 제공합니다.
 */
UCLASS()
class WILDCARD_API UWildLoginWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 로그인 요청 (UI 버튼 등에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Auth")
	void RequestSignIn(const FString& Username, const FString& Password);

	/** 회원가입 요청 (UI 버튼 등에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Auth")
	void RequestSignUp(const FString& Username, const FString& Password);

protected:
	/** 로그인 성공 시 이동할 다음 레벨 이름 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "WildCard|Auth")
	FName NextLevelName = FName(TEXT("start_test"));

	/** 현재 참조 중인 인증 서브시스템 */
	UPROPERTY(BlueprintReadOnly, Category = "WildCard|Auth")
	TObjectPtr<UWildAuthSubsystem> AuthSubsystem;

	/** 로그인 성공 시 호출되는 기본 처리 (기본값: NextLevelName으로 OpenLevel) */
	UFUNCTION(BlueprintNativeEvent, Category = "WildCard|Auth")
	void OnLoginSucceeded(const FWildUserSession& Session);
	virtual void OnLoginSucceeded_Implementation(const FWildUserSession& Session);

	/** 블루프린트에서 UI 갱신을 위해 구현 가능한 로그인 완료 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "WildCard|Auth")
	void BP_OnSignInCompleted(EWildAuthResult Result, const FWildUserSession& Session, const FString& ErrorMessage);

	/** 블루프린트에서 UI 갱신을 위해 구현 가능한 회원가입 완료 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "WildCard|Auth")
	void BP_OnSignUpCompleted(EWildAuthResult Result, const FWildUserSession& Session, const FString& ErrorMessage);

private:
	UFUNCTION()
	void HandleSignInCompleted(EWildAuthResult Result, const FWildUserSession& Session, const FString& ErrorMessage);

	UFUNCTION()
	void HandleSignUpCompleted(EWildAuthResult Result, const FWildUserSession& Session, const FString& ErrorMessage);
};
