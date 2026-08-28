// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Effect/EffectBase.h"

UWorld* UEffectBase::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	if (UObject* Outer = GetOuter())
	{
		return Outer->GetWorld();
	}

	return nullptr;
}
