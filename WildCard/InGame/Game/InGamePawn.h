// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InGamePawn.generated.h"

UCLASS()
class WILDCARD_API AInGamePawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AInGamePawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveUnitActor(FVector InHasUnitTileLocation, FVector InEmptyTileLocation);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_FightUnitActor(const FString& InAttacker, const FString& InDefender);

};
