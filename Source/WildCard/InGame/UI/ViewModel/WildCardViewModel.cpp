// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/UI/ViewModel/WildCardViewModel.h"

void UWildCardViewModel::InitializeFromData(const FWildCardDisplayData& InDynamicData, const FCardData* InStaticData)
{
	// 1. 서버 동적 데이터 세팅
	UE_MVVM_SET_PROPERTY_VALUE(InstanceID, InDynamicData.InstanceID);
	UE_MVVM_SET_PROPERTY_VALUE(CurrentManaCost, InDynamicData.CurrentManaCost);
	UE_MVVM_SET_PROPERTY_VALUE(Force, InDynamicData.Force);
	UE_MVVM_SET_PROPERTY_VALUE(Intelligence, InDynamicData.Intelligence);
	UE_MVVM_SET_PROPERTY_VALUE(Attack, InDynamicData.Attack);
	UE_MVVM_SET_PROPERTY_VALUE(Health, InDynamicData.Health);

	// 2. 데이터 테이블 정적 데이터 세팅
	if (InStaticData)
	{
		UE_MVVM_SET_PROPERTY_VALUE(CardName, InStaticData->CardName);
		UE_MVVM_SET_PROPERTY_VALUE(CardImage, InStaticData->GetCardImage());
		UE_MVVM_SET_PROPERTY_VALUE(CardFrameImage, InStaticData->GetCardFrameImage());
		UE_MVVM_SET_PROPERTY_VALUE(CardType, InStaticData->CardType);
	}
}
