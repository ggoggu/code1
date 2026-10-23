// Fill out your copyright notice in the Description page of Project Settings.


#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InGame/Data/BattleData.h"
#include "SelectActionWidget.generated.h"

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActionMenuButtonPressed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSelectActionPressed);


class UButton;
class UTextBlock;
class APlayerUnit;
class AUnitBase;
enum class EWildCardPhase : uint8;

UCLASS()
class WILDCARD_API USelectActionWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Move;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Combat;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Effect;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_TacticalSkill;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Move;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_TacticalSkill;

	UFUNCTION()
	void OnMoveButtonClicked();

	UFUNCTION()
	void OnCombatButtonClicked();

	UFUNCTION()
	void OnEffectButtonClicked();

	UFUNCTION()
	void OnTacticalSkillButtonClicked();

public:
	UPROPERTY(BlueprintAssignable, Category = "Action")
	FOnSelectActionPressed OnMoveButtonPressed;

	UPROPERTY(BlueprintAssignable, Category = "Action")
	FOnSelectActionPressed OnCombatButtonPressed;

	UPROPERTY(BlueprintAssignable, Category = "Action")
	FOnSelectActionPressed OnEffectButtonPressed;

	UPROPERTY(BlueprintAssignable, Category = "Action")
	FOnSelectActionPressed OnTacticalSkillButtonPressed;

	UFUNCTION(BlueprintCallable, Category = "Action")
	void SetupForPlayerUnit(APlayerUnit* InPlayerUnit, EWildCardPhase CurrentPhase, bool bHasChainPriority);

	UFUNCTION(BlueprintCallable, Category = "Action")
	void SetupForGeneralUnit(AUnitBase* InUnit, EWildCardPhase CurrentPhase = EWildCardPhase::None, bool bHasChainPriority = false);

	UFUNCTION(BlueprintCallable, Category = "Action")
	void SetPositionFromWorldLocation(FVector WorldLocation);
};
