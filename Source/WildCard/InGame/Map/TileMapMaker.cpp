// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Map/TileMapMaker.h"
#include "InGame/Map/TileMap.h"
#include "InGame/Map/TileLayoutDataAsset.h"

// Sets default values
ATileMapMaker::ATileMapMaker()
{
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ATileMapMaker::BeginPlay()
{
	Super::BeginPlay();
	
}

ATileMap* ATileMapMaker::CreateMap(FVector Location)
{
    if (!HasAuthority())
    {
        return nullptr;
    }

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

    if (!IsValid(LayoutData))
    {
        UE_LOG(LogTemp, Error, TEXT("TileMapMaker requires a LayoutData asset"));
        return nullptr;
    }

    const int32 LayoutWidth = LayoutData->Width;
    const int32 LayoutHeight = LayoutData->Height;
    if (LayoutWidth <= 0 || LayoutHeight <= 0 ||
        static_cast<int64>(LayoutWidth) * LayoutHeight > MAX_int32 ||
        !FMath::IsFinite(Size) || Size <= 0.f)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid tile map dimensions or tile size"));
        return nullptr;
    }

	//TODO : 맵 생성 이후 추가로 타일이 늘어날것을 대비해 seclectedCells를 전역변수로 변경 후 추가 해야함
    TSet<FIntPoint> SelectedCells;
    for (const FIntPoint& Cell : LayoutData->EnabledCells)
    {
        // Resizing the editor grid may leave saved cells outside its bounds.
        if (Cell.X >= 0 && Cell.X < LayoutHeight &&
            Cell.Y >= 0 && Cell.Y < LayoutWidth)
        {
            SelectedCells.Add(Cell);
        }
    }

    ATileMap* Map = GetWorld()->SpawnActor<ATileMap>(
        WildMapClass,
        Location,
        FRotator::ZeroRotator
    );

    if (!IsValid(Map))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn tile map"));
        return nullptr;
    }
    Map->SetPlayerStartCells(LayoutData->Player1Start, LayoutData->Player2Start);

    Map->InitSize(LayoutWidth, LayoutHeight);
    for (int32 r = 0; r < LayoutHeight; r++)
    {
        for (int32 c = 0; c < LayoutWidth; c++)
        {
            if (!SelectedCells.Contains(FIntPoint(r, c)))
            {
                continue;
            }

            Map->AddTile(WildTileClass, TTuple<int32, int32>(r, c), Size);

        }
    }

    return Map;
}

