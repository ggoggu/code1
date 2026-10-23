// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IWildAuthProvider.h"
#include "UObject/StrongObjectPtr.h"

#include "WildAccountSaveGame.h"

class UGameInstance;

/**
 * 로컬 디스크(SaveGame) 기반 인증 프로바이더
 * 현재 로컬 개발 및 테스트를 담당하며 가상 네트워크 지연을 시뮬레이션합니다.
 */
class WILDCARD_API FWildLocalAuthProvider : public IWildAuthProvider, public TSharedFromThis<FWildLocalAuthProvider>
{
public:
	explicit FWildLocalAuthProvider(UGameInstance* InGameInstance);
	virtual ~FWildLocalAuthProvider() override;

	// IWildAuthProvider 인터페이스 구현
	virtual void Initialize() override;
	virtual void SignUp(const FWildAuthCredentials& InCredentials, const FOnWildAuthCompleted& InCallback) override;
	virtual void SignIn(const FWildAuthCredentials& InCredentials, const FOnWildAuthCompleted& InCallback) override;
	virtual void SignOut(TFunction<void()> InCallback) override;

private:
	TWeakObjectPtr<UGameInstance> GameInstanceWeak;

	static const FString SaveSlotName;
	static constexpr float SimulatedNetworkDelaySeconds = 0.25f;

	TStrongObjectPtr<UWildAccountSaveGame> CachedSaveGame;

	void LoadAccounts();
	void SaveAccounts();
	void ExecuteWithSimulatedDelay(TFunction<void()> Task);
};
