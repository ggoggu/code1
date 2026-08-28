// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/UI/WildHUDViewModel.h"

void UWildHUDViewModel::UpdateMyHandCards(const TArray<UWildCardViewModel*>& InCards)
{
	MyHandCards = InCards;
	
	// 배열 전체가 변경되었음을 UI에 알림 (Hand Widget에서 카드들을 재정렬하도록 유도)
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(MyHandCards);
}
