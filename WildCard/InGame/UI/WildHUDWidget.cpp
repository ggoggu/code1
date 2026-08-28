// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/UI/WildHUDWidget.h"
#include "WildHandWidget.h"
#include "WildHUDViewModel.h"
#include "WildDragDropOperation.h"
#include "WildCardViewModel.h"
#include "InGameController.h"
#include "WildEnemyHandWidget.h"

void UWildHUDWidget::InitializeHUD(UWildHUDViewModel* InViewModel)
{
	if (!InViewModel) 
	{
		return;
	}

	HUDViewModel = InViewModel;

	// 하위 위젯인 손패 UI에 뷰모델을 전달하여 스스로 초기화하도록 명령합니다.
	if (HandWidget)
	{
		HandWidget->InitializeHandWidget(HUDViewModel);
	}

	if (EnemyHandWidget)
	{
		EnemyHandWidget->InitializeEnemyHandWidget(HUDViewModel);
	}
}
