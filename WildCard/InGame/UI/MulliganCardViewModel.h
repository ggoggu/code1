// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "BattleData.h"
#include "MulliganCardViewModel.generated.h"

/**
 * 
 */
UCLASS()
class WILDCARD_API UMulliganCardViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()
	
public:
	// 구조체 데이터를 뷰모델에 주입
	void InitializeFromData(const FWildCardDisplayData& InData);

	UFUNCTION(BlueprintCallable)
	void ToggleSelection();

	bool IsSelectedForSwap() const { return bIsSelectedForSwap; }
	int32 GetInstanceID() const { return InstanceID; }
public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Mulligan")
	int32 InstanceID; // 서버 통신용 고유 식별자

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Mulligan")
	FString CardName;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Mulligan")
	UTexture2D* CardImage;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Mulligan")
	UTexture2D* CardFrameImage;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Mulligan")
	int32 Force;
	
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Mulligan")
	int32 Intelligence;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Mulligan")
	int32 Attack;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Mulligan")
	int32 Health;

	// 교체 대기 상태인지 여부 (UI에서 V 체크 표시 등에 사용)
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Mulligan")
	bool bIsSelectedForSwap;
	
	
};
