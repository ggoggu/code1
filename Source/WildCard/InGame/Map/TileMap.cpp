// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Map/TileMap.h"
#include "InGame/Map/Tile.h"
#include "Net/UnrealNetwork.h"
#include "BattleData.h"
#include "InGameState.h"
#include "GameFramework/PlayerState.h"
#include "InGamePlayerState.h"
#include "UnitBase.h"
#include "InGame/Unit/PlayerUnit.h"

// Sets default values
ATileMap::ATileMap()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
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

void ATileMap::OnRep_FlatTiles()
{
	RebuildTiles2D();
}

void ATileMap::RebuildTiles2D()
{
	if (MapWidth <= 0 || MapHeight <= 0) return;

	Tiles.Empty();
	Tiles.AddDefaulted(MapHeight);
	for (int32 r = 0; r < MapHeight; ++r)
	{
		Tiles[r].SetNumZeroed(MapWidth);
		for (int32 c = 0; c < MapWidth; ++c)
		{
			int32 Index = r * MapWidth + c;
			if (FlatTiles.IsValidIndex(Index))
			{
				Tiles[r][c] = FlatTiles[Index];
			}
		}
	}
}

void ATileMap::InitSize(int Width, int Height)
{
	MapWidth = Width;
	MapHeight = Height;
	FlatTiles.SetNumZeroed(Width * Height);

	Tiles.Empty();
	Tiles.AddDefaulted(Height);
	for (auto& row : Tiles)
	{
		row.AddDefaulted(Width);
	}
}

void ATileMap::AddTile(TSubclassOf<ATile> TileClass, TTuple<int, int> Idx, float Size)
{
    AInGamePlayerState* FirstPlayerState = nullptr;
    AInGamePlayerState* SecondPlayerState = nullptr;

    for (APlayerState* PlayerState : GetWorld()->GetGameState<AInGameState>()->PlayerArray)
    {
        if (!PlayerState)
        {
            continue;
        }

        if (FirstPlayerState == nullptr)
        {
            FirstPlayerState = Cast<AInGamePlayerState>(PlayerState);
        }

        else if (FirstPlayerState != nullptr)
        {
            SecondPlayerState = Cast<AInGamePlayerState>(PlayerState);
            break;
        }
    }

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
	Tiles[Idx.Key][Idx.Value] = SingleTile;

	int32 FlatIdx = Idx.Key * MapWidth + Idx.Value;
	if (FlatTiles.IsValidIndex(FlatIdx))
	{
		FlatTiles[FlatIdx] = SingleTile;
	}
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

TArray<ATile*> ATileMap::GetTilesWithinDistance(TWeakObjectPtr<ATile> InCenterTile, int32 MaxDistance)
{
    TArray<ATile*> AllTiles;
    if (MaxDistance <= 0 || !InCenterTile.IsValid())
    {
        return AllTiles;
    }

    for (int32 d = 1; d <= MaxDistance; ++d)
    {
        TArray<ATile*> RingTiles = GetTilesAtDistance(InCenterTile, d);
        for (ATile* Tile : RingTiles)
        {
            if (Tile)
            {
                AllTiles.AddUnique(Tile);
            }
        }
    }

    return AllTiles;
}

ATile* ATileMap::GetTileAt(FAxial AxialCoord)
{
    if (Tiles.IsValidIndex(AxialCoord.Q) && Tiles[AxialCoord.Q].IsValidIndex(AxialCoord.R) && Tiles[AxialCoord.Q][AxialCoord.R])
    {
        return Tiles[AxialCoord.Q][AxialCoord.R];
    }

    if (MapWidth > 0 && MapHeight > 0)
    {
        if (AxialCoord.Q >= 0 && AxialCoord.Q < MapHeight && AxialCoord.R >= 0 && AxialCoord.R < MapWidth)
        {
            int32 FlatIdx = AxialCoord.Q * MapWidth + AxialCoord.R;
            if (FlatTiles.IsValidIndex(FlatIdx) && FlatTiles[FlatIdx])
            {
                return FlatTiles[FlatIdx];
            }
        }
    }

    return nullptr;
}

void ATileMap::TESTUnitSpawn()
{
    AInGamePlayerState* FirstPlayerState = nullptr;
    AInGamePlayerState* SecondPlayerState = nullptr;

    AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
    if (!GameState)
    {
        return;
    }

    for (APlayerState* PlayerState : GameState->PlayerArray)
    {
        if (!PlayerState)
        {
            continue;
        }

        if (FirstPlayerState == nullptr)
        {
            FirstPlayerState = Cast<AInGamePlayerState>(PlayerState);
        }
        else if (SecondPlayerState == nullptr)
        {
            SecondPlayerState = Cast<AInGamePlayerState>(PlayerState);
            break;
        }
    }

    if ((PlayerStartCells.Num() != 2) || (PlayerStartCells[0] == PlayerStartCells[1]))
    {
        UE_LOG(LogTemp, Error, TEXT("Two distinct player start cells are required."));
        return;
    }

    ATile* Player1Tile = GetTileAt(FAxial(PlayerStartCells[0].X, PlayerStartCells[0].Y));

    ATile* Player2Tile = GetTileAt(FAxial(PlayerStartCells[1].X, PlayerStartCells[1].Y));

    if (!IsValid(Player1Tile) || !IsValid(Player2Tile))
    {
        UE_LOG(LogTemp, Error, TEXT("Player start cells must reference generated tiles."));
        return;
    }

    //TODO : if문에 들어가는 조건이 변경되었는데 해당 사항이 문제없는지 확인 필요
    if (FirstPlayerState && !FirstPlayerState->GetPlayerUnit() && !Player1Tile->HasUnit())
    {
        Player1Tile->SpawnPlayerUnit(FirstPlayerState->GetPlayerUnitClass(), FirstPlayerState, FirstPlayerState->GetPlayerClass());
    }

    if (SecondPlayerState && !SecondPlayerState->GetPlayerUnit() && !Player2Tile->HasUnit())
    {
        Player2Tile->SpawnPlayerUnit(SecondPlayerState->GetPlayerUnitClass(), SecondPlayerState, SecondPlayerState->GetPlayerClass());
    }
}

void ATileMap::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATileMap, InitialSeed);
    DOREPLIFETIME_CONDITION(ATileMap, MapWidth, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(ATileMap, MapHeight, COND_InitialOnly);
    DOREPLIFETIME_CONDITION(ATileMap, FlatTiles, COND_InitialOnly);
}

