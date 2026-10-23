// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WildAuthTypes.h"

/**
 * 인증 프로바이더 인터페이스
 * 로컬 Mock/SaveGame, EOS SDK, AWS Cognito/Lambda 등 백엔드 구현체를 추상화합니다.
 */
class WILDCARD_API IWildAuthProvider
{
public:
	virtual ~IWildAuthProvider() = default;

	/** 프로바이더 초기화 */
	virtual void Initialize() = 0;

	/** 회원가입 요청 */
	virtual void SignUp(const FWildAuthCredentials& InCredentials, const FOnWildAuthCompleted& InCallback) = 0;

	/** 로그인 요청 */
	virtual void SignIn(const FWildAuthCredentials& InCredentials, const FOnWildAuthCompleted& InCallback) = 0;

	/** 로그아웃 요청 */
	virtual void SignOut(TFunction<void()> InCallback) = 0;
};
