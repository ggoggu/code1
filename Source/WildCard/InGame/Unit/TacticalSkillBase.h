// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "InGame/Data/BattleData.h"
#include "TacticalSkillBase.generated.h"

class APlayerUnit;
class ATile;
class AInGameController;
class UEffectBase;

UCLASS(Blueprintable, Abstract)
class WILDCARD_API UTacticalSkillBase : public UObject
{
	GENERATED_BODY()

public:
	UTacticalSkillBase();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactical Skill")
	FName SkillID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactical Skill")
	FText SkillName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactical Skill")
	FText SkillDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactical Skill")
	EWildPlayerClass RequiredClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactical Skill")
	int32 ManaCost;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactical Skill")
	int32 Range;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactical Skill")
	ETacticalSkillTargetType TargetType;

	// 스킬 효과 클래스 (UEffectBase 연동을 통한 확장 지원)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactical Skill")
	TSubclassOf<UEffectBase> SkillEffectClass;

	// 시각/UI 연출 식별 태그 (GameplayTag)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactical Skill|Visual")
	FGameplayTag VisualCueTag;

	// 클라이언트 측 전술 스킬 발동/타겟팅 시작 진입점 (스킬별 다양한 타겟팅/UI 진입을 다형성으로 처리)
	virtual void InitiateSkill(AInGameController* PC, APlayerUnit* SourceUnit);

	// 스킬 발동 시작 가능 여부 (UI 버튼 활성화 여부 판정 등)
	virtual bool CanInitiate(APlayerUnit* SourceUnit) const;

	virtual bool CanActivate(APlayerUnit* SourceUnit, ATile* TargetTile, bool bCheckMana = true) const;
	virtual bool CanActivateAction(APlayerUnit* SourceUnit, const FWildActionData& ActionData, bool bCheckMana = true) const;

	virtual void GetValidTargetTiles(APlayerUnit* SourceUnit, TArray<ATile*>& OutTiles) const;

	virtual bool Execute(APlayerUnit* SourceUnit, ATile* TargetTile);
	virtual bool ExecuteAction(APlayerUnit* SourceUnit, const FWildActionData& ActionData);

	// 연출 큐에 전달할 기본 수치 (데미지/회복량 등)
	virtual int32 GetSkillNumericValue() const { return 0; }

	// 체인 해결 시 클라이언트 플레이어의 비동기 추가 선택이 필요한지 여부 (예: 패에서 되돌릴 카드 선택 등)
	virtual bool RequiresPlayerPromptOnResolve(const FWildActionData& ActionData) const { return false; }

	// 체인 해결 시 클라이언트 플레이어에게 비동기 선택 프롬프트 요청
	virtual void PromptPlayerOnResolve(class AInGameController* PC, const FWildActionData& ActionData) {}

	UFUNCTION(BlueprintNativeEvent, Category = "Tactical Skill")
	bool OnExecute(APlayerUnit* SourceUnit, ATile* TargetTile);
	virtual bool OnExecute_Implementation(APlayerUnit* SourceUnit, ATile* TargetTile);

	static TSubclassOf<UTacticalSkillBase> GetDefaultSkillClassForPlayerClass(EWildPlayerClass InClass);
};

// 전사 전술 스킬 (강타 - 인접 적 유닛에게 물리 피해)
UCLASS(Blueprintable)
class WILDCARD_API UWarriorTacticalSkill : public UTacticalSkillBase
{
	GENERATED_BODY()

public:
	UWarriorTacticalSkill();

	virtual bool CanActivate(APlayerUnit* SourceUnit, ATile* TargetTile, bool bCheckMana = true) const override;
	virtual bool Execute(APlayerUnit* SourceUnit, ATile* TargetTile) override;
	virtual int32 GetSkillNumericValue() const override { return DamageAmount; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Warrior Tactical Skill")
	int32 DamageAmount;
};

// 마법사 전술 스킬 (비전 탄막 - 사거리 내 적 타일/유닛에게 마법 피해)
UCLASS(Blueprintable)
class WILDCARD_API UMageTacticalSkill : public UTacticalSkillBase
{
	GENERATED_BODY()

public:
	UMageTacticalSkill();

	virtual bool CanActivate(APlayerUnit* SourceUnit, ATile* TargetTile, bool bCheckMana = true) const override;
	virtual bool Execute(APlayerUnit* SourceUnit, ATile* TargetTile) override;
	virtual int32 GetSkillNumericValue() const override { return SpellDamage; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mage Tactical Skill")
	int32 SpellDamage;
};


// 도사 전술 스킬 (회복의 진 - 자신 또는 아군 체력 회복)
UCLASS(Blueprintable)
class WILDCARD_API UTaoistTacticalSkill : public UTacticalSkillBase
{
	GENERATED_BODY()

public:
	UTaoistTacticalSkill();

	virtual bool CanActivate(APlayerUnit* SourceUnit, ATile* TargetTile, bool bCheckMana = true) const override;
	virtual bool Execute(APlayerUnit* SourceUnit, ATile* TargetTile) override;
	virtual int32 GetSkillNumericValue() const override { return HealAmount; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Taoist Tactical Skill")
	int32 HealAmount;
};
