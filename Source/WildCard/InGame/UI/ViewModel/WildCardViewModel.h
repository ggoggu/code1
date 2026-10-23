// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "BattleData.h"
#include "WildCardViewModel.generated.h"

/**
 * 
 */

UCLASS()
class WILDCARD_API UWildCardViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	// --- 동적 데이터 (서버 기준) ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Card|Dynamic")
	int32 InstanceID;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Card|Dynamic")
	int32 CurrentManaCost;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Card|Dynamic")
	int32 Force;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Card|Dynamic")
	int32 Intelligence;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Card|Dynamic")
	int32 Attack;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Card|Dynamic")
	int32 Health;

	// --- 정적 데이터 (클라이언트 기준) ---
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Card|Static")
	FString CardName;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Card|Static")
	UTexture2D* CardImage;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Card|Static")
	UTexture2D* CardFrameImage;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Card|Static")
	FGameplayTagContainer CardType;

public:
	// 동적 데이터(서버)와 정적 데이터(데이터테이블)를 한 번에 주입받아 초기화
	void InitializeFromData(const FWildCardDisplayData& InDynamicData, const FCardData* InStaticData);

	// 단일 스탯 변경을 위한 Setter (데미지를 입거나 버프를 받을 때 사용)
	void SetHealth(int32 InHealth) { UE_MVVM_SET_PROPERTY_VALUE(Health, InHealth); }
	void SetAttack(int32 InAttack) { UE_MVVM_SET_PROPERTY_VALUE(Attack, InAttack); }
	
};
