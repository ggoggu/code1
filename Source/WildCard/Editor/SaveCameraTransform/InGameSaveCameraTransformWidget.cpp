// Fill out your copyright notice in the Description page of Project Settings.

#include "Editor/SaveCameraTransform/InGameSaveCameraTransformWidget.h"
#include "Editor/SaveCameraTransform/CameraTransfromDataAsset.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Camera/PlayerCameraManager.h"

void UInGameSaveCameraTransformWidget::SaveInGameCamera()
{
	if (!CameraDataAsset) return;

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;

	if (PC->PlayerCameraManager)
	{
		Location = PC->PlayerCameraManager->GetCameraLocation();
		Rotation = PC->PlayerCameraManager->GetCameraRotation();
	}
	else if (APawn* Pawn = PC->GetPawn())
	{
		Location = Pawn->GetActorLocation();
		Rotation = Pawn->GetActorRotation();
	}
	else
	{
		Location = PC->GetSpawnLocation();
		Rotation = PC->GetControlRotation();
	}

	FTransform NewTransform(Rotation, Location, FVector::OneVector);

#if WITH_EDITOR
	CameraDataAsset->Modify();
#endif
	CurrentIndex = CameraDataAsset->SavedTransforms.Add(NewTransform);
#if WITH_EDITOR
	CameraDataAsset->MarkPackageDirty();
#endif
}

void UInGameSaveCameraTransformWidget::MoveToNextTransform()
{
	if (!CameraDataAsset || CameraDataAsset->SavedTransforms.IsEmpty()) return;

	CurrentIndex = (CurrentIndex + 1) % CameraDataAsset->SavedTransforms.Num();
	ApplyTransformToPlayer(CameraDataAsset->SavedTransforms[CurrentIndex]);
}

void UInGameSaveCameraTransformWidget::MoveToPreviousTransform()
{
	if (!CameraDataAsset || CameraDataAsset->SavedTransforms.IsEmpty()) return;

	CurrentIndex = (CurrentIndex - 1 + CameraDataAsset->SavedTransforms.Num()) % CameraDataAsset->SavedTransforms.Num();
	ApplyTransformToPlayer(CameraDataAsset->SavedTransforms[CurrentIndex]);
}

void UInGameSaveCameraTransformWidget::ApplyTransformToPlayer(const FTransform& TargetTransform)
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	PC->SetControlRotation(TargetTransform.Rotator());
	if (APawn* Pawn = PC->GetPawn())
	{
		Pawn->SetActorLocationAndRotation(TargetTransform.GetLocation(), TargetTransform.Rotator());
	}
}
