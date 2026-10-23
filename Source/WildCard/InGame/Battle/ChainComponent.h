// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BattleData.h"
#include "ChainComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChainUpdated, int32, CurrentStackCount);

class AWildActionPreviewArrow;
class AWildPreviewGhostUnit;

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
	void ResolveNextChainAction();
	/** 체인 해결 시 추가 입력 프롬프트가 필요한 전술 스킬인지 확인 (다형성 위임) */
	bool IsPromptingSkillAction(const FWildActionData& ActionData) const;
	void ResolvePromptingSkillAction(const FWildActionData& ActionData);

	// 하위 호환용 인라인 래퍼
	FORCEINLINE bool IsSearchLibraryAction(const FWildActionData& ActionData) const { return IsPromptingSkillAction(ActionData); }
	FORCEINLINE void ResolveSearchLibraryAction(const FWildActionData& ActionData) { ResolvePromptingSkillAction(ActionData); }
	void ExecuteSearchLibrarySwap(class AInGamePlayerState* PS, int32 SelectedCardInstanceID);
	void OnSearchLibrarySelectionConfirmed(int32 PlayerID, int32 SelectedCardInstanceID);

	UFUNCTION()
	void OnSearchLibraryTimeout();
	void DeclareAction(const FWildActionData& ActionData);
	void ProcessNextMainAction();
	int32 GetActionStackNum() const { return ActionCount; }

	void ClearPublicMainActions();
	void UpdatePublicPreviews();
	void ClearPublicPreviews();

	const TArray<FWildActionData>& GetPublicMainActions() const { return PublicMainActions; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(ReplicatedUsing = OnRep_PublicMainActions)
	TArray<FWildActionData> PublicMainActions;

	UFUNCTION()
	void OnRep_PublicMainActions();

	UPROPERTY()
	TArray<TObjectPtr<AActor>> ActivePreviewActors;

	UPROPERTY()
	TArray<TObjectPtr<AWildActionPreviewArrow>> PooledPreviewArrows;

	UPROPERTY()
	TArray<TObjectPtr<AWildPreviewGhostUnit>> PooledGhostUnits;

	AWildActionPreviewArrow* GetOrCreatePooledArrow();
	AWildPreviewGhostUnit* GetOrCreatePooledGhostUnit();

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

	// 첫 번째 소환 시 일반 유닛 대신 소환될 Player BP 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Player")
	TSubclassOf<class APlayerUnit> PlayerUnitClass;

	// 첫 번째 소환을 Player BP로 소환할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Player")
	bool bSpawnPlayerUnitOnFirstSummon = false;

	// 미리보기 고스트 유닛의 불투명도 (0.0: 완전 투명, 1.0: 완전 불투명)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Visual", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float PreviewGhostOpacity = 0.35f;

	// 미리보기 고스트 유닛의 탈색/회색조 비중 (0.0: 원본 색상, 1.0: 완전한 회색 흑백)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Visual", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float PreviewGhostDesaturation = 1.0f;

	// 미리보기 고스트 유닛에 곱해지는 회색 틴트 색상
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview|Visual")
	FLinearColor PreviewGhostTintColor = FLinearColor(0.35f, 0.35f, 0.35f, 1.0f);

	FTimerHandle ChainTimerHandle;

	UPROPERTY()
	bool bAwaitingSearchLibrarySelection = false;

	UPROPERTY()
	FWildActionData CurrentSearchLibraryAction;

	FTimerHandle SearchLibraryTimeoutTimerHandle;

	UFUNCTION()
	void StartChainWindowForCurrentAction(int32 PromptPlayerID);

	UFUNCTION()
	void InitiateMainActionResolution();

	void CheckAndResolveMainActionConflicts(FWildActionData& FirstAction, FWildActionData& SecondAction, class AInGamePlayerState* FirstPS, class AInGamePlayerState* SecondPS);

	void ProcessMiddlePhase();

	void ExecuteMainActionsSimultaneously();
	
	UFUNCTION()
	void OnRep_ActionStack();

	UFUNCTION()
	void OnServerPhaseChanged(EWildCardPhase NewPhase);

private:
	void ExecuteAction(const FWildActionData& ActionData);
	
};
