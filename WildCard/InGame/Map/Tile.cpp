// Fill out your copyright notice in the Description page of Project Settings.

#include "InGame/Map/Tile.h"
#include "Net/UnrealNetwork.h"
#include "InGame/Unit/UnitBase.h"

// Sets default values
ATile::ATile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	SetActorEnableCollision(true);
}

// Called when the game starts or when spawned
void ATile::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ATile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATile::SpawnUnit(FCardData InCardData, TSubclassOf<AUnitBase> UnitClass, int32 InPlayerCount)
{
	if (!HasAuthority() || !UnitClass) return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	AUnitBase* NewUnit = GetWorld()->SpawnActor<AUnitBase>(
		UnitClass,
		GetActorLocation(),
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (NewUnit)
	{
		NewUnit->SetCurrentTile(this);
		NewUnit->InitUnitCardData(InCardData, InPlayerCount);
		SetPlacedUnit(NewUnit);
	}
}

void ATile::MoveToUnit(AUnitBase* Unit, FVector PositionToMove)
{
}

void ATile::OnRep_TileState()
{
	UE_LOG(LogTemp, Warning, TEXT("Tile State Replicated: %d"), static_cast<int32>(TileState));
}

void ATile::SetUnitMoveToTileInBP(FVector InHasUnitTileLocation, FVector InEmptyTileLocation)
{
}

void ATile::SetUnitAttackTargetInBP(const FString& InAttacker, const FString& InDefender)
{
}

bool ATile::HasOwnerIDtoActionUnit(int32 InOwnerId)
{
	return false;
}

void ATile::OnRep_MoveUnit()
{
}

void ATile::OnRep_Axial()
{
}

void ATile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATile, TileState);
	DOREPLIFETIME(ATile, PlacedUnit);
	DOREPLIFETIME(ATile, Axial);
	DOREPLIFETIME(ATile, PlayerController);
}
