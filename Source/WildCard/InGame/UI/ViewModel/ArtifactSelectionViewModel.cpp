// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/UI/ViewModel/ArtifactSelectionViewModel.h"

void UArtifactSelectionViewModel::SetupAvailableArtifacts(const TArray<FArtifactData>& InArtifacts)
{
	AvailableArtifacts.Empty();
	for (const FArtifactData& ArtifactData : InArtifacts)
	{
		UArtifactItemViewModel* NewItemVM = NewObject<UArtifactItemViewModel>(this);
		if (NewItemVM)
		{
			NewItemVM->InitializeFromData(ArtifactData);
			AvailableArtifacts.Add(NewItemVM);
		}
	}
	// 배열 갱신 알림
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(AvailableArtifacts);
}

void UArtifactSelectionViewModel::SelectArtifact(UArtifactItemViewModel* SelectedVM)
{
	if (!SelectedVM) return;

	// 단일 선택 로직: 선택된 항목만 true, 나머지는 false로 해제
	for (UArtifactItemViewModel* ItemVM : AvailableArtifacts)
	{
		if (ItemVM)
		{
			ItemVM->SetbIsSelected(ItemVM == SelectedVM);
		}
	}
}

FName UArtifactSelectionViewModel::GetSelectedArtifactID() const
{
	for (UArtifactItemViewModel* ItemVM : AvailableArtifacts)
	{
		if (ItemVM && ItemVM->GetbIsSelected())
		{
			return ItemVM->GetArtifactID();
		}
	}
	return NAME_None;
}

void UArtifactSelectionViewModel::SetGamesetActive(bool bActive)
{
	UE_MVVM_SET_PROPERTY_VALUE(bIsGamesetActive, bActive);
}
