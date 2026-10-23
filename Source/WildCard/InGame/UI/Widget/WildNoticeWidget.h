// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BattleData.h"
#include "WildNoticeWidget.generated.h"

class UTextBlock;
class UBorder;
class UWidgetAnimation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNoticeDismissedSignature);

/**
 * 게임 내 특정 타이밍(턴 시작, 체인 시작 등)에 화면에 나타났다가
 * 일정 시간 후 자동으로 페이드 아웃/소멸하는 알림 위젯 베이스 클래스입니다.
 */
UCLASS()
class WILDCARD_API UWildNoticeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWildNoticeWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable, Category = "Notice")
	FOnNoticeDismissedSignature OnNoticeDismissed;

	// 공지 표시 함수 (유형, 커스텀 메시지, 표시 시간 - 음수면 기본값 사용)
	UFUNCTION(BlueprintCallable, Category = "Notice")
	virtual void ShowNotice(EWildTimingNoticeType InType, const FText& InMessage = FText::GetEmpty(), float InDuration = -1.0f);

	// 즉시 또는 아웃트로 후 닫기
	UFUNCTION(BlueprintCallable, Category = "Notice")
	virtual void DismissNotice();

	// 타이밍 타입 가져오기
	UFUNCTION(BlueprintCallable, Category = "Notice")
	EWildTimingNoticeType GetNoticeType() const { return NoticeType; }

	// 표시된 메시지 가져오기
	UFUNCTION(BlueprintCallable, Category = "Notice")
	FText GetNoticeMessage() const { return NoticeMessage; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 블루프린트에서 UMG 애니메이션이나 사운드 등을 자유롭게 연출할 수 있는 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "Notice")
	void BP_OnNoticeShown(EWildTimingNoticeType InType, const FText& InMessage, float InDuration);

	// 블루프린트에서 닫힐 때 호출되는 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "Notice")
	void BP_OnNoticeDismissed();

	// UMG 바인딩 위젯 (Optional)
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "Notice")
	TObjectPtr<UTextBlock> Text_NoticeMessage;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "Notice")
	TObjectPtr<UTextBlock> Text_NoticeTitle;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "Notice")
	TObjectPtr<UBorder> Border_Background;

	// UMG 애니메이션이 있을 경우 자동 연동 (Optional)
	UPROPERTY(Transient, BlueprintReadWrite, meta = (BindWidgetAnimOptional), Category = "Notice")
	TObjectPtr<UWidgetAnimation> Anim_Intro;

	UPROPERTY(Transient, BlueprintReadWrite, meta = (BindWidgetAnimOptional), Category = "Notice")
	TObjectPtr<UWidgetAnimation> Anim_Outro;

	// 기본 표시 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notice")
	float DefaultDisplayDuration;

	// 페이드 인/아웃 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notice")
	float FadeDuration;

	UPROPERTY(BlueprintReadOnly, Category = "Notice")
	EWildTimingNoticeType NoticeType;

	UPROPERTY(BlueprintReadOnly, Category = "Notice")
	FText NoticeMessage;

private:
	FTimerHandle DismissTimerHandle;
	float ElapsedTime;
	float TotalDisplayTime;
	bool bIsDismissing;
	bool bIsShowing;

	void BuildDynamicUIIfNeeded();
	void FinishDismiss();
	FLinearColor GetColorForNoticeType(EWildTimingNoticeType InType) const;
	FText GetDefaultTitleForNoticeType(EWildTimingNoticeType InType) const;
	FText GetDefaultMessageForNoticeType(EWildTimingNoticeType InType) const;
};
