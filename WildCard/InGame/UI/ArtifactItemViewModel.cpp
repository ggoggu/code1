// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/UI/ArtifactItemViewModel.h"

void UArtifactItemViewModel::InitializeFromData(const FArtifactData& InData)
{
	UE_MVVM_SET_PROPERTY_VALUE(ArtifactID, InData.ArtifactID);
	UE_MVVM_SET_PROPERTY_VALUE(ArtifactName, InData.ArtifactName);
	UE_MVVM_SET_PROPERTY_VALUE(EffectText, InData.EffectText);
	UE_MVVM_SET_PROPERTY_VALUE(ArtifactImage, InData.ArtifactImage);
	UE_MVVM_SET_PROPERTY_VALUE(bIsSelected, false);
}


void UArtifactItemViewModel::ToggleSelection()
{
	UE_MVVM_SET_PROPERTY_VALUE(bIsSelected, !bIsSelected);
}