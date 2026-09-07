// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/UI/SelectActionWidget.h"
#include "Components/Button.h"
#include "InGameController.h"
#include "InGamePlayerState.h"
#include "SelectActionWidget.h"

void USelectActionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Move)
	{
		Button_Move->OnClicked.AddDynamic(this, &USelectActionWidget::OnMoveButtonClicked);
	}
	if (Button_Combat)
	{
		Button_Combat->OnClicked.AddDynamic(this, &USelectActionWidget::OnCombatButtonClicked);
	}
	if (Button_Effect)
	{
		Button_Effect->OnClicked.AddDynamic(this, &USelectActionWidget::OnEffectButtonClicked);
	}
}

void USelectActionWidget::OnMoveButtonClicked()
{
	OnMoveButtonPressed.Broadcast();
	AInGameController* PC = Cast<AInGameController>(GetOwningLocalPlayer());
	int32 PID = PC->GetPlayerState<AInGamePlayerState>()->GetPlayerID();
	UE_LOG(LogTemp, Log, TEXT("USelectActionWidget::OnMoveButtonClicked - PlayerID: %d"), PID);
}

void USelectActionWidget::OnCombatButtonClicked()
{
	OnCombatButtonPressed.Broadcast();
	AInGameController* PC = Cast<AInGameController>(GetOwningLocalPlayer());
	int32 PID = PC->GetPlayerState<AInGamePlayerState>()->GetPlayerID();
	UE_LOG(LogTemp, Log, TEXT("USelectActionWidget::OnCombatButtonClicked - PlayerID: %d"), PID);
}

void USelectActionWidget::OnEffectButtonClicked()
{
	OnEffectButtonPressed.Broadcast();
	AInGameController* PC = Cast<AInGameController>(GetOwningLocalPlayer());
	int32 PID = PC->GetPlayerState<AInGamePlayerState>()->GetPlayerID();
	UE_LOG(LogTemp, Log, TEXT("USelectActionWidget::OnEffectButtonClicked - PlayerID: %d"), PID);
}
