// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/UI/MulliganCardViewModel.h"
#include "WildCardGameInstance.h"
#include "Engine/DataTable.h"

void UMulliganCardViewModel::InitializeFromData(const FWildCardDisplayData& InData)
{
	UE_MVVM_SET_PROPERTY_VALUE(InstanceID, InData.InstanceID);
	UE_MVVM_SET_PROPERTY_VALUE(Force, InData.Force);
	UE_MVVM_SET_PROPERTY_VALUE(Intelligence, InData.Intelligence);
	UE_MVVM_SET_PROPERTY_VALUE(Attack, InData.Attack);
	UE_MVVM_SET_PROPERTY_VALUE(Health, InData.Health);
	UE_MVVM_SET_PROPERTY_VALUE(bIsSelectedForSwap, false);

	// [중요] 데이터 테이블에서 진짜 이름을 가져와야 "이상한 글자"가 사라집니다.
	UWorld* World = GetWorld();
	if (World)
	{
		UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(World->GetGameInstance());
		if (GI && GI->CardDataTable)
		{
			FCardData* StaticData = GI->CardDataTable->FindRow<FCardData>(InData.CardID, TEXT("MulliganContext"));
			if (StaticData)
			{
				// RowName(NewRow_1)이 아닌 진짜 이름(화염구 등)을 주입합니다.
				UE_MVVM_SET_PROPERTY_VALUE(CardName, StaticData->CardName);
				UE_MVVM_SET_PROPERTY_VALUE(CardImage, StaticData->CardImage);
				UE_MVVM_SET_PROPERTY_VALUE(CardFrameImage, StaticData->CardFrameImage);
			}
		}
	}
}

void UMulliganCardViewModel::ToggleSelection()
{
	UE_LOG(LogTemp, Display, TEXT("ToggleSelection"));
	UE_MVVM_SET_PROPERTY_VALUE(bIsSelectedForSwap, !bIsSelectedForSwap);
}
