// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "BattleData.h"
#include "MulliganViewModel.generated.h"

/**
 * 
 */
UCLASS()
class WILDCARD_API UMulliganViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	// UI에 바인딩될 5장의 카드 리스트
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Mulligan")
	TArray<class UMulliganCardViewModel*> MulliganCards;

	// UI 표시/숨김 제어용
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Mulligan")
	bool bIsMulliganPhaseActive;
	
	// 클라이언트에서 페이즈 변경 시 호출
	void SetMulliganActive(bool bActive);
	// 서버로부터 받은 5장의 카드로 리스트 세팅
	void SetupMulliganCards(const TArray<FWildCardDisplayData>& InCards);

	// 교체할 카드의 ID 목록을 수집하여 반환
	UFUNCTION(BlueprintCallable, Category = "Mulligan")
	TArray<int32> GetSelectedCardIDsToSwap() const;
	
};
