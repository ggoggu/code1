// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WildAccountSaveGame.generated.h"

/**
 * 로컬에 저장되는 단일 계정 데이터
 */
USTRUCT(BlueprintType)
struct FWildAccountRecord
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Account")
	FString UserId;

	UPROPERTY(VisibleAnywhere, Category = "Account")
	FString Username;

	UPROPERTY(VisibleAnywhere, Category = "Account")
	FString PasswordHash;

	UPROPERTY(VisibleAnywhere, Category = "Account")
	FDateTime CreatedAt = FDateTime::MinValue();

	UPROPERTY(VisibleAnywhere, Category = "Account")
	FDateTime LastLoginAt = FDateTime::MinValue();
};

/**
 * 로컬 테스트용 계정 데이터 세이브게임
 */
UCLASS()
class WILDCARD_API UWildAccountSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// Key: Username (소문자 표준화 권장)
	UPROPERTY(VisibleAnywhere, Category = "Account")
	TMap<FString, FWildAccountRecord> Accounts;
};
