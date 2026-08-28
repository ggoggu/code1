// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BattleData.h"
#include "InGameController.generated.h"

class UWildCardViewModel;
/**
 * 
 */

UCLASS()
class WILDCARD_API AInGameController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void CompleteMulligan();

	UFUNCTION(BlueprintCallable)
	void PlayChainCard(const FWildActionData& ActionData);

	UFUNCTION(BlueprintCallable)
	void PassChain();

	UFUNCTION(BlueprintCallable)
	void CompleteGameset();

	UFUNCTION(BlueprintCallable)
	void CompletePrivateAction(const FWildActionData& ActionData);

	void RefreshMulliganUI(const TArray<FWildCardDisplayData>& NewCards);

protected:

	FTimerHandle BindTimer;
	
	virtual void BeginPlay() override;

	// 다양한 케이스 테스트용으로 불피요시 지울 수 ㅣㅇㅆ음
	void TryBindGameState();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SubmitAction(FWildActionData ActionData);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CompleteMulligan();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CompleteGameset();
	
	UFUNCTION()
	void OnMatchStateChanged(EWildMatchState NewState);
	
	UFUNCTION()
	void OnGamePhaseChanged(EWildCardPhase NewPhase);
	
	// Debug
public:
	UFUNCTION(BlueprintCallable, Category = "Debug|Phase")
	void Debug_Ready();
	
	UFUNCTION(BlueprintCallable, Category = "Debug|Phase")
	void Debug_Pass();

	UFUNCTION(BlueprintCallable, Category = "Debug|Phase")
	void Debug_Chain();

	// Widget
public:
	void AttemptPlayCard(UWildCardViewModel* CardViewModel, FVector2D ScreenPosition);
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> MulliganWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UWildHUDWidget> HUDClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> GamesetWidgetClass;

	FTimerHandle EnemyStateBindTimer;
	void TryBindEnemyState();

	UFUNCTION()
	void OnEnemyHandCountUpdated(int32 NewHandCount);

private:
	UPROPERTY()
	class UUserWidget* MulliganWidgetInstance;
	
	UPROPERTY()
	class UWildHUDWidget* HUDInstance;

	UPROPERTY()
	class UUserWidget* GamesetWidgetInstance;

	UPROPERTY()
	class UWildHUDViewModel* MainHUDViewModel;
	
	void ShowMulliganUI();
	void HideMulliganUI();
	void CreateHUD();
	
	void ShowGamesetUI();
	void HideGamesetUI();

	UFUNCTION()
	void OnTimerUpdated(int32 RemainTime);

	UFUNCTION()
	void UpdateHandFromState(const TArray<FWildCardDisplayData>& HandData);
	
};
