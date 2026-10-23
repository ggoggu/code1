// Fill out your copyright notice in the Description page of Project Settings.

#include "WildLoginWidget.h"
#include "WildAuthSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UWildLoginWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		AuthSubsystem = GameInstance->GetSubsystem<UWildAuthSubsystem>();
		if (AuthSubsystem)
		{
			AuthSubsystem->OnSignInCompleted.AddDynamic(this, &UWildLoginWidget::HandleSignInCompleted);
			AuthSubsystem->OnSignUpCompleted.AddDynamic(this, &UWildLoginWidget::HandleSignUpCompleted);
		}
	}
}

void UWildLoginWidget::NativeDestruct()
{
	if (AuthSubsystem)
	{
		AuthSubsystem->OnSignInCompleted.RemoveDynamic(this, &UWildLoginWidget::HandleSignInCompleted);
		AuthSubsystem->OnSignUpCompleted.RemoveDynamic(this, &UWildLoginWidget::HandleSignUpCompleted);
	}

	Super::NativeDestruct();
}

void UWildLoginWidget::RequestSignIn(const FString& Username, const FString& Password)
{
	if (AuthSubsystem)
	{
		AuthSubsystem->SignIn(Username, Password);
	}
}

void UWildLoginWidget::RequestSignUp(const FString& Username, const FString& Password)
{
	if (AuthSubsystem)
	{
		AuthSubsystem->SignUp(Username, Password);
	}
}

void UWildLoginWidget::OnLoginSucceeded_Implementation(const FWildUserSession& Session)
{
	if (!NextLevelName.IsNone())
	{
		UE_LOG(LogTemp, Log, TEXT("[WildAuth] Login succeeded. Opening next level: %s"), *NextLevelName.ToString());
		UGameplayStatics::OpenLevel(this, NextLevelName);
	}
}

void UWildLoginWidget::HandleSignInCompleted(EWildAuthResult Result, const FWildUserSession& Session, const FString& ErrorMessage)
{
	BP_OnSignInCompleted(Result, Session, ErrorMessage);

	if (Result == EWildAuthResult::Success)
	{
		OnLoginSucceeded(Session);
	}
}

void UWildLoginWidget::HandleSignUpCompleted(EWildAuthResult Result, const FWildUserSession& Session, const FString& ErrorMessage)
{
	BP_OnSignUpCompleted(Result, Session, ErrorMessage);
}
