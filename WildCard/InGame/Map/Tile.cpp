// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Map/Tile.h"
#include "UnitBase.h"
#include "CardBase.h"
#include "InGamePawn.h"
#include "BattleData.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ATile::ATile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);
	SetActorEnableCollision(true);

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	
	//StaticMesh->SetupAttachment(RootComponent);
	RootComponent = StaticMesh;

}

void ATile::SpawnUnit(FCardData InCardData, TSubclassOf<AUnitBase> UnitClass, int32 InPlayerCount)
{
    FVector SpawnLocation = GetActorLocation();
	//SpawnLocation.Z += 100.0f; // Adjust Z-axis to place unit above the tile
    //SpawnLocation.Y += 2000.0f;

 //   PlacedUnit = GetWorld()->SpawnActor<AWildCardActor>(
 //       UnitClass,
 //       SpawnLocation,
 //       //TODO : �ӽ� Rotation, ȭ���� �ڸ� �ٶ󺸰� �ϴ� ������ ��� �̷��� �س���.
 //       FRotator(0, 180, 0)
 //   );

 //   PlacedUnit->InitUnitCardData(InCardData, InPlayerCount);

	//AWildProtoGameState* GS = GetWorld()->GetGameState<AWildProtoGameState>();
	//WILD_CHECK(GS);

 //   GS->HandleUnitSpawned(PlacedUnit, this);


}

void ATile::MoveToUnit(AUnitBase *Unit, FVector PositionToMove)
{
    // 2. �̵���ų ���� ��ǥ ��ǥ (����)
    FVector TargetLocation(1000.0f, 500.0f, 200.0f);

    // 3. �浹 ó�� ���� (Hit sweeping)
    bool bSweep = false; // �̵� ��ο� �浹�� �ִ��� Ȯ���ϰ� �ʹٸ� true
    FHitResult SweepResult; // bSweep�� true�� �� �浹 ������ ����˴ϴ�.

    if (Unit)
    {
        // SetActorLocation�� ����Ͽ� ���͸� ��� �̵���Ű�ų�, Sweep ����� ����Ͽ� �浹�� Ȯ���ϸ� �̵���ŵ�ϴ�.
        Unit->SetActorLocation(
            PositionToMove,
            bSweep,       
            &SweepResult,  
            ETeleportType::None  
        ); 
    }
}

void ATile::OnRep_TileState()
{
    //SetTileState(TileState);
}

void ATile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATile, TileState);
    DOREPLIFETIME(ATile, PlacedUnit);
    DOREPLIFETIME(ATile, Axial);

}

void ATile::SetUnitMoveToTileInBP(FVector InHasUnitTileLocation, FVector InEmptyTileLocation)
{
	Cast<AInGamePawn>(GetWorld()->GetFirstPlayerController()->GetPawn())->Server_MoveUnitActor(InHasUnitTileLocation, InEmptyTileLocation);
}

void ATile::SetUnitAttackTargetInBP(const FString& InAttacker, const FString& InDefender)
{
    Cast<AInGamePawn>(GetWorld()->GetFirstPlayerController()->GetPawn())->Server_FightUnitActor(InAttacker, InDefender);
}

bool ATile::HasOwnerIDtoActionUnit(int32 InOwnerId)
{
    if (InOwnerId != PlacedUnit->GetOwnerId())
    {
        return false;
    }
    else
    {
		return true;
    }

}

void ATile::OnRep_MoveUnit()
{
    if (IsValid(PlacedUnit))
    {
        //#. PlacedUnit�� �����
		PlacedUnit->SetActorLocation(GetActorLocation());
    }
    else
    {
		//#. PlacedUnit�� ��������
    }
}

void ATile::OnRep_Axial()
{
    //WILD_SIMPLE(Warning);
}

