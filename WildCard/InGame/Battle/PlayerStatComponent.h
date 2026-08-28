// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerStatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChangedSignature, int32, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnManaChangedSignature, int32, NewMana);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelChangedSignature, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnForceChangedSignature, int32, CurrentForce, int32, MaxForce);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class WILDCARD_API UPlayerStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPlayerStatComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category = "Stats|Events")
	FOnHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Stats|Events")
	FOnManaChangedSignature OnManaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Stats|Events")
	FOnLevelChangedSignature OnLevelChanged;

	UPROPERTY(BlueprintAssignable, Category = "Stats|Events")
	FOnForceChangedSignature OnForceChanged;

	void TakeDamage(int32 DamageAmount);
	void Heal(int32 HealAmount);
	
	bool ConsumeMana(int32 Amount);
	void AddMana(int32 Amount);

	bool ConsumeForce(int32 Amount);
	void RefreshForce();
	void AddLevel(int32 Amount);

	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Stats")
	int32 Health;

	UPROPERTY(ReplicatedUsing = OnRep_Mana, BlueprintReadOnly, Category = "Stats")
	int32 Mana;

	UPROPERTY(ReplicatedUsing = OnRep_Level, BlueprintReadOnly, Category = "Stats")
	int32 PlayerLevel;

	UPROPERTY(ReplicatedUsing = OnRep_MaxForce, BlueprintReadOnly, Category = "Stats")
	int32 MaxForce;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentForce, BlueprintReadOnly, Category = "Stats")
	int32 CurrentForce;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION() virtual void OnRep_Health();
	UFUNCTION() virtual void OnRep_Mana();
	UFUNCTION() virtual void OnRep_Level();
	UFUNCTION() virtual void OnRep_MaxForce();
	UFUNCTION() virtual void OnRep_CurrentForce();
		
};
