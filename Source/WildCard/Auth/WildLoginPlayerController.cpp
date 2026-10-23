// Fill out your copyright notice in the Description page of Project Settings.

#include "WildLoginPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "WildAuthSubsystem.h"
#include "Auth/UI/SWildLoginFallbackWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"

AWildLoginPlayerController::AWildLoginPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AWildLoginPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 마우스 커서 및 UI 전용 입력 모드 설정
	FInputModeUIOnly InputModeData;
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputModeData);
	bShowMouseCursor = true;

	if (!IsLocalController())
	{
		return;
	}

	// 1. 위젯 클래스가 미지정된 경우 로그 출력 (이후 미지정 시 폴백 Slate UI 표시)
	if (!LoginWidgetClass)
	{
		UE_LOG(LogTemp, Log, TEXT("[WildAuth] LoginWidgetClass가 지정되지 않아 개발용 폴백 Slate UI를 사용합니다."));
	}

	// 2. 인증 서브시스템 이벤트 구독
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWildAuthSubsystem* AuthSubsystem = GI->GetSubsystem<UWildAuthSubsystem>())
		{
			AuthSubsystem->OnSignInCompleted.AddDynamic(this, &AWildLoginPlayerController::HandleSignInCompleted);
			AuthSubsystem->OnSignUpCompleted.AddDynamic(this, &AWildLoginPlayerController::HandleSignUpCompleted);
		}
	}

	// 3. 지정된 UMG 위젯 클래스가 있으면 생성 및 뷰포트 배치
	if (LoginWidgetClass)
	{
		LoginWidgetInstance = CreateWidget<UUserWidget>(this, LoginWidgetClass);
		if (LoginWidgetInstance)
		{
			LoginWidgetInstance->AddToViewport();
			UE_LOG(LogTemp, Log, TEXT("[WildAuth] LoginWidget created and added to viewport: %s"), *LoginWidgetClass->GetName());
			return;
		}
	}

	// 4. UMG 위젯이 없는 경우 검은 화면 방지용 개발자 폴백 Slate UI 표시
	UE_LOG(LogTemp, Warning, TEXT("[WildAuth] LoginWidgetClass is null in AWildLoginPlayerController! Showing fallback Slate Login UI. To customize UI, create a Widget Blueprint inheriting from UWildLoginWidget and assign it to LoginWidgetClass."));
	ShowFallbackLoginUI();
}

void AWildLoginPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWildAuthSubsystem* AuthSubsystem = GI->GetSubsystem<UWildAuthSubsystem>())
		{
			AuthSubsystem->OnSignInCompleted.RemoveDynamic(this, &AWildLoginPlayerController::HandleSignInCompleted);
			AuthSubsystem->OnSignUpCompleted.RemoveDynamic(this, &AWildLoginPlayerController::HandleSignUpCompleted);
		}
	}

	RemoveFallbackLoginUI();

	Super::EndPlay(EndPlayReason);
}

void AWildLoginPlayerController::ShowFallbackLoginUI()
{
	if (FallbackLoginWidget.IsValid())
	{
		return;
	}

	TSharedRef<SWildLoginFallbackWidget> WidgetRef = SNew(SWildLoginFallbackWidget)
		.PlayerController(this);

	FallbackLoginWidget = WidgetRef;
	FallbackLoginWidgetRef = WidgetRef;

	if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
	{
		if (LocalPlayer->ViewportClient)
		{
			LocalPlayer->ViewportClient->AddViewportWidgetContent(WidgetRef, 100);
			UE_LOG(LogTemp, Log, TEXT("[WildAuth] Fallback Slate Login UI added to viewport."));
			return;
		}
	}

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->AddViewportWidgetContent(WidgetRef, 100);
		UE_LOG(LogTemp, Log, TEXT("[WildAuth] Fallback Slate Login UI added via GameViewport."));
	}
}

void AWildLoginPlayerController::RemoveFallbackLoginUI()
{
	if (!FallbackLoginWidget.IsValid())
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
	{
		if (LocalPlayer->ViewportClient)
		{
			LocalPlayer->ViewportClient->RemoveViewportWidgetContent(FallbackLoginWidget.ToSharedRef());
		}
	}
	else if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(FallbackLoginWidget.ToSharedRef());
	}

	FallbackLoginWidget.Reset();
	FallbackLoginWidgetRef.Reset();
}

void AWildLoginPlayerController::RequestSignInFromFallback(const FString& Username, const FString& Password)
{
	if (Username.TrimStartAndEnd().IsEmpty() || Password.TrimStartAndEnd().IsEmpty())
	{
		SetFallbackStatusText(FText::FromString(TEXT("아이디와 비밀번호를 모두 입력해주세요.")), FLinearColor(1.0f, 0.4f, 0.4f, 1.0f));
		return;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWildAuthSubsystem* AuthSubsystem = GI->GetSubsystem<UWildAuthSubsystem>())
		{
			SetFallbackStatusText(FText::FromString(TEXT("로그인 요청 중...")), FLinearColor(0.4f, 0.8f, 1.0f, 1.0f));
			AuthSubsystem->SignIn(Username, Password);
			return;
		}
	}

	SetFallbackStatusText(FText::FromString(TEXT("UWildAuthSubsystem을 찾을 수 없습니다.")), FLinearColor(1.0f, 0.4f, 0.4f, 1.0f));
}

void AWildLoginPlayerController::RequestSignUpFromFallback(const FString& Username, const FString& Password)
{
	if (Username.TrimStartAndEnd().IsEmpty() || Password.TrimStartAndEnd().IsEmpty())
	{
		SetFallbackStatusText(FText::FromString(TEXT("아이디와 비밀번호를 모두 입력해주세요.")), FLinearColor(1.0f, 0.4f, 0.4f, 1.0f));
		return;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWildAuthSubsystem* AuthSubsystem = GI->GetSubsystem<UWildAuthSubsystem>())
		{
			SetFallbackStatusText(FText::FromString(TEXT("회원가입 요청 중...")), FLinearColor(0.4f, 0.8f, 1.0f, 1.0f));
			AuthSubsystem->SignUp(Username, Password);
			return;
		}
	}

	SetFallbackStatusText(FText::FromString(TEXT("UWildAuthSubsystem을 찾을 수 없습니다.")), FLinearColor(1.0f, 0.4f, 0.4f, 1.0f));
}

void AWildLoginPlayerController::RequestQuickStartFromFallback()
{
	UE_LOG(LogTemp, Log, TEXT("[WildAuth] Quick Start selected. Opening level: %s"), *NextLevelName.ToString());
	UGameplayStatics::OpenLevel(this, NextLevelName);
}

void AWildLoginPlayerController::SetFallbackStatusText(const FText& InText, const FLinearColor& InColor)
{
	if (TSharedPtr<SWildLoginFallbackWidget> Widget = FallbackLoginWidgetRef.Pin())
	{
		Widget->SetStatusText(InText, InColor);
	}
}

void AWildLoginPlayerController::HandleSignInCompleted(EWildAuthResult Result, const FWildUserSession& Session, const FString& ErrorMessage)
{
	// 커스텀 UMG 위젯이 활성화된 경우 UMG(UWildLoginWidget)가 직접 처리하도록 위임
	if (LoginWidgetInstance)
	{
		return;
	}

	if (Result == EWildAuthResult::Success)
	{
		UE_LOG(LogTemp, Log, TEXT("[WildAuth] SignIn succeeded for '%s'. Opening next level: %s"), *Session.Username, *NextLevelName.ToString());
		SetFallbackStatusText(FText::Format(FText::FromString(TEXT("로그인 성공! '{0}' 님 환영합니다. 레벨 이동 중...")), FText::FromString(Session.Username)), FLinearColor(0.4f, 1.0f, 0.5f, 1.0f));

		if (!NextLevelName.IsNone())
		{
			UGameplayStatics::OpenLevel(this, NextLevelName);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[WildAuth] SignIn failed: %s"), *ErrorMessage);
		SetFallbackStatusText(FText::Format(FText::FromString(TEXT("로그인 실패: {0}")), FText::FromString(ErrorMessage)), FLinearColor(1.0f, 0.4f, 0.4f, 1.0f));
	}
}

void AWildLoginPlayerController::HandleSignUpCompleted(EWildAuthResult Result, const FWildUserSession& Session, const FString& ErrorMessage)
{
	if (LoginWidgetInstance)
	{
		return;
	}

	if (Result == EWildAuthResult::Success)
	{
		UE_LOG(LogTemp, Log, TEXT("[WildAuth] SignUp succeeded for '%s'."), *Session.Username);
		SetFallbackStatusText(FText::Format(FText::FromString(TEXT("회원가입 성공! '{0}' 계정으로 로그인해주세요.")), FText::FromString(Session.Username)), FLinearColor(0.4f, 1.0f, 0.5f, 1.0f));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[WildAuth] SignUp failed: %s"), *ErrorMessage);
		SetFallbackStatusText(FText::Format(FText::FromString(TEXT("회원가입 실패: {0}")), FText::FromString(ErrorMessage)), FLinearColor(1.0f, 0.4f, 0.4f, 1.0f));
	}
}
