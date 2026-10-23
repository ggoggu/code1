// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "WildMatchmakingTypes.h"

class AWildDeckSelectPlayerController;

/**
 * 하스스톤 스타일 매칭 대기 및 성사 모달 Slate 위젯
 */
class WILDCARD_API SWildMatchmakingDialogWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWildMatchmakingDialogWidget) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AWildDeckSelectPlayerController>, PlayerController)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** 매칭 상태 변경 수신 */
	void OnMatchmakingStateChanged(EWildMatchmakingState NewState);

	/** 타이머 틱 수신 */
	void OnTimerTick(int32 ElapsedSeconds, int32 RemainingSecondsUntilBot);

	/** 매칭 성공 수신 */
	void OnMatchFound(const FWildMatchResult& MatchResult);

private:
	FReply OnCancelClicked();
	void UpdateWheelAnimationText();

private:
	TWeakObjectPtr<AWildDeckSelectPlayerController> PCWeak;

	TSharedPtr<class SBorder> BackdropBorder;
	TSharedPtr<class SVerticalBox> ContentContainer;

	TSharedPtr<class STextBlock> TitleText;
	TSharedPtr<class STextBlock> SubTitleText;
	TSharedPtr<class STextBlock> RouletteText;
	TSharedPtr<class STextBlock> TimerText;
	TSharedPtr<class SButton> CancelButton;

	// 매칭 성공 시 표시될 영역
	TSharedPtr<class SWidget> MatchFoundWidget;

	int32 RouletteIndex = 0;
};
