// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Game/InGamePawn.h"

// Sets default values
AInGamePawn::AInGamePawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AInGamePawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AInGamePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AInGamePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AInGamePawn::Server_MoveUnitActor_Implementation(FVector InHasUnitTileLocation, FVector InEmptyTileLocation)
{
}

bool AInGamePawn::Server_MoveUnitActor_Validate(FVector InHasUnitTileLocation, FVector InEmptyTileLocation)
{
	return true;
}

void AInGamePawn::Server_FightUnitActor_Implementation(const FString& InAttacker, const FString& InDefender)
{
}

bool AInGamePawn::Server_FightUnitActor_Validate(const FString& InAttacker, const FString& InDefender)
{
	return true;
}
