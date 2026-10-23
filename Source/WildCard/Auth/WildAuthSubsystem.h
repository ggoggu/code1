// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WildAuthTypes.h"
#include "IWildAuthProvider.h"
#include "WildAuthSubsystem.generated.h"

/**
 * 게임 인스턴스 전역에서 계정 인증(회원가입, 로그인, 세션 유지)을 총괄하는 서브시스템
 * UI/MVVM 및 게임플레이 로직에서 호출하며 내부적으로 IWildAuthProvider를 통해 실제 인증을 수행합니다.
 */
UCLASS()
class WILDCARD_API UWildAuthSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 회원가입 요청 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Auth")
	void SignUp(const FString& Username, const FString& Password);

	/** 로그인 요청 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Auth")
	void SignIn(const FString& Username, const FString& Password);

	/** 로그아웃 요청 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Auth")
	void SignOut();

	/** 로그인 여부 확인 */
	UFUNCTION(BlueprintPure, Category = "WildCard|Auth")
	bool IsLoggedIn() const;

	/** 현재 유효한 세션 정보 반환 */
	UFUNCTION(BlueprintPure, Category = "WildCard|Auth")
	const FWildUserSession& GetCurrentSession() const;

	/** 현재 인증 요청 진행 중 여부 */
	UFUNCTION(BlueprintPure, Category = "WildCard|Auth")
	bool IsAuthenticating() const { return bIsAuthenticating; }

	/** 외부에서 인증 프로바이더 교체 (추후 EOS / AWS 주입 시 사용) */
	void SetAuthProvider(TSharedPtr<IWildAuthProvider> InProvider);

public:
	/** 회원가입 완료 알림 */
	UPROPERTY(BlueprintAssignable, Category = "WildCard|Auth")
	FOnWildAuthResultSignature OnSignUpCompleted;

	/** 로그인 완료 알림 */
	UPROPERTY(BlueprintAssignable, Category = "WildCard|Auth")
	FOnWildAuthResultSignature OnSignInCompleted;

	/** 로그아웃 완료 알림 */
	UPROPERTY(BlueprintAssignable, Category = "WildCard|Auth")
	FOnWildAuthResultSignature OnSignOutCompleted;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WildCard|Auth")
	FWildUserSession CurrentSession;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WildCard|Auth")
	bool bIsAuthenticating = false;

private:
	TSharedPtr<IWildAuthProvider> ActiveProvider;

	void HandleSignUpResponse(EWildAuthResult Result, const FWildUserSession& Session, const FString& ErrorMessage);
	void HandleSignInResponse(EWildAuthResult Result, const FWildUserSession& Session, const FString& ErrorMessage);
	void HandleSignOutResponse();
};
