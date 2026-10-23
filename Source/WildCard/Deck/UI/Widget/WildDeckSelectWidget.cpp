// Fill out your copyright notice in the Description page of Project Settings.

#include "WildDeckSelectWidget.h"
#include "WildDeckSubsystem.h"
#include "WildDeckSelectPlayerController.h"

void UWildDeckSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWildDeckSubsystem* DeckSub = GI->GetSubsystem<UWildDeckSubsystem>())
		{
			DeckSub->OnDeckSelected.AddDynamic(this, &UWildDeckSelectWidget::HandleDeckSelected);
			DeckSub->OnDeckListChanged.AddDynamic(this, &UWildDeckSelectWidget::HandleDeckListChanged);
		}
	}
}

void UWildDeckSelectWidget::NativeDestruct()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWildDeckSubsystem* DeckSub = GI->GetSubsystem<UWildDeckSubsystem>())
		{
			DeckSub->OnDeckSelected.RemoveDynamic(this, &UWildDeckSelectWidget::HandleDeckSelected);
			DeckSub->OnDeckListChanged.RemoveDynamic(this, &UWildDeckSelectWidget::HandleDeckListChanged);
		}
	}

	Super::NativeDestruct();
}

void UWildDeckSelectWidget::SelectDeck(const FWildDeckData& InDeck)
{
	if (AWildDeckSelectPlayerController* PC = Cast<AWildDeckSelectPlayerController>(GetOwningPlayer()))
	{
		PC->SelectDeck(InDeck);
	}
}

void UWildDeckSelectWidget::RequestStartMatch()
{
	if (AWildDeckSelectPlayerController* PC = Cast<AWildDeckSelectPlayerController>(GetOwningPlayer()))
	{
		PC->RequestStartMatch();
	}
}

void UWildDeckSelectWidget::RequestReturnToLogin()
{
	if (AWildDeckSelectPlayerController* PC = Cast<AWildDeckSelectPlayerController>(GetOwningPlayer()))
	{
		PC->RequestReturnToLogin();
	}
}

TArray<FWildDeckData> UWildDeckSelectWidget::GetAvailableDecks() const
{
	if (AWildDeckSelectPlayerController* PC = Cast<AWildDeckSelectPlayerController>(GetOwningPlayer()))
	{
		if (UWildDeckSubsystem* DeckSub = GetGameInstance()->GetSubsystem<UWildDeckSubsystem>())
		{
			return DeckSub->GetUserDecks(PC->GetActiveUsername());
		}
	}
	return TArray<FWildDeckData>();
}

FWildDeckData UWildDeckSelectWidget::GetCurrentSelectedDeck() const
{
	if (UWildDeckSubsystem* DeckSub = GetGameInstance()->GetSubsystem<UWildDeckSubsystem>())
	{
		return DeckSub->GetSelectedDeck();
	}
	return FWildDeckData();
}

bool UWildDeckSelectWidget::HasValidSelectedDeck() const
{
	if (UWildDeckSubsystem* DeckSub = GetGameInstance()->GetSubsystem<UWildDeckSubsystem>())
	{
		return DeckSub->HasSelectedDeck() && DeckSub->GetSelectedDeck().IsValidDeck();
	}
	return false;
}

void UWildDeckSelectWidget::HandleDeckSelected(const FWildDeckData& NewSelectedDeck)
{
	OnDeckSelectionChanged(NewSelectedDeck);
}

void UWildDeckSelectWidget::HandleDeckListChanged()
{
	OnDeckListUpdated();
}
