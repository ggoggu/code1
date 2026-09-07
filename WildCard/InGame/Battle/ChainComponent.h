// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BattleData.h"
#include "ChainComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChainUpdated, int32, CurrentStackCount);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class WILDCARD_API UChainComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UChainComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(BlueprintAssignable) FOnChainUpdated OnChainUpdatedDelegate;
	
	void AddActionToChain(const FWildActionData& NewAction);
	void ResolveChain();
	void DeclareAction(const FWildActionData& ActionData);
	void ProcessNextMainAction();

protected:
	UPROPERTY()
	TArray<FWildActionData> MainActionQueue;

	UPROPERTY()
	TArray<FWildActionData> PendingResolutionMainActions;
	
	UPROPERTY(ReplicatedUsing = OnRep_ActionStack)
	TArray<FWildActionData> ActionStack;
	
	UPROPERTY(Replicated)
	int32 ActionCount;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	TSubclassOf<AUnitBase> UnitClass;

	void ProcessMiddlePhase();

	void ExecuteMainActionsSimultaneously();
	
	UFUNCTION()
	void OnRep_ActionStack();

	UFUNCTION()
	void OnServerPhaseChanged(EWildCardPhase NewPhase);

private:
	void ExecuteAction(const FWildActionData& ActionData);
	
};
