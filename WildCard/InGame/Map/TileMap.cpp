// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Map/TileMap.h"
#include "InGame/Map/Tile.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ATileMap::ATileMap()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	MapRandomStream = FRandomStream();
}

// Called when the game starts or when spawned
void ATileMap::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		InitialSeed = FMath::RandRange(1000, 100000);

		MapRandomStream.Initialize(InitialSeed);
	}
}

void ATileMap::OnRep_InitialSeed()
{
	MapRandomStream.Initialize(InitialSeed);
	UE_LOG(LogTemp, Warning, TEXT("OnRep_InitialSeed called. InitialSeed: %d"), InitialSeed);
}

void ATileMap::InitSize(int Width, int Height)
{
	Tiles.AddDefaulted(Height);
	for (auto& row : Tiles)
	{
		row.AddDefaulted(Width);
	}
}

void ATileMap::AddTile(TSubclassOf<ATile> TileClass, TTuple<int, int> Idx, float Size)
{
	float Width = 2.f * Size;
	float Height = sqrt(3.f) * Size;

	float LocX = Idx.Value * (Width * 0.75f);
	float LocY = Idx.Key * Height;

	if (Idx.Value % 2 != 0)
	{
		LocY += Height * 0.5f;
	}

	SingleTile = GetWorld()->SpawnActor<ATile>(
		TileClass,
		FVector(LocX, LocY, 0.f),
		FRotator(90.f, 0.f, 0.f)
	);

	FVector Loc = SingleTile->GetActorLocation();
	UE_LOG(LogTemp, Warning, TEXT("Tile [%d, %d] Location: X=%.2f, Y=%.2f, Z=%.2f"),
		Idx.Key, Idx.Value, Loc.X, Loc.Y, Loc.Z);

	SingleTile->Axial = FAxial(Idx.Key, Idx.Value);

	SingleTile->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	SingleTile->SetIdxText(Idx.Key, Idx.Value);

	//TEST SPAWN//////////////////////////
	if (Idx.Key == 1 && Idx.Value == 2)
	{
		SingleTile->SetTileState(3);
	}

	else if (Idx.Key == 3 && Idx.Value == 7)
	{
		SingleTile->SetTileState(3);
	}
	//////////////////////////////////////
	else
	{
		SingleTile->SetTileState(0);
	}
	Tiles[Idx.Key][Idx.Value] = SingleTile;
}

TArray<ATile*> ATileMap::GetTilesAtDistance(TWeakObjectPtr<ATile> InCenterTile, int32 Distance)
{
    TArray<ATile*> FoundTiles;
    if (Distance <= 0) return FoundTiles;

    if (InCenterTile == nullptr)
        return FoundTiles;


    FAxial CenterAxial = InCenterTile->Axial;

    if (Distance == 1)
    {
        // 거리가 1일 때 (인접 타일 6개)
        if (CenterAxial.R % 2 == 0)
        {
            for (const FAxial& Dir : OddHexDirections)
            {
                FAxial NeighborAxial = FAxial(CenterAxial.Q + Dir.Q, CenterAxial.R + Dir.R);
                if (ATile* NeighborTile = GetTileAt(NeighborAxial))
                {
                    FoundTiles.Add(NeighborTile);
                }
            }
        }
        else
        {
            for (const FAxial& Dir : EvenHexDirections)
            {
                FAxial NeighborAxial = FAxial(CenterAxial.Q + Dir.Q, CenterAxial.R + Dir.R);
                if (ATile* NeighborTile = GetTileAt(NeighborAxial))
                {
                    FoundTiles.Add(NeighborTile);
                }
            }
        }


    }
    else
    {
        // 거리가 2 이상일 때 (헥스 링 알고리즘)
        // 시작점 위치 잡기 (Distance만큼 0번 방향으로 이동)
        if (CenterAxial.R % 2 == 0)
        {
            FAxial CurrentHex = FAxial(CenterAxial.Q + (OddHexDirections[4].Q * Distance),
                CenterAxial.R + (OddHexDirections[4].R * Distance));

            for (int32 i = 0; i < 6; ++i)
            {
                for (int32 j = 0; j < Distance; ++j)
                {
                    if (ATile* FoundTile = GetTileAt(CurrentHex))
                    {
                        FoundTiles.Add(FoundTile);
                    }
                    // 다음 칸으로 이동
                    CurrentHex.Q += OddHexDirections[i].Q;
                    CurrentHex.R += OddHexDirections[i].R;
                }
            }
        }
        else
        {
            FAxial CurrentHex = FAxial(CenterAxial.Q + (EvenHexDirections[4].Q * Distance),
                CenterAxial.R + (EvenHexDirections[4].R * Distance));

            for (int32 i = 0; i < 6; ++i)
            {
                for (int32 j = 0; j < Distance; ++j)
                {
                    if (ATile* FoundTile = GetTileAt(CurrentHex))
                    {
                        FoundTiles.Add(FoundTile);
                    }
                    // 다음 칸으로 이동
                    CurrentHex.Q += EvenHexDirections[i].Q;
                    CurrentHex.R += EvenHexDirections[i].R;
                }
            }
        }
    }

    return FoundTiles;
}

ATile* ATileMap::GetTileAt(FAxial AxialCoord)
{
    if (Tiles.IsValidIndex(AxialCoord.Q) && Tiles[AxialCoord.Q].IsValidIndex(AxialCoord.R))
        return Tiles[AxialCoord.Q][AxialCoord.R];;

    return nullptr;
}

void ATileMap::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATileMap, InitialSeed);

}

