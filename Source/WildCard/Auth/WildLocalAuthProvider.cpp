// Fill out your copyright notice in the Description page of Project Settings.

#include "WildLocalAuthProvider.h"
#include "WildAccountSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Misc/SecureHash.h"

const FString FWildLocalAuthProvider::SaveSlotName = TEXT("WildCard_LocalAccounts");

FWildLocalAuthProvider::FWildLocalAuthProvider(UGameInstance* InGameInstance)
	: GameInstanceWeak(InGameInstance)
{
}

FWildLocalAuthProvider::~FWildLocalAuthProvider()
{
	CachedSaveGame.Reset();
}

void FWildLocalAuthProvider::Initialize()
{
	LoadAccounts();
	UE_LOG(LogTemp, Log, TEXT("[WildAuth] FWildLocalAuthProvider initialized. Cached accounts count: %d"),
		CachedSaveGame.IsValid() ? CachedSaveGame->Accounts.Num() : 0);
}

void FWildLocalAuthProvider::SignUp(const FWildAuthCredentials& InCredentials, const FOnWildAuthCompleted& InCallback)
{
	const FString TrimmedUsername = InCredentials.Username.TrimStartAndEnd();
	const FString TrimmedPassword = InCredentials.Password.TrimStartAndEnd();

	if (TrimmedUsername.IsEmpty() || TrimmedPassword.IsEmpty())
	{
		ExecuteWithSimulatedDelay([InCallback]()
		{
			InCallback.ExecuteIfBound(EWildAuthResult::InvalidCredentials, FWildUserSession(), TEXT("아이디와 비밀번호를 모두 입력해주세요."));
		});
		return;
	}

	if (TrimmedUsername.Len() < 3 || TrimmedPassword.Len() < 4)
	{
		ExecuteWithSimulatedDelay([InCallback]()
		{
			InCallback.ExecuteIfBound(EWildAuthResult::InvalidCredentials, FWildUserSession(), TEXT("아이디는 3자 이상, 비밀번호는 4자 이상이어야 합니다."));
		});
		return;
	}

	ExecuteWithSimulatedDelay([this, TrimmedUsername, TrimmedPassword, InCallback]()
	{
		LoadAccounts();

		const FString AccountKey = TrimmedUsername.ToLower();
		if (CachedSaveGame->Accounts.Contains(AccountKey))
		{
			UE_LOG(LogTemp, Warning, TEXT("[WildAuth] SignUp failed: User '%s' already exists."), *TrimmedUsername);
			InCallback.ExecuteIfBound(EWildAuthResult::UserAlreadyExists, FWildUserSession(), TEXT("이미 존재하는 아이디입니다."));
			return;
		}

		FWildAccountRecord NewAccount;
		NewAccount.UserId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
		NewAccount.Username = TrimmedUsername;
		NewAccount.PasswordHash = FMD5::HashAnsiString(*TrimmedPassword);
		NewAccount.CreatedAt = FDateTime::UtcNow();
		NewAccount.LastLoginAt = NewAccount.CreatedAt;

		CachedSaveGame->Accounts.Add(AccountKey, NewAccount);
		SaveAccounts();

		FWildUserSession Session;
		Session.UserId = NewAccount.UserId;
		Session.Username = NewAccount.Username;
		Session.DisplayName = NewAccount.Username;
		Session.AccessToken = FString::Printf(TEXT("local_tok_%s"), *FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens));
		Session.SessionStartTime = FDateTime::UtcNow();

		UE_LOG(LogTemp, Log, TEXT("[WildAuth] SignUp succeeded: User '%s' (UserId: %s) registered."), *TrimmedUsername, *Session.UserId);
		InCallback.ExecuteIfBound(EWildAuthResult::Success, Session, FString());
	});
}

void FWildLocalAuthProvider::SignIn(const FWildAuthCredentials& InCredentials, const FOnWildAuthCompleted& InCallback)
{
	const FString TrimmedUsername = InCredentials.Username.TrimStartAndEnd();
	const FString TrimmedPassword = InCredentials.Password.TrimStartAndEnd();

	if (TrimmedUsername.IsEmpty() || TrimmedPassword.IsEmpty())
	{
		ExecuteWithSimulatedDelay([InCallback]()
		{
			InCallback.ExecuteIfBound(EWildAuthResult::InvalidCredentials, FWildUserSession(), TEXT("아이디와 비밀번호를 모두 입력해주세요."));
		});
		return;
	}

	ExecuteWithSimulatedDelay([this, TrimmedUsername, TrimmedPassword, InCallback]()
	{
		LoadAccounts();

		const FString AccountKey = TrimmedUsername.ToLower();
		const FWildAccountRecord* Account = CachedSaveGame->Accounts.Find(AccountKey);
		if (!Account)
		{
			UE_LOG(LogTemp, Warning, TEXT("[WildAuth] SignIn failed: User '%s' not found."), *TrimmedUsername);
			InCallback.ExecuteIfBound(EWildAuthResult::UserNotFound, FWildUserSession(), TEXT("존재하지 않는 아이디입니다."));
			return;
		}

		const FString InputHash = FMD5::HashAnsiString(*TrimmedPassword);
		if (Account->PasswordHash != InputHash)
		{
			UE_LOG(LogTemp, Warning, TEXT("[WildAuth] SignIn failed: Incorrect password for user '%s'."), *TrimmedUsername);
			InCallback.ExecuteIfBound(EWildAuthResult::InvalidCredentials, FWildUserSession(), TEXT("비밀번호가 일치하지 않습니다."));
			return;
		}

		// 로그인 시간 갱신
		FWildAccountRecord UpdatedAccount = *Account;
		UpdatedAccount.LastLoginAt = FDateTime::UtcNow();
		CachedSaveGame->Accounts.Add(AccountKey, UpdatedAccount);
		SaveAccounts();

		FWildUserSession Session;
		Session.UserId = UpdatedAccount.UserId;
		Session.Username = UpdatedAccount.Username;
		Session.DisplayName = UpdatedAccount.Username;
		Session.AccessToken = FString::Printf(TEXT("local_tok_%s"), *FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens));
		Session.SessionStartTime = FDateTime::UtcNow();

		UE_LOG(LogTemp, Log, TEXT("[WildAuth] SignIn succeeded: User '%s' logged in."), *TrimmedUsername);
		InCallback.ExecuteIfBound(EWildAuthResult::Success, Session, FString());
	});
}

void FWildLocalAuthProvider::SignOut(TFunction<void()> InCallback)
{
	ExecuteWithSimulatedDelay([InCallback]()
	{
		UE_LOG(LogTemp, Log, TEXT("[WildAuth] SignOut succeeded locally."));
		if (InCallback)
		{
			InCallback();
		}
	});
}

void FWildLocalAuthProvider::LoadAccounts()
{
	if (CachedSaveGame.IsValid())
	{
		return;
	}

	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
	{
		USaveGame* LoadedGame = UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0);
		if (UWildAccountSaveGame* CastedSave = Cast<UWildAccountSaveGame>(LoadedGame))
		{
			CachedSaveGame.Reset(CastedSave);
			return;
		}
	}

	UWildAccountSaveGame* NewSave = Cast<UWildAccountSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UWildAccountSaveGame::StaticClass()));
	CachedSaveGame.Reset(NewSave);
	SaveAccounts();
}

void FWildLocalAuthProvider::SaveAccounts()
{
	if (CachedSaveGame.IsValid())
	{
		UGameplayStatics::SaveGameToSlot(CachedSaveGame.Get(), SaveSlotName, 0);
	}
}

void FWildLocalAuthProvider::ExecuteWithSimulatedDelay(TFunction<void()> Task)
{
	if (!GameInstanceWeak.IsValid())
	{
		Task();
		return;
	}

	UWorld* World = GameInstanceWeak->GetWorld();
	if (!World)
	{
		Task();
		return;
	}

	FTimerHandle DelayHandle;
	World->GetTimerManager().SetTimer(DelayHandle, [Task]()
	{
		Task();
	}, SimulatedNetworkDelaySeconds, false);
}
