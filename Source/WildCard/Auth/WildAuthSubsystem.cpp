// Fill out your copyright notice in the Description page of Project Settings.

#include "WildAuthSubsystem.h"
#include "WildLocalAuthProvider.h"
#include "Engine/GameInstance.h"

void UWildAuthSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 기본값으로 로컬 프로바이더 등록 (추후 Config나 플래그에 따라 EOS/AWS 등으로 교체 가능)
	SetAuthProvider(MakeShared<FWildLocalAuthProvider>(GetGameInstance()));

	UE_LOG(LogTemp, Log, TEXT("[WildAuth] UWildAuthSubsystem initialized with default LocalAuthProvider."));
}

void UWildAuthSubsystem::Deinitialize()
{
	ActiveProvider.Reset();
	CurrentSession.Reset();
	bIsAuthenticating = false;

	Super::Deinitialize();
}

void UWildAuthSubsystem::SetAuthProvider(TSharedPtr<IWildAuthProvider> InProvider)
{
	ActiveProvider = InProvider;
	if (ActiveProvider.IsValid())
	{
		ActiveProvider->Initialize();
	}
}

void UWildAuthSubsystem::SignUp(const FString& Username, const FString& Password)
{
	if (bIsAuthenticating)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WildAuth] SignUp ignored: Another authentication operation is in progress."));
		return;
	}

	if (!ActiveProvider.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[WildAuth] SignUp failed: No active authentication provider."));
		OnSignUpCompleted.Broadcast(EWildAuthResult::UnknownError, FWildUserSession(), TEXT("인증 프로바이더가 설정되지 않았습니다."));
		return;
	}

	bIsAuthenticating = true;

	FWildAuthCredentials Credentials;
	Credentials.Username = Username;
	Credentials.Password = Password;

	ActiveProvider->SignUp(Credentials, FOnWildAuthCompleted::CreateUObject(this, &UWildAuthSubsystem::HandleSignUpResponse));
}

void UWildAuthSubsystem::SignIn(const FString& Username, const FString& Password)
{
	if (bIsAuthenticating)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WildAuth] SignIn ignored: Another authentication operation is in progress."));
		return;
	}

	if (!ActiveProvider.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[WildAuth] SignIn failed: No active authentication provider."));
		OnSignInCompleted.Broadcast(EWildAuthResult::UnknownError, FWildUserSession(), TEXT("인증 프로바이더가 설정되지 않았습니다."));
		return;
	}

	bIsAuthenticating = true;

	FWildAuthCredentials Credentials;
	Credentials.Username = Username;
	Credentials.Password = Password;

	ActiveProvider->SignIn(Credentials, FOnWildAuthCompleted::CreateUObject(this, &UWildAuthSubsystem::HandleSignInResponse));
}

void UWildAuthSubsystem::SignOut()
{
	if (!IsLoggedIn())
	{
		UE_LOG(LogTemp, Warning, TEXT("[WildAuth] SignOut ignored: User is not logged in."));
		return;
	}

	bIsAuthenticating = true;

	if (ActiveProvider.IsValid())
	{
		ActiveProvider->SignOut([this]()
		{
			HandleSignOutResponse();
		});
	}
	else
	{
		HandleSignOutResponse();
	}
}

bool UWildAuthSubsystem::IsLoggedIn() const
{
	return CurrentSession.IsValid();
}

const FWildUserSession& UWildAuthSubsystem::GetCurrentSession() const
{
	return CurrentSession;
}

void UWildAuthSubsystem::HandleSignUpResponse(EWildAuthResult Result, const FWildUserSession& Session, const FString& ErrorMessage)
{
	bIsAuthenticating = false;

	if (Result == EWildAuthResult::Success)
	{
		CurrentSession = Session;
	}

	OnSignUpCompleted.Broadcast(Result, Session, ErrorMessage);
}

void UWildAuthSubsystem::HandleSignInResponse(EWildAuthResult Result, const FWildUserSession& Session, const FString& ErrorMessage)
{
	bIsAuthenticating = false;

	if (Result == EWildAuthResult::Success)
	{
		CurrentSession = Session;
	}

	OnSignInCompleted.Broadcast(Result, Session, ErrorMessage);
}

void UWildAuthSubsystem::HandleSignOutResponse()
{
	bIsAuthenticating = false;
	CurrentSession.Reset();

	OnSignOutCompleted.Broadcast(EWildAuthResult::Success, FWildUserSession(), FString());
}
