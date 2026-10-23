// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BattleData.h"
#include "WildVisualEffectWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVisualEffectFinishedSignature);

/**
 * 유닛/카드 효과 발동 시 생성되는 개별 독립 연출 위젯의 베이스 클래스입니다.
 * 1가지 연출 = 1개의 독립 WBP 에셋으로 제작하여 협업 시 Git 충돌을 원천 차단합니다.
 */
UCLASS(Blueprintable)
class WILDCARD_API UWildVisualEffectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "VisualEffect")
	FOnVisualEffectFinishedSignature OnVisualEffectFinished;

	/** 연출 데이터 수신 및 재생 시작 함수 */
	UFUNCTION(BlueprintCallable, Category = "VisualEffect")
	virtual void PlayVisualEffect(const FWildVisualCueData& InCueData);

	/** 연출 애니메이션 종료 시 블루프린트 또는 C++에서 호출하여 스스로 부모에서 제거 */
	UFUNCTION(BlueprintCallable, Category = "VisualEffect")
	virtual void FinishVisualEffect();

	UFUNCTION(BlueprintPure, Category = "VisualEffect")
	const FWildVisualCueData& GetCueData() const { return CachedCueData; }

protected:
	/** 블루프린트에서 UMG 애니메이션이나 사운드 연출을 자유롭게 구현할 수 있는 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "VisualEffect")
	void BP_OnPlayVisualEffect(const FWildVisualCueData& InCueData);

	/** 연출 종료 시 호출되는 블루프린트 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "VisualEffect")
	void BP_OnVisualEffectFinished();

	UPROPERTY(BlueprintReadOnly, Category = "VisualEffect")
	FWildVisualCueData CachedCueData;

	/** 비정상적으로 연출 종료(FinishVisualEffect)가 호출되지 않았을 때 자가 소멸하는 안전 제한 시간 (초, 0 이하면 비활성화) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VisualEffect")
	float SafetyAutoFinishDuration = 3.0f;

	FTimerHandle AutoFinishTimerHandle;
};

/**
 * 유닛 이동(Move) 연출을 담당하는 독립 위젯 클래스
 */
UCLASS(Blueprintable)
class WILDCARD_API UWildVisualEffectMoveWidget : public UWildVisualEffectWidget
{
	GENERATED_BODY()

public:
	virtual void PlayVisualEffect(const FWildVisualCueData& InCueData) override;

protected:
	/** 이동 연출 전용 블루프린트 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "VisualEffect|Move")
	void BP_OnMoveVisualEffect(AActor* MovingUnit, int32 FromQ, int32 FromR, int32 ToQ, int32 ToR);
};

/**
 * 유닛 공격(Attack) 및 전투 연출을 담당하는 독립 위젯 클래스
 */
UCLASS(Blueprintable)
class WILDCARD_API UWildVisualEffectAttackWidget : public UWildVisualEffectWidget
{
	GENERATED_BODY()

public:
	virtual void PlayVisualEffect(const FWildVisualCueData& InCueData) override;

protected:
	/** 공격 연출 전용 블루프린트 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "VisualEffect|Attack")
	void BP_OnAttackVisualEffect(AActor* Attacker, AActor* Defender, int32 DamageAmount);
};

