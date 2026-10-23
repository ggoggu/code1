// Fill out your copyright notice in the Description page of Project Settings.

#include "Editor/SaveCameraTransform/SaveCameaTransformEditorWidget.h"

#if WITH_EDITOR
#include "LevelEditor.h"
#include "EditorViewportClient.h"
#include "SLevelViewport.h"
#include "Editor/SaveCameraTransform/CameraTransfromDataAsset.h"

void USaveCameaTransformEditorWidget::SaveCurrentEditorCamera()
{
	if (!CameraDataAsset) return;

	// 현재 활성화된 에디터 뷰포트 찾기
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<SLevelViewport> ActiveViewport = LevelEditorModule.GetFirstActiveLevelViewport();

	if (ActiveViewport.IsValid())
	{
		FEditorViewportClient& Client = ActiveViewport->GetAssetViewportClient();
		
		FVector Location = Client.GetViewLocation();
		FRotator Rotation = Client.GetViewRotation();
		FTransform NewTransform(Rotation, Location, FVector::OneVector);

		CameraDataAsset->Modify(); // 변경사항 예약
		CurrentIndex = CameraDataAsset->SavedTransforms.Add(NewTransform);
		
		// 에셋 저장 상태 표시
		CameraDataAsset->MarkPackageDirty();
	}
}

void USaveCameaTransformEditorWidget::MoveToNextTransform()
{
	if (!CameraDataAsset || CameraDataAsset->SavedTransforms.IsEmpty()) return;

	CurrentIndex = (CurrentIndex + 1) % CameraDataAsset->SavedTransforms.Num();
	ApplyTransformToEditorCamera(CameraDataAsset->SavedTransforms[CurrentIndex]);
}

void USaveCameaTransformEditorWidget::MoveToPreviousTransform()
{
	if (!CameraDataAsset || CameraDataAsset->SavedTransforms.IsEmpty()) return;

	CurrentIndex = (CurrentIndex - 1 + CameraDataAsset->SavedTransforms.Num()) % CameraDataAsset->SavedTransforms.Num();
	ApplyTransformToEditorCamera(CameraDataAsset->SavedTransforms[CurrentIndex]);
}

void USaveCameaTransformEditorWidget::ApplyTransformToEditorCamera(const FTransform& TargetTransform)
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<SLevelViewport> ActiveViewport = LevelEditorModule.GetFirstActiveLevelViewport();

	if (ActiveViewport.IsValid())
	{
		FEditorViewportClient& Client = ActiveViewport->GetAssetViewportClient();
		
		// 에디터 뷰포트 카메라 위치/회전 강제 설정
		Client.SetViewLocation(TargetTransform.GetLocation());
		Client.SetViewRotation(TargetTransform.Rotator());
		Client.Invalidate(); // 뷰포트 갱신
	}
}
#endif
