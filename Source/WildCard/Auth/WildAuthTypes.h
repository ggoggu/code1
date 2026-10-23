// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WildAuthTypes.generated.h"

/**
 * 회원가입 및 로그인 결과 열거형
 */
UENUM(BlueprintType)
enum class EWildAuthResult : uint8
{
	Success UMETA(DisplayName = "Success"),
	InvalidCredentials UMETA(DisplayName = "Invalid Credentials"),
	UserAlreadyExists UMETA(DisplayName = "User Already Exists"),
	UserNotFound UMETA(DisplayName = "User Not Found"),
	NetworkError UMETA(DisplayName = "Network Error"),
	UnknownError UMETA(DisplayName = "Unknown Error")
};

/**
 * 인증 요청 시 전달되는 자격 증명 (아이디/비밀번호 등)
 */
USTRUCT(BlueprintType)
struct FWildAuthCredentials
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auth")
	FString Username;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auth")
	FString Password;
};

/**
 * 로그인 성공 시 발급 및 유지되는 유저 세션 정보
 */
USTRUCT(BlueprintType)
struct FWildUserSession
{
	GENERATED_BODY()

	// 고유 계정 ID (로컬: GUID, 추후 EOS: PUID, AWS: Cognito Sub)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Auth")
	FString UserId;

	// 계정 이름
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Auth")
	FString Username;

	// 인게임 노출용 닉네임
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Auth")
	FString DisplayName;

	// Dedicated Server 또는 백엔드 인증 토큰 (추후 JWT / Session Token)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Auth")
	FString AccessToken;

	// 세션 생성 일시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Auth")
	FDateTime SessionStartTime = FDateTime::MinValue();

	bool IsValid() const
	{
		return !UserId.IsEmpty();
	}

	void Reset()
	{
		UserId.Empty();
		Username.Empty();
		DisplayName.Empty();
		AccessToken.Empty();
		SessionStartTime = FDateTime::MinValue();
	}
};

/** C++ 내부 콜백용 비동기 델리게이트 */
DECLARE_DELEGATE_ThreeParams(FOnWildAuthCompleted, EWildAuthResult /*Result*/, const FWildUserSession& /*Session*/, const FString& /*ErrorMessage*/);

/** 블루프린트 / UMG / MVVM 연동용 다이내믹 멀티캐스트 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWildAuthResultSignature, EWildAuthResult, Result, const FWildUserSession&, Session, const FString&, ErrorMessage);
