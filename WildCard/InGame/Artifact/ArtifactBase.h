// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BattleData.h"
#include "ArtifactBase.generated.h"

/**
 * 
 */
UCLASS()
class WILDCARD_API UArtifactBase : public UObject
{
	GENERATED_BODY()

public:
	UArtifactBase();

	// 데이터 테이블 정보를 바탕으로 서버에서 성유물 초기화
	UFUNCTION(BlueprintCallable, Category = "Artifact")
	virtual void InitializeArtifact(FName InArtifactID, int32 InMaxDurability, const TArray<TSubclassOf<class UEffectBase>>& InEffectClasses);

	// 성유물 효과 발동 시도 (서버 권한에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Artifact")
	virtual void UseArtifact(const FWildActionData& ActionData);

	// 내구도가 0이 되거나 특정 효과로 인해 성유물이 무효화될 때 처리
	UFUNCTION(BlueprintCallable, Category = "Artifact")
	virtual void DeactivateArtifact();
	


public:
	FORCEINLINE FName GetArtifactID() const { return ArtifactID; }
	FORCEINLINE int32 GetCurrentDurability() const { return CurrentDurability; }
	FORCEINLINE int32 GetMaxDurability() const { return MaxDurability; }
	FORCEINLINE bool IsArtifactActive() const { return bIsActive; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Artifact|State")
	FName ArtifactID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Artifact|State")
	int32 CurrentDurability;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Artifact|State")
	int32 MaxDurability;

	// 성유물의 활성화 상태 여부 (내구도가 0이 되면 false)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Artifact|State")
	bool bIsActive;

	UPROPERTY(VisibleAnywhere, Instanced, Category = "Artifact|Effects")
	TArray<UEffectBase*> ArtifactEffects;
	
};
