// Fill out your copyright notice in the Description page of Project Settings.

#include "InGame/Map/WildActionPreviewArrow.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AWildActionPreviewArrow::AWildActionPreviewArrow()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickInterval = 0.0333f;
	SetActorEnableCollision(false);

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshObj(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMeshObj(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MoveMatObj(TEXT("/Game/InGame/Unit/BP/M_Moveable.M_Moveable"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMatObj(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	if (MoveMatObj.Succeeded())
	{
		BaseMaterial = MoveMatObj.Object;
	}
	else if (BasicMatObj.Succeeded())
	{
		BaseMaterial = BasicMatObj.Object;
	}

	UStaticMesh* CylinderMesh = CylinderMeshObj.Succeeded() ? CylinderMeshObj.Object : nullptr;
	UStaticMesh* ConeMesh = ConeMeshObj.Succeeded() ? ConeMeshObj.Object : nullptr;

	// 모바일 최적화: 단일 인스턴스드 스태틱 메쉬 컴포넌트 (ISMC)로 모든 원통 세그먼트를 1 드로우 콜로 렌더링
	InstancedSegmentsComp = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("InstancedSegmentsComp"));
	InstancedSegmentsComp->SetupAttachment(RootComponent);
	InstancedSegmentsComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	InstancedSegmentsComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InstancedSegmentsComp->SetCastShadow(false);
	if (CylinderMesh)
	{
		InstancedSegmentsComp->SetStaticMesh(CylinderMesh);
	}

	// 화살촉 원뿔 메쉬 (1 드로우 콜)
	ArrowHeadMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrowHeadMeshComp"));
	ArrowHeadMeshComp->SetupAttachment(RootComponent);
	ArrowHeadMeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	ArrowHeadMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArrowHeadMeshComp->SetCastShadow(false);
	if (ConeMesh)
	{
		ArrowHeadMeshComp->SetStaticMesh(ConeMesh);
	}
	ArrowHeadMeshComp->SetVisibility(false);
}

void AWildActionPreviewArrow::BeginPlay()
{
	Super::BeginPlay();
	EnsureMaterial();
}

void AWildActionPreviewArrow::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bUseMaterialShaderPulse)
	{
		SetActorTickEnabled(false);
		return;
	}

	if (DynMaterial)
	{
		PulseTimer += DeltaTime * 3.0f;
		const float PulseAlpha = (FMath::Sin(PulseTimer) + 1.0f) * 0.5f;
		const FLinearColor PulsedColor = BaseColor * (1.2f + PulseAlpha * 0.8f);
		DynMaterial->SetVectorParameterValue(FName("EmissiveColor"), PulsedColor);
	}
}

void AWildActionPreviewArrow::SetArrowColor(const FLinearColor& InColor)
{
	BaseColor = InColor;
	if (DynMaterial)
	{
		DynMaterial->SetVectorParameterValue(FName("Color"), BaseColor);
		DynMaterial->SetVectorParameterValue(FName("BaseColor"), BaseColor);
		DynMaterial->SetVectorParameterValue(FName("EmissiveColor"), BaseColor * 2.0f);
	}
}

void AWildActionPreviewArrow::ClearArrow()
{
	if (InstancedSegmentsComp)
	{
		InstancedSegmentsComp->ClearInstances();
	}
	if (ArrowHeadMeshComp)
	{
		ArrowHeadMeshComp->SetVisibility(false);
	}
	SetActorTickEnabled(false);
}

void AWildActionPreviewArrow::EnsureMaterial()
{
	if (!DynMaterial)
	{
		if (!BaseMaterial)
		{
			BaseMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT("/Game/InGame/Unit/BP/M_Moveable.M_Moveable")));
			if (!BaseMaterial)
			{
				BaseMaterial = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
			}
		}

		if (BaseMaterial)
		{
			DynMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		}
	}

	if (DynMaterial)
	{
		DynMaterial->SetVectorParameterValue(FName("Color"), BaseColor);
		DynMaterial->SetVectorParameterValue(FName("BaseColor"), BaseColor);
		DynMaterial->SetVectorParameterValue(FName("Tint"), BaseColor);
		DynMaterial->SetVectorParameterValue(FName("EmissiveColor"), BaseColor * 2.0f);
		DynMaterial->SetScalarParameterValue(FName("Roughness"), 0.2f);

		if (InstancedSegmentsComp)
		{
			InstancedSegmentsComp->SetMaterial(0, DynMaterial);
		}
		if (ArrowHeadMeshComp)
		{
			ArrowHeadMeshComp->SetMaterial(0, DynMaterial);
		}
	}
}

void AWildActionPreviewArrow::SetupArrow(const FVector& StartLocation, const FVector& EndLocation, const FLinearColor& InColor)
{
	BaseColor = InColor;

	FVector AdjustedStart = StartLocation;
	FVector AdjustedEnd = EndLocation;
	if (AdjustedStart.Z < 30.0f)
	{
		AdjustedStart.Z += DefaultHeightOffset;
	}
	if (AdjustedEnd.Z < 30.0f)
	{
		AdjustedEnd.Z += DefaultHeightOffset;
	}

	CachedStart = AdjustedStart;
	CachedEnd = AdjustedEnd;

	SetActorLocation(AdjustedStart);
	SetActorTickEnabled(!bUseMaterialShaderPulse);

	EnsureMaterial();
	UpdateCurveGeometry();
}

void AWildActionPreviewArrow::UpdateCurveGeometry()
{
	const float Distance2D = FVector::Dist2D(CachedStart, CachedEnd);
	const float Distance3D = FVector::Dist(CachedStart, CachedEnd);

	if (Distance3D <= 1.0f)
	{
		ClearArrow();
		return;
	}

	if (!InstancedSegmentsComp || !ArrowHeadMeshComp)
	{
		return;
	}

	// 1. 포물선 호(Arc) 제어점 계산: 유닛 위에서 출발해 허공으로 둥글게 솟아오른 뒤 목표 유닛 위로 하강
	const float ArcHeight = FMath::Clamp(Distance2D * ArcHeightMultiplier, MinArcHeight, MaxArcHeight);
	const FVector P0 = CachedStart;
	const FVector P2 = CachedEnd;
	const FVector MidXY = (P0 + P2) * 0.5f;
	const float PeakZ = FMath::Max(P0.Z, P2.Z) + ArcHeight;
	const FVector P1 = FVector(MidXY.X, MidXY.Y, PeakZ);

	auto GetBezierPoint = [&](float t) -> FVector
	{
		const float OneMinusT = 1.0f - t;
		return OneMinusT * OneMinusT * P0 + 2.0f * OneMinusT * t * P1 + t * t * P2;
	};

	auto GetBezierTangent = [&](float t) -> FVector
	{
		const float OneMinusT = 1.0f - t;
		return 2.0f * OneMinusT * (P1 - P0) + 2.0f * t * (P2 - P1);
	};

	// 2. 화살촉 비율 계산 (곡선 끝부분에서 ArrowHeadLength 만큼 할당)
	const float HeadFraction = FMath::Clamp(ArrowHeadLength / FMath::Max(10.0f, Distance3D), 0.05f, 0.25f);
	const float BodyEndT = 1.0f - HeadFraction;

	// 3. 곡선 몸통(Cylinder)을 ISMC 인스턴스로 일괄 생성
	if (!InstancedSegmentsComp->GetStaticMesh())
	{
		UStaticMesh* FallbackCylinder = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
		if (FallbackCylinder)
		{
			InstancedSegmentsComp->SetStaticMesh(FallbackCylinder);
		}
	}

	InstancedSegmentsComp->ClearInstances();

	const int32 NumSegs = FMath::Clamp(ActiveSegmentCount, 4, 32);
	TArray<FTransform> InstanceTransforms;
	InstanceTransforms.Reserve(NumSegs);

	for (int32 i = 0; i < NumSegs; ++i)
	{
		const float tA = ((float)i / (float)NumSegs) * BodyEndT;
		const float tB = ((float)(i + 1) / (float)NumSegs) * BodyEndT;

		const FVector PosA = GetBezierPoint(tA);
		const FVector PosB = GetBezierPoint(tB);

		const FVector SegVec = PosB - PosA;
		const float SegLen = SegVec.Size();

		if (SegLen <= 0.1f)
		{
			continue;
		}

		const FVector SegCenter = (PosA + PosB) * 0.5f;
		const FRotator SegRot = FRotationMatrix::MakeFromZ(SegVec.GetSafeNormal()).Rotator();

		// 출발지(0.85)에서 화살촉 방향(1.0)으로 가며 미세하게 굵어지는 유선형 테이퍼링
		const float TaperFactor = FMath::Lerp(0.85f, 1.0f, (float)i / (float)NumSegs);
		const float CurrentRadius = TubeRadius * TaperFactor;

		// UE 실린더 기본: 반지름 50, 높이 100
		const FVector SegScale(CurrentRadius / 50.0f, CurrentRadius / 50.0f, SegLen / 100.0f);

		InstanceTransforms.Add(FTransform(SegRot, SegCenter, SegScale));
	}

	if (InstanceTransforms.Num() > 0)
	{
		InstancedSegmentsComp->AddInstances(InstanceTransforms, false, true, false);
	}

	// 4. 화살촉(Cone) 배치: 곡선의 끝점(P2)을 향해 꽂히도록 정렬
	if (!ArrowHeadMeshComp->GetStaticMesh())
	{
		UStaticMesh* FallbackCone = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/Engine/BasicShapes/Cone.Cone")));
		if (FallbackCone)
		{
			ArrowHeadMeshComp->SetStaticMesh(FallbackCone);
		}
	}

	const FVector EndTangent = GetBezierTangent(1.0f).GetSafeNormal();
	const FRotator HeadRot = FRotationMatrix::MakeFromZ(EndTangent).Rotator();

	// UE 콘 기본: 반지름 50, 높이 100, 뾰족한 끝(Tip)은 +Z 방향
	const FVector HeadScale(ArrowHeadRadius / 50.0f, ArrowHeadRadius / 50.0f, ArrowHeadLength / 100.0f);

	FVector LocalTip = FVector(0.0f, 0.0f, 50.0f);
	if (ArrowHeadMeshComp->GetStaticMesh())
	{
		const FBoxSphereBounds Bounds = ArrowHeadMeshComp->GetStaticMesh()->GetBounds();
		if (Bounds.BoxExtent.Z > 1.0f)
		{
			LocalTip = Bounds.Origin + FVector(0.0f, 0.0f, Bounds.BoxExtent.Z);
		}
	}

	const FVector ScaledLocalTip = LocalTip * HeadScale;
	const FVector HeadLoc = P2 - HeadRot.RotateVector(ScaledLocalTip);

	ArrowHeadMeshComp->SetWorldLocationAndRotation(HeadLoc, HeadRot);
	ArrowHeadMeshComp->SetWorldScale3D(HeadScale);
	ArrowHeadMeshComp->SetVisibility(true);
}
