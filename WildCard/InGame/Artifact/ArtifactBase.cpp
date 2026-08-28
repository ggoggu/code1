// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Artifact/ArtifactBase.h"
#include "EffectBase.h"

UArtifactBase::UArtifactBase()
{
	ArtifactID = NAME_None;
	CurrentDurability = 0;
	MaxDurability = 0;
	bIsActive = false;
}

void UArtifactBase::InitializeArtifact(FName InArtifactID, int32 InMaxDurability,
	const TArray<TSubclassOf<UEffectBase>>& InEffectClasses)
{
	ArtifactID = InArtifactID;
	MaxDurability = InMaxDurability;
	CurrentDurability = InMaxDurability;
	bIsActive = true;

	ArtifactEffects.Empty();
	for (TSubclassOf<UEffectBase> EffectClass : InEffectClasses)
	{
		if (EffectClass)
		{
			UEffectBase* NewEffect = NewObject<UEffectBase>(this, EffectClass);
			ArtifactEffects.Add(NewEffect);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Artifact] 성유물 로직 객체 생성 완료: %s (초기 내구도: %d/%d)"), 
		*ArtifactID.ToString(), CurrentDurability, MaxDurability);
}

void UArtifactBase::UseArtifact(const FWildActionData& ActionData)
{
	if (!bIsActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Artifact] 이미 비활성화된 성유물(%s)은 사용할 수 없습니다."), *ArtifactID.ToString());
		return;
	}

	if (CurrentDurability > 0)
	{
		// [수정됨] 보유한 모든 Effect를 순회하며 실행합니다.
		for (UEffectBase* Effect : ArtifactEffects)
		{
			if (Effect && Effect->TriggerTiming == EWildEffectTriggerTiming::Instant)
			{
				Effect->ExecuteEffect(ActionData);
			}
		}

		CurrentDurability--;
		UE_LOG(LogTemp, Log, TEXT("[Artifact] 성유물 %s 사용됨. 남은 내구도: %d"), *ArtifactID.ToString(), CurrentDurability);

		if (CurrentDurability <= 0)
		{
			DeactivateArtifact();
		}
	}
}

void UArtifactBase::DeactivateArtifact()
{
	if (bIsActive)
	{
		bIsActive = false;
		CurrentDurability = 0;
		UE_LOG(LogTemp, Warning, TEXT("[Artifact] 성유물 %s 의 내구도가 모두 소진되어 무효화(비활성화)되었습니다."), *ArtifactID.ToString());
		
		// 여기에 추후 PlayerState를 통해 클라이언트 UI(MVVM)에 동기화 패킷을 쏴주는 알림 코드가 연동될 예정입니다.
	}
}
