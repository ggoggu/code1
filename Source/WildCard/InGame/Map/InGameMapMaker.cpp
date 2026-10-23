// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Map/InGameMapMaker.h"
#include "Tile.h"
#include "InGameMap.h"

// Sets default values
AInGameMapMaker::AInGameMapMaker()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

AInGameMap* AInGameMapMaker::CreateMap(FVector Location)
{
	UE_LOG(LogTemp, Warning, TEXT("Class Name: %s"), *GetClass()->GetName());
	//#. �������� ����
	if (!HasAuthority())
	{
		return nullptr;
	}

	if (!WildMapClass)
	{
		return nullptr;
	}
	if (!WildTileClass)
	{
		return nullptr;
	}
    
	AInGameMap* WildMap = GetWorld()->SpawnActor<AInGameMap>(
	   WildMapClass,
	   Location,
	   FRotator::ZeroRotator
	);
	//WildMap->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	WildMap->InitSize(Width, Height);
	for (int r = 0; r < Height; r++)
	{
		for (int c = 0; c < Width; c++)
		{

			WildMap->AddTile(WildTileClass, TTuple<int, int>(r, c), Size);

		}
	}

	//if (HasAuthority())
	//{
	//    WildMap->SetAllTileRandom();
	//}

	//WildMap->SetAllTileRandom();

	return WildMap;
}

// Called when the game starts or when spawned
void AInGameMapMaker::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("Root Relative: %s"),
	   *GetRootComponent()->GetRelativeLocation().ToString());
	
}

