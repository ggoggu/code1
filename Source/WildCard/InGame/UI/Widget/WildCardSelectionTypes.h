// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WildCardSelectionTypes.generated.h"

class UWildCardViewModel;

/**
 * 손패 위젯의 상호작용 모드
 */
UENUM(BlueprintType)
enum class EHandInteractionMode : uint8
{
	Normal UMETA(DisplayName = "일반"),
	CardSelection UMETA(DisplayName = "카드 선택")
};

DECLARE_DELEGATE_OneParam(FOnCardSelectionConfirmed, const TArray<int32>& /*SelectedCardIDs*/);
DECLARE_DELEGATE(FOnCardSelectionCancelled);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCardSelectionCountChanged, int32 /*CurrentCount*/);

/**
 * 범용 카드 선택 요청 컨텍스트
 * 개별 카드, 주문, 전술 스킬이 UI에 카드 선택을 요청할 때 사용하는 파라미터 구조체입니다.
 */
USTRUCT(BlueprintType)
struct WILDCARD_API FWildCardSelectionContext
{
	GENERATED_BODY()

	// 안내 프롬프트 텍스트 (예: "패에서 카드 한장을 선택하세요")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Selection")
	FText PromptText = FText::FromString(TEXT("카드를 선택하세요."));

	// 확인 버튼 텍스트 (예: "확인", "선택 완료", "버리기")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Selection")
	FText ConfirmButtonText = FText::FromString(TEXT("확인"));

	// 최소 선택 매수 (기본 1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Selection")
	int32 MinSelectionCount = 1;

	// 최대 선택 매수 (기본 1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Selection")
	int32 MaxSelectionCount = 1;

	// 취소(배경 클릭 등) 허용 여부 (기본 true, 필수 선택 효과인 경우 false)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Card Selection")
	bool bAllowCancel = true;

	// 선택 필터 함수 (nullptr이면 모든 카드 선택 가능)
	TFunction<bool(const UWildCardViewModel*)> CanSelectFilter = nullptr;

	// 선택 완료 시 호출될 델리게이트
	FOnCardSelectionConfirmed OnConfirmedDelegate;

	// 취소 시 호출될 델리게이트
	FOnCardSelectionCancelled OnCancelledDelegate;

	FWildCardSelectionContext() = default;

	FWildCardSelectionContext(const FText& InPrompt, int32 InMinCount = 1, int32 InMaxCount = 1, const FText& InConfirmText = FText::FromString(TEXT("확인")))
		: PromptText(InPrompt)
		, ConfirmButtonText(InConfirmText)
		, MinSelectionCount(InMinCount)
		, MaxSelectionCount(InMaxCount)
	{
	}
};
