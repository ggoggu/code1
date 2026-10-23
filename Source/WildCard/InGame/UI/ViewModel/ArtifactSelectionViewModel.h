// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "ArtifactItemViewModel.h"
#include "ArtifactSelectionViewModel.generated.h"

/**
 * 
 */
UCLASS()
class WILDCARD_API UArtifactSelectionViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	// UI에 바인딩될 성유물 리스트 
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Artifact")
	TArray<class UArtifactItemViewModel*> AvailableArtifacts;

	// UI 표시/숨김 및 대기 상태 제어용
	UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Artifact")
	bool bIsGamesetActive;

	// 서버/데이터 테이블로부터 받은 성유물 목록으로 하위 아이템 세팅
	void SetupAvailableArtifacts(const TArray<FArtifactData>& InArtifacts);

	// 단일 선택 처리용 함수 (UI에서 특정 아이템을 클릭했을 때 호출)
	UFUNCTION(BlueprintCallable, Category = "Artifact")
	void SelectArtifact(UArtifactItemViewModel* SelectedVM);

	// 컨트롤러에서 제출 버튼을 누를 때 선택된 ID를 가져감
	UFUNCTION(BlueprintCallable, Category = "Artifact")
	FName GetSelectedArtifactID() const;

	// C++ 로직용 빠른 Setter
	void SetGamesetActive(bool bActive);
	
};
