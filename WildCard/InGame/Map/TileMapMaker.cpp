// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Map/TileMapMaker.h"
#include "InGame/Map/TileMap.h"

// Sets default values
ATileMapMaker::ATileMapMaker()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ATileMapMaker::BeginPlay()
{
	Super::BeginPlay();
	
}

ATileMap* ATileMapMaker::CreateMap(FVector Location)
{
    UE_LOG(LogTemp, Warning, TEXT("Class Name: %s"), *GetClass()->GetName());

    if (!WildMapClass)
    {
        UE_LOG(LogTemp, Error, TEXT("WildMapClass is nullptr"));
        return nullptr;
    }
    if (!WildTileClass)
    {
        UE_LOG(LogTemp, Error, TEXT("WildTileClass is nullptr"));
        return nullptr;
    }

    ATileMap* Map = GetWorld()->SpawnActor<ATileMap>(
        WildMapClass,
        Location,
        FRotator::ZeroRotator
    );
    //Map->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
    Map->InitSize(Width, Height);
    for (int r = 0; r < Height; r++)
    {
        for (int c = 0; c < Width; c++)
        {
            Map->AddTile(WildTileClass, TTuple<int, int>(r, c), Size);
        }
    }

    //if (HasAuthority())
    //{
    //    WildMap->SetAllTileRandom();
    //}

    //Map->SetAllTileRandom();

    return Map;
}

