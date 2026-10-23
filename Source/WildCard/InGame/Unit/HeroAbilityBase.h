// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "InGame/Data/BattleData.h"
#include "HeroAbilityBase.generated.h"

class APlayerUnit;
class ATile;

UENUM(BlueprintType)
enum class EHeroAbilityTargetType : uint8
{
	Self UMETA(DisplayName = "자신/전역"),
	TargetTile UMETA(DisplayName = "대상 타일"),
	TargetEnemyUnit UMETA(DisplayName = "대상 적 유닛"),
	TargetAllyUnit UMETA(DisplayName = "대상 아군 유닛")
};

UCLASS(Blueprintable, Abstract)
class WILDCARD_API UHeroAbilityBase : public UObject
{
	GENERATED_BODY()

public:
	UHeroAbilityBase();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero Ability")
	FText AbilityName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero Ability")
	FText AbilityDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero Ability")
	int32 ManaCost;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero Ability")
	int32 Range;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hero Ability")
	EHeroAbilityTargetType TargetType;

	UPROPERTY(BlueprintReadOnly, Category = "Hero Ability")
	bool bUsedThisTurn;

	virtual bool CanActivate(APlayerUnit* SourceUnit, ATile* TargetTile) const;
	virtual bool Activate(APlayerUnit* SourceUnit, ATile* TargetTile);
	virtual void ResetTurnUsage();

	UFUNCTION(BlueprintNativeEvent, Category = "Hero Ability")
	bool OnActivate(APlayerUnit* SourceUnit, ATile* TargetTile);
	virtual bool OnActivate_Implementation(APlayerUnit* SourceUnit, ATile* TargetTile);

	static TSubclassOf<UHeroAbilityBase> GetDefaultAbilityClassForPlayerClass(EWildPlayerClass InClass);
};

// 전사 영웅 능력 (방패 강타)
UCLASS(Blueprintable)
class WILDCARD_API UWarriorHeroAbility : public UHeroAbilityBase
{
	GENERATED_BODY()

public:
	UWarriorHeroAbility();

	virtual bool CanActivate(APlayerUnit* SourceUnit, ATile* TargetTile) const override;
	virtual bool Activate(APlayerUnit* SourceUnit, ATile* TargetTile) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Warrior Ability")
	int32 DamageAmount;
};

// 마법사 영웅 능력 (화염구)
UCLASS(Blueprintable)
class WILDCARD_API UMageHeroAbility : public UHeroAbilityBase
{
	GENERATED_BODY()

public:
	UMageHeroAbility();

	virtual bool CanActivate(APlayerUnit* SourceUnit, ATile* TargetTile) const override;
	virtual bool Activate(APlayerUnit* SourceUnit, ATile* TargetTile) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mage Ability")
	int32 SpellDamage;
};

// 도사 영웅 능력 (치유의 기운)
UCLASS(Blueprintable)
class WILDCARD_API UTaoistHeroAbility : public UHeroAbilityBase
{
	GENERATED_BODY()

public:
	UTaoistHeroAbility();

	virtual bool CanActivate(APlayerUnit* SourceUnit, ATile* TargetTile) const override;
	virtual bool Activate(APlayerUnit* SourceUnit, ATile* TargetTile) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Taoist Ability")
	int32 HealAmount;
};
