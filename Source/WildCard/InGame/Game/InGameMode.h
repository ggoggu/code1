// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "InGameMode.generated.h"

class APlayerUnit;
class ATile;
class AInGamePlayerState;


/**
 * 
 */
UCLASS()
class WILDCARD_API AInGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AInGameMode();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	int32 GetPlayerCount() const { return Server_PlayerCount; }

	UFUNCTION(BlueprintCallable)
	void SetPlayerCount(int32 NewPlayerCount) { Server_PlayerCount = NewPlayerCount; }

	//TODO : Hand Draw  Unit   ö ׽Ʈ
	int32 GenerateNextCardInstanceID();

	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	// 플레이어 유닛 사망 시 패배 처리 및 매치 종료 판정
	UFUNCTION(BlueprintCallable, Category = "Game")
	void HandlePlayerDefeat(int32 DefeatedPlayerId);

	// 타일에 플레이어 유닛 스폰 헬퍼 함수
	UFUNCTION(BlueprintCallable, Category = "Unit")
	APlayerUnit* SpawnPlayerUnitForPlayer(AInGamePlayerState* InPlayerState, ATile* StartTile, TSubclassOf<APlayerUnit> PlayerUnitClass = nullptr);

	// 게임 시작 시 모든 플레이어의 시작 유닛 자동 배치
	UFUNCTION(BlueprintCallable, Category = "PlayerUnit")
	void SpawnInitialPlayerUnits();

protected:
	//UPROPERTY(EditDefaultsOnly, Category = "Manager|Class")
	//TSubclassOf<AWildProtoEffectManager> EffectManagerClass;

	// TODO : Transient   ʿ ( ũ  ص Ǵ°͵ ϴ°,  )
	//UPROPERTY(Transient)
	

public:
	int32 Server_PlayerCount = 0;

	/** 개발자용: 시작 마나를 10으로 설정할지 여부 (에디터 디테일 패널 및 CLI -DevMana10 지원) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WildCard|Developer")
	bool bDevStartWithTenMana = true;

	/** 개발자용: 시작 마나 수치 (기본값: 10) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WildCard|Developer")
	int32 DevStartingMana = 10;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UPhaseComponent> PhaseComponent;

	//UPROPERTY()
	//TObjectPtr<AWildProtoEffectManager> EffectManagerInstance;

private:
	int32 CurrentCardInstanceID = 123;
	
};
