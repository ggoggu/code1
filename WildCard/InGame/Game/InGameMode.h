// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "InGameMode.generated.h"

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

	//TODO : Hand Draw �� Unit ���� ���� �����ö� �׽�Ʈ
	int32 GenerateNextCardInstanceID();

	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

protected:
	//UPROPERTY(EditDefaultsOnly, Category = "Manager|Class")
	//TSubclassOf<AWildProtoEffectManager> EffectManagerClass;

	// TODO : Transient�� ���� ���� �ʿ� (���� ��ũ�� ���� ���ص� �Ǵ°͵��� ���ϴ°�, ������ �����)
	//UPROPERTY(Transient)
	

public:
	int32 Server_PlayerCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UPhaseComponent> PhaseComponent;

	//UPROPERTY()
	//TObjectPtr<AWildProtoEffectManager> EffectManagerInstance;

private:
	int32 CurrentCardInstanceID = 123;
	
};
