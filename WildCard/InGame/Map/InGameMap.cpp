// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Map/InGameMap.h"
#include "InGameMap.h"
#include "Tile.h"
#include "InGameHexagon.h"
#include "Net/UnrealNetwork.h"


// Sets default values
AInGameMap::AInGameMap()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

	MapRandomStream = FRandomStream();

}

// Called when the game starts or when spawned
void AInGameMap::BeginPlay()
{
	Super::BeginPlay();

    if (HasAuthority())
    { 
        InitialSeed = FMath::RandRange(1000, 100000);
         
        MapRandomStream.Initialize(InitialSeed);
        SetAllTileRandom();
    }
	
}

void AInGameMap::SetAllTileIdle()
{
   for (auto& tile : Tiles)
   {
       for (auto& t : tile)
       {

           //t->SetTileState(ETileState::Idle);
       } 
   }
}

void AInGameMap::SetAllTileMoveble()
{
    for (auto& tile : Tiles)
    {
        for (auto& t : tile)
        {

            //t->SetTileState(ETileState::Moveable);
        }
    }
}

void AInGameMap::SetAllTileRandom()
{    
    if (!HasAuthority())
    {
        return;
    }

    for (auto& tile : Tiles)
    {
        for (auto& t : tile)
        {
            int RandomNumber = MapRandomStream.RandRange(0, 3);
            //t->SetTileState(static_cast<ETileState>(RandomNumber));
            t->OnRep_TileState();
        }
    } 
}

void AInGameMap::ShowMoveableFormCurrent(ATile* SelectTile)
{
   TArray<ATile*> MoveableTiles;
   FCube Cube = SelectTile->Axial.ToCube();
   for (int i = 0; i < 6; i++)
   {
      FCube NCube = FHexagon::CubeNeighbor(Cube, i);
      TTuple<int, int> Idx = NCube.ToAxial().ToIndex();
      if (Idx.Key < 0 || Idx.Value < 0 || Idx.Key >= Tiles.Num() || Idx.Value >= Tiles[Idx.Key].Num()) continue;
      MoveableTiles.Add(Tiles[Idx.Key][Idx.Value]);
   }
   SetTileMoveable(MoveableTiles);
}

void AInGameMap::ShowSpawnableFormCurrent(ATile* SelectTile)
{
   TArray<ATile*> SpawnableTiles;
   FCube Cube = SelectTile->Axial.ToCube();
   for (int i = 0; i < 6; i++)
   {
      FCube NCube = FHexagon::CubeNeighbor(Cube, i);
      TTuple<int, int> Idx = NCube.ToAxial().ToIndex();
      if (Idx.Key < 0 || Idx.Value < 0 || Idx.Key >= Tiles.Num() || Idx.Value >= Tiles[Idx.Key].Num()) continue;
      SpawnableTiles.Add(Tiles[Idx.Key][Idx.Value]);
   }
   SetTileMoveable(SpawnableTiles);
}

void AInGameMap::SetTileSpawnable(TArray<ATile*>& SelectTiles)
{
   for (auto& Tile : SelectTiles)
   {
      //Tile->SetTileState(ETileState::Spawnable);
   }
}

void AInGameMap::SetTileMoveable(TArray<ATile*>& SelectTiles)
{
   for (auto& Tile : SelectTiles)
   {
      //Tile->SetTileState(ETileState::Moveable);
   }
}

void AInGameMap::OnRep_InitialSeed()
{
    MapRandomStream.Initialize(InitialSeed);
}

void AInGameMap::InitSize(int Width, int Height)
{
   Tiles.AddDefaulted(Height);
   for (auto& row : Tiles)
   {
      row.AddDefaulted(Width);
   }
}

void AInGameMap::AddTile(TSubclassOf<ATile> TileClass, TTuple<int, int> Idx, float Size)
{
   //float Horiz = 3.f / 2.f * Size;
   //float Verti = sqrt(3) * Size;   

   ////#. ��/��� �Դٰ���: Row Ȧ¦�� ���� X�� ��ĭ �̵�
   //float LocY = (float)Idx.Key * Verti;
   //float XOffset = (Idx.Key % 2 != 0) ? (Horiz / 2.f) : 0.f;
   //float LocX = (float)Idx.Value * Horiz + XOffset;

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
   //UE_LOG(Warning, TEXT("TILE STATE : [%d / %d] - %d"), Idx.Key, Idx.Value, SingleTile->GetTileState());
   Tiles[Idx.Key][Idx.Value] = SingleTile;
}

TArray<ATile*> AInGameMap::GetTilesAtDistance(TWeakObjectPtr<ATile> InCenterTile, int32 Distance)
{
    TArray<ATile*> FoundTiles;
    if (Distance <= 0) return FoundTiles;

    if (InCenterTile == nullptr) {
        //WILD_UELOG(Warning, TEXT("Error"));
        return FoundTiles;
    }
        

    FAxial CenterAxial = InCenterTile->Axial;

    if (Distance == 1)
    {
        // �Ÿ��� 1�� �� (���� Ÿ�� 6��)
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
        //WILD_UELOG(Warning, TEXT("Wrong Distance."));
        // �Ÿ��� 2 �̻��� �� (���� �� �˰�����)
        // ������ ��ġ ��� (Distance��ŭ 0�� �������� �̵�)
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
                    // ���� ĭ���� �̵�
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
                    // ���� ĭ���� �̵�
                    CurrentHex.Q += EvenHexDirections[i].Q;
                    CurrentHex.R += EvenHexDirections[i].R;
                }
            }
        }
    }

    return FoundTiles;
}

ATile* AInGameMap::GetTileAt(FAxial AxialCoord)
{
    if (Tiles.IsValidIndex(AxialCoord.Q) && Tiles[AxialCoord.Q].IsValidIndex(AxialCoord.R))
        return Tiles[AxialCoord.Q][AxialCoord.R];;

    return nullptr;
}

void AInGameMap::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AInGameMap, InitialSeed);
}
