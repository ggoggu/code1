// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InGamePawn.generated.h"

class UCameraTransfromDataAsset;
class AInGamePlayerState;

UCLASS()
class WILDCARD_API AInGamePawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AInGamePawn();

	virtual void OnConstruction(const FTransform& Transform) override;

	// =========================================================================
	// [CameraDataAsset 기반 플레이어별 카메라 인덱스 설정]
	// =========================================================================
	/** 카메라 위치 및 회전이 저장된 데이터 에셋 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WildCard|Camera")
	TObjectPtr<UCameraTransfromDataAsset> CameraDataAsset;

	/** 플레이어 1(Index 0)에게 적용할 CameraDataAsset 인덱스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WildCard|Camera")
	int32 Player1CameraIndex = 0;

	/** 플레이어 2(Index 1)에게 적용할 CameraDataAsset 인덱스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WildCard|Camera")
	int32 Player2CameraIndex = 1;

	/** 기본 카메라 인덱스 (기존 호환용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WildCard|Camera")
	int32 DefaultCameraIndex = 0;

	/** 게임 시작(BeginPlay) 및 컨트롤러 빙의(Possess) 시 자동으로 플레이어 카메라를 적용할지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WildCard|Camera")
	bool bAutoApplyCameraOnBeginPlay = true;

	/** 카메라 위치 및 회전을 고정 유지할지 여부 (외력/드래그 밀림 방지) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WildCard|Camera")
	bool bLockCameraTransform = true;

	// =========================================================================
	// [에디터 미리보기 (Editor Preview)]
	// =========================================================================
#if WITH_EDITORONLY_DATA
	/** 에디터 뷰포트에서 실시간 미리보기할 플레이어 번호 (0: 플레이어 1, 1: 플레이어 2) - 값 수정 시 즉시 뷰포트 반영 */
	UPROPERTY(EditAnywhere, Category = "WildCard|Camera|EditorPreview", meta = (ClampMin = "0", ClampMax = "1"))
	int32 PreviewPlayerIndex = 0;
#endif

	// =========================================================================
	// [카메라 제어 함수]
	// =========================================================================
	/** 현재 로컬 플레이어에 맞는 카메라를 즉시 적용 (에디터 디테일 패널 버튼으로도 클릭 가능) */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "WildCard|Camera")
	void ApplyCameraSettings();

	/** 지정된 플레이어 번호(0: P1, 1: P2)에 맞게 카메라 즉시 적용 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Camera")
	void ApplyCameraForPlayerIndex(int32 InPlayerIndex);

	/** 지정한 SavedTransforms 인덱스의 카메라 위치/각도를 즉시 적용 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Camera")
	void ApplySavedCameraTransform(int32 Index);

	/** 로컬 플레이어 번호(0 또는 1) 확인 */
	UFUNCTION(BlueprintCallable, Category = "WildCard|Camera")
	int32 GetLocalPlayerIndex() const;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void Restart() override;

	UPROPERTY(Transient)
	FTransform LockedCameraTransform;

	UPROPERTY(Transient)
	bool bHasAppliedCamera = false;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveUnitActor(FVector InHasUnitTileLocation, FVector InEmptyTileLocation);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_FightUnitActor(const FString& InAttacker, const FString& InDefender);
};
