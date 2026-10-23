// Fill out your copyright notice in the Description page of Project Settings.

#include "InGame/Map/Tile.h"
#include "WildCard.h"
#include "Net/UnrealNetwork.h"
#include "InGame/Unit/UnitBase.h"
#include "InGame/Unit/PlayerUnit.h"
#include "InGame/Game/InGamePlayerState.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

// Sets default values
ATile::ATile()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	SetReplicateMovement(false);
	SetActorEnableCollision(true);

	SetNetUpdateFrequency(2.0f);
	SetMinNetUpdateFrequency(0.5f);
	SetNetDormancy(DORM_Initial);
}

UStaticMeshComponent* ATile::GetBaseStaticMeshComponent()
{
	if (StaticMesh)
	{
		return StaticMesh;
	}

	TArray<UStaticMeshComponent*> MeshComps;
	GetComponents<UStaticMeshComponent>(MeshComps);
	for (UStaticMeshComponent* Comp : MeshComps)
	{
		if (Comp)
		{
			StaticMesh = Comp;
			return StaticMesh;
		}
	}

	return nullptr;
}

void ATile::SetPlacedUnit(AUnitBase* InPlacedUnit)
{
	PlacedUnit = InPlacedUnit;
	if (PlacedUnit)
	{
		PlacedUnit->DisableMouseClickCollision();
	}
	if (HasAuthority())
	{
		FlushNetDormancy();
	}
}

// Called when the game starts or when spawned
void ATile::BeginPlay()
{
	Super::BeginPlay();

	GetBaseStaticMeshComponent();

	// 마우스 클릭 트레이스 채널에 대해 Block 설정하여 유닛이 위에 있어도 타일이 감지되도록 보장
	TArray<UPrimitiveComponent*> PrimComps;
	GetComponents<UPrimitiveComponent>(PrimComps);
	for (UPrimitiveComponent* PrimComp : PrimComps)
	{
		if (PrimComp)
		{
			if (PrimComp->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
			{
				PrimComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			}
			PrimComp->SetCollisionResponseToChannel(ECC_MouseClick, ECR_Block);
		}
	}
}

// Called every frame
void ATile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATile::SetTileHovered_Implementation(bool bHovered)
{
	if (bIsTileHovered == bHovered)
	{
		return;
	}

	bIsTileHovered = bHovered;

	UStaticMeshComponent* TargetMesh = GetBaseStaticMeshComponent();
	if (!TargetMesh)
	{
		return;
	}

	if (bHovered)
	{
		UMaterialInterface* CurrentMat = TargetMesh->GetMaterial(0);
		if (CurrentMat && CurrentMat != HoverDynamicMaterial && CurrentMat != MHovered)
		{
			CachedMaterialBeforeHover = CurrentMat;
		}

		if (MHovered)
		{
			// 에디터에서 지정한 호버 머티리얼이 있으면 최우선 적용
			TargetMesh->SetMaterial(0, MHovered);
		}
		else
		{
			// 동적 머티리얼 인스턴스를 생성하여 HoverColor(선명한 오렌지 골드 등) 적용
			if (!HoverDynamicMaterial)
			{
				UMaterialInterface* BaseMat = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial")));
				if (!BaseMat)
				{
					BaseMat = CachedMaterialBeforeHover ? CachedMaterialBeforeHover.Get() : MIdle.Get();
				}

				if (BaseMat)
				{
					HoverDynamicMaterial = UMaterialInstanceDynamic::Create(BaseMat, this);
				}
			}

			if (HoverDynamicMaterial)
			{
				HoverDynamicMaterial->SetVectorParameterValue(FName("Color"), HoverColor);
				HoverDynamicMaterial->SetVectorParameterValue(FName("BaseColor"), HoverColor);
				HoverDynamicMaterial->SetVectorParameterValue(FName("Tint"), HoverColor);
				HoverDynamicMaterial->SetVectorParameterValue(FName("EmissiveColor"), HoverColor * 1.5f);
				TargetMesh->SetMaterial(0, HoverDynamicMaterial);
			}
			else if (MMoveable)
			{
				// Fallback: MMoveable (파란색 타일 머티리얼) 적용
				TargetMesh->SetMaterial(0, MMoveable);
			}
		}
	}
	else
	{
		if (CachedMaterialBeforeHover)
		{
			TargetMesh->SetMaterial(0, CachedMaterialBeforeHover);
			CachedMaterialBeforeHover = nullptr;
		}
		else
		{
			switch (TileState)
			{
			case ETileState::Spawnable:
				if (MSpawnable) TargetMesh->SetMaterial(0, MSpawnable);
				break;
			case ETileState::Moveable:
				if (MMoveable) TargetMesh->SetMaterial(0, MMoveable);
				break;
			case ETileState::Idle:
				if (MIdle) TargetMesh->SetMaterial(0, MIdle);
				break;
			case ETileState::Combatable:
			case ETileState::Effectable:
			case ETileState::None:
			default:
				if (MNone) TargetMesh->SetMaterial(0, MNone);
				break;
			}
		}

		SetTileState(static_cast<int32>(TileState));
	}
}

void ATile::SpawnUnit(FCardData InCardData, TSubclassOf<AUnitBase> UnitClass, int32 InPlayerCount, int32 CardInstanceID)
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
	UE_LOG(LogTemp, Warning, TEXT("Before / Spawned Unit: %s at Tile: %s"), *NewUnit->GetName(), *GetName());
	if (NewUnit)
	{

		NewUnit->SetCurrentTile(this);
		NewUnit->InitUnitCardData(InCardData, InPlayerCount, CardInstanceID);
		SetPlacedUnit(NewUnit);
		UE_LOG(LogTemp, Warning, TEXT("After / Spawned Unit: %s at Tile: %s"), *PlacedUnit->GetName(), *GetName());

	}
}

APlayerUnit* ATile::SpawnPlayerUnit(TSubclassOf<APlayerUnit> PlayerUnitClass, AInGamePlayerState* InPlayerState, EWildPlayerClass InClass)
{
	if (!HasAuthority() || !InPlayerState) return nullptr;

	TSubclassOf<APlayerUnit> ClassToSpawn = PlayerUnitClass ? PlayerUnitClass : InPlayerState->GetPlayerUnitClass();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	APlayerUnit* NewPlayerUnit = GetWorld()->SpawnActor<APlayerUnit>(
		ClassToSpawn,
		GetActorLocation(),
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (NewPlayerUnit)
	{
		NewPlayerUnit->SetCurrentTile(this);
		NewPlayerUnit->InitializePlayerUnit(InPlayerState, InClass, nullptr, InPlayerState->GetSelectedTacticalSkillClass());
		SetPlacedUnit(NewPlayerUnit);
		UE_LOG(LogTemp, Warning, TEXT("[Tile] Spawned PlayerUnit: %s at Tile: %s for Player: %d"), *NewPlayerUnit->GetName(), *GetName(), InPlayerState->GetPlayerId());
	}

	return NewPlayerUnit;
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

	DOREPLIFETIME_CONDITION(ATile, Axial, COND_InitialOnly);
	DOREPLIFETIME(ATile, PlacedUnit);
}
