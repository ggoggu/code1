// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/UI/MulliganViewModel.h"
#include "MulliganCardViewModel.h"

void UMulliganViewModel::SetMulliganActive(bool bActive)
{
	UE_MVVM_SET_PROPERTY_VALUE(bIsMulliganPhaseActive, bActive);
}

void UMulliganViewModel::SetupMulliganCards(const TArray<FWildCardDisplayData>& InCards)
{
	MulliganCards.Empty();
	for (const FWildCardDisplayData& CardData : InCards)
	{
		UMulliganCardViewModel* NewCardVM = NewObject<UMulliganCardViewModel>(this);
		if (NewCardVM)
		{
			NewCardVM->InitializeFromData(CardData);
			MulliganCards.Add(NewCardVM);
		}
	}
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(MulliganCards);
	
	
}

TArray<int32> UMulliganViewModel::GetSelectedCardIDsToSwap() const
{
	TArray<int32> SelectedInstanceIDs;

	// 보유 중인 카드 뷰모델들을 순회하며 선택된 카드만 걸러냅니다.
	for (UMulliganCardViewModel* CardVM : MulliganCards)
	{
		if (CardVM && CardVM->IsSelectedForSwap())
		{
			SelectedInstanceIDs.Add(CardVM->GetInstanceID());
		}
	}

	return SelectedInstanceIDs;
}
