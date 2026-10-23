// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "SaveCameaTransformEditorWidget.generated.h"

class UCameraTransfromDataAsset;

/**
 * 
 */
UCLASS()
class WILDCARD_API USaveCameaTransformEditorWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	// 데이터 에셋 (블루프린트 디테일창에서 할당)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Tool")
	TObjectPtr<UCameraTransfromDataAsset> CameraDataAsset;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Camera Tool")
	int32 CurrentIndex = -1;

	// 버튼에 바인딩할 함수들
	UFUNCTION(BlueprintCallable, Category = "Camera Tool")
	void SaveCurrentEditorCamera();

	UFUNCTION(BlueprintCallable, Category = "Camera Tool")
	void MoveToNextTransform();

	UFUNCTION(BlueprintCallable, Category = "Camera Tool")
	void MoveToPreviousTransform();

private:
	// 에디터 뷰포트 카메라 위치를 실제로 옮기는 로직
	void ApplyTransformToEditorCamera(const FTransform& TargetTransform);
};
