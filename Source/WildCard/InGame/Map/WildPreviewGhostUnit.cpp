// Fill out your copyright notice in the Description page of Project Settings.

#include "InGame/Map/WildPreviewGhostUnit.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "UObject/ConstructorHelpers.h"

AWildPreviewGhostUnit::AWildPreviewGhostUnit()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0333f;
	SetActorEnableCollision(false);

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;
	SetActorScale3D(FVector(3.0f, 3.0f, 3.0f));

	CardVisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CardVisualRoot"));
	CardVisualRoot->SetupAttachment(RootComponent);

	CardMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CardMeshComp"));
	CardMeshComp->SetupAttachment(CardVisualRoot);
	CardMeshComp->SetRelativeScale3D(CardMeshScale);
	CardMeshComp->SetRelativeLocation(CardMeshOffset);
	CardMeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CardMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CardMeshComp->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultCubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultCubeMesh.Succeeded())
	{
		CardMeshComp->SetStaticMesh(DefaultCubeMesh.Object);
	}
}

void AWildPreviewGhostUnit::BeginPlay()
{
	Super::BeginPlay();

	if (CardMeshComp && !DynMaterial)
	{
		UMaterialInterface* BaseMat = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT("/Game/Mesh/AdvancedPortalsSystemVFX/Materials/M_Portal_Image.M_Portal_Image")));
		if (!BaseMat)
		{
			BaseMat = CardMeshComp->GetMaterial(0);
		}
		if (!BaseMat)
		{
			BaseMat = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial")));
		}

		if (BaseMat)
		{
			DynMaterial = UMaterialInstanceDynamic::Create(BaseMat, this);
			CardMeshComp->SetMaterial(0, DynMaterial);
			DynMaterial->SetScalarParameterValue(FName("distortion speed "), 0.0f);
			DynMaterial->SetScalarParameterValue(FName("distortion speed"), 0.0f);
			DynMaterial->SetVectorParameterValue(FName("Texture Tint"), TintColor);
			DynMaterial->SetScalarParameterValue(FName("Opacity"), Opacity);
			DynMaterial->SetScalarParameterValue(FName("Texture Desaturation"), Desaturation);
		}
	}
}

void AWildPreviewGhostUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bEnableFacingCamera)
	{
		UpdateFacingCamera();
	}

	if (DynMaterial)
	{
		PulseTimer += DeltaTime * 2.0f;
		const float PulseAlpha = (FMath::Sin(PulseTimer) + 1.0f) * 0.5f;
		const float CurrentOpacity = FMath::Clamp(Opacity * (0.85f + PulseAlpha * 0.25f), 0.0f, 1.0f);
		DynMaterial->SetScalarParameterValue(FName("Opacity"), CurrentOpacity);
	}
}

void AWildPreviewGhostUnit::SetGhostOpacity(float InOpacity)
{
	Opacity = FMath::Clamp(InOpacity, 0.0f, 1.0f);
	if (DynMaterial)
	{
		DynMaterial->SetScalarParameterValue(FName("Opacity"), Opacity);
	}
}

void AWildPreviewGhostUnit::SetGhostDesaturation(float InDesaturation)
{
	Desaturation = FMath::Clamp(InDesaturation, 0.0f, 1.0f);
	if (DynMaterial)
	{
		DynMaterial->SetScalarParameterValue(FName("Texture Desaturation"), Desaturation);
	}
}

void AWildPreviewGhostUnit::UpdateFacingCamera()
{
	if (IsRunningDedicatedServer() || !CardVisualRoot || !bEnableFacingCamera || BillboardMode == EUnitBillboardMode::Disabled)
	{
		return;
	}

	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!CameraManager)
	{
		return;
	}

	if (BillboardMode == EUnitBillboardMode::CameraAligned)
	{
		FRotator TargetRotation = (-CameraManager->GetCameraRotation().Vector()).Rotation();
		TargetRotation += BillboardRotationOffset;
		CardVisualRoot->SetWorldRotation(TargetRotation);
		return;
	}

	const FVector CameraLocation = CameraManager->GetCameraLocation();
	const FVector VisualLocation = CardVisualRoot->GetComponentLocation();
	FVector DirectionToCamera = CameraLocation - VisualLocation;

	if (BillboardMode == EUnitBillboardMode::CylindricalYaw)
	{
		DirectionToCamera.Z = 0.0f;
	}

	if (!DirectionToCamera.IsNearlyZero())
	{
		FRotator TargetRotation = DirectionToCamera.Rotation();
		TargetRotation += BillboardRotationOffset;
		CardVisualRoot->SetWorldRotation(TargetRotation);
	}
}

void AWildPreviewGhostUnit::SetupGhostUnit(UTexture2D* CardTexture, UTexture2D* FrameTexture, const FLinearColor& InTintColor, float InOpacity, float InDesaturation)
{
	TintColor = InTintColor;
	Opacity = InOpacity;
	Desaturation = InDesaturation;

	if (!DynMaterial && CardMeshComp)
	{
		UMaterialInterface* BaseMat = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT("/Game/Mesh/AdvancedPortalsSystemVFX/Materials/M_Portal_Image.M_Portal_Image")));
		if (!BaseMat)
		{
			BaseMat = CardMeshComp->GetMaterial(0);
		}
		if (BaseMat)
		{
			DynMaterial = UMaterialInstanceDynamic::Create(BaseMat, this);
			CardMeshComp->SetMaterial(0, DynMaterial);
		}
	}

	if (DynMaterial)
	{
		// 포탈/불꽃 왜곡 애니메이션을 꺼서 카드가 깨끗한 고스트 형태로 보이도록 설정
		DynMaterial->SetScalarParameterValue(FName("distortion speed "), 0.0f);
		DynMaterial->SetScalarParameterValue(FName("distortion speed"), 0.0f);

		if (CardTexture)
		{
			DynMaterial->SetTextureParameterValue(FName("Portal Texture"), CardTexture);
			DynMaterial->SetTextureParameterValue(FName("CardTexture"), CardTexture);
			DynMaterial->SetTextureParameterValue(FName("CardImage"), CardTexture);
			DynMaterial->SetTextureParameterValue(FName("BaseColorTexture"), CardTexture);
			DynMaterial->SetTextureParameterValue(FName("Texture"), CardTexture);
			DynMaterial->SetTextureParameterValue(FName("texture"), CardTexture);
			DynMaterial->SetTextureParameterValue(FName("Albedo"), CardTexture);
		}

		if (FrameTexture)
		{
			DynMaterial->SetTextureParameterValue(FName("CardFrameTexture"), FrameTexture);
			DynMaterial->SetTextureParameterValue(FName("CardFrame"), FrameTexture);
			DynMaterial->SetTextureParameterValue(FName("FrameTexture"), FrameTexture);
		}

		// 회색 곱하기(Gray multiply) 및 반투명 적용
		DynMaterial->SetVectorParameterValue(FName("Texture Tint"), TintColor);
		DynMaterial->SetVectorParameterValue(FName("Color"), TintColor);
		DynMaterial->SetVectorParameterValue(FName("BaseColor"), TintColor);
		DynMaterial->SetVectorParameterValue(FName("Tint"), TintColor);
		DynMaterial->SetVectorParameterValue(FName("EmissiveColor"), TintColor * 0.1f);
		DynMaterial->SetScalarParameterValue(FName("Opacity"), Opacity);
		DynMaterial->SetScalarParameterValue(FName("Texture Desaturation"), Desaturation);
	}
}
