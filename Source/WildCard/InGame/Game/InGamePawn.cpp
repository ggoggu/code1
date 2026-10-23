#include "InGame/Game/InGamePawn.h"
#include "InGame/Game/InGamePlayerState.h"
#include "InGame/Game/InGameState.h"
#include "Editor/SaveCameraTransform/CameraTransfromDataAsset.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerController.h"

// Sets default values
AInGamePawn::AInGamePawn()
{
	PrimaryActorTick.bCanEverTick = true;

	bAutoApplyCameraOnBeginPlay = true;
	bLockCameraTransform = true;

	Player1CameraIndex = 0;
	Player2CameraIndex = 1;
	DefaultCameraIndex = 0;

#if WITH_EDITORONLY_DATA
	PreviewPlayerIndex = 0;
#endif

	static ConstructorHelpers::FObjectFinder<UCameraTransfromDataAsset> CameraAssetFinder(TEXT("/Game/Editor/SaveCameraTrasform/CameraTransform.CameraTransform"));
	if (CameraAssetFinder.Succeeded())
	{
		CameraDataAsset = CameraAssetFinder.Object;
	}
}

void AInGamePawn::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#if WITH_EDITOR
	// 에디터에서 변수 값(인덱스, 에셋 등)을 수정했을 때 즉시 뷰포트에 반영
	if (GetWorld() && !GetWorld()->IsGameWorld())
	{
		ApplyCameraForPlayerIndex(PreviewPlayerIndex);
	}
#endif
}

// Called when the game starts or when spawned
void AInGamePawn::BeginPlay()
{
	Super::BeginPlay();
	
	if (bAutoApplyCameraOnBeginPlay && GetNetMode() != NM_DedicatedServer)
	{
		ApplyCameraSettings();
	}
}

void AInGamePawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (bAutoApplyCameraOnBeginPlay && GetNetMode() != NM_DedicatedServer)
	{
		ApplyCameraSettings();
	}
}

void AInGamePawn::Restart()
{
	Super::Restart();

	if (bAutoApplyCameraOnBeginPlay && IsLocallyControlled())
	{
		ApplyCameraSettings();
	}
}

void AInGamePawn::ApplyCameraSettings()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const int32 PlayerIdx = GetLocalPlayerIndex();
	ApplyCameraForPlayerIndex(PlayerIdx);
}

void AInGamePawn::ApplyCameraForPlayerIndex(int32 InPlayerIndex)
{
	const int32 TargetIndex = (InPlayerIndex == 1) ? Player2CameraIndex : Player1CameraIndex;
	ApplySavedCameraTransform(TargetIndex);
}

void AInGamePawn::ApplySavedCameraTransform(int32 Index)
{
	if (!CameraDataAsset)
	{
		CameraDataAsset = LoadObject<UCameraTransfromDataAsset>(nullptr, TEXT("/Game/Editor/SaveCameraTrasform/CameraTransform.CameraTransform"));
	}

	if (!CameraDataAsset || !CameraDataAsset->SavedTransforms.IsValidIndex(Index))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InGamePawn] 유효하지 않은 CameraDataAsset 인덱스입니다: %d (총 %d개 저장됨)"),
			Index, CameraDataAsset ? CameraDataAsset->SavedTransforms.Num() : 0);
		return;
	}

	const FTransform& TargetTransform = CameraDataAsset->SavedTransforms[Index];
	SetActorLocationAndRotation(TargetTransform.GetLocation(), TargetTransform.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetControlRotation(TargetTransform.Rotator());
	}

	LockedCameraTransform = TargetTransform;
	bHasAppliedCamera = true;

	UE_LOG(LogTemp, Log, TEXT("[InGamePawn] CameraDataAsset 인덱스 %d 적용 완료: Location(%s), Rotation(%s)"),
		Index, *TargetTransform.GetLocation().ToString(), *TargetTransform.Rotator().ToString());
}

int32 AInGamePawn::GetLocalPlayerIndex() const
{
#if WITH_EDITOR
	if (GetWorld() && !GetWorld()->IsGameWorld())
	{
		return PreviewPlayerIndex;
	}
#endif

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC && GetWorld())
	{
		PC = GetWorld()->GetFirstPlayerController();
	}

	if (PC)
	{
		if (AInGamePlayerState* PS = PC->GetPlayerState<AInGamePlayerState>())
		{
			if (PS->GetPlayerCount() >= 0)
			{
				return PS->GetPlayerCount();
			}

			if (AInGameState* GS = GetWorld()->GetGameState<AInGameState>())
			{
				int32 FoundIdx = GS->PlayerArray.IndexOfByKey(PS);
				if (FoundIdx != INDEX_NONE)
				{
					return FoundIdx;
				}
			}

			int32 RawId = PS->GetPlayerId();
			if (RawId >= 0)
			{
				return RawId % 2;
			}
		}
	}

	return 0;
}

// Called every frame
void AInGamePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 카메라 고정 옵션이 켜져 있는 경우, 외력/입력에 의한 카메라 흔들림 방지
	if (bLockCameraTransform && bHasAppliedCamera && GetNetMode() != NM_DedicatedServer)
	{
		if (!GetActorLocation().Equals(LockedCameraTransform.GetLocation(), 0.1f) ||
			!GetActorRotation().Equals(LockedCameraTransform.Rotator(), 0.1f))
		{
			SetActorLocationAndRotation(LockedCameraTransform.GetLocation(), LockedCameraTransform.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				PC->SetControlRotation(LockedCameraTransform.Rotator());
			}
		}
	}
}

// Called to bind functionality to input
void AInGamePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AInGamePawn::Server_MoveUnitActor_Implementation(FVector InHasUnitTileLocation, FVector InEmptyTileLocation)
{
}

bool AInGamePawn::Server_MoveUnitActor_Validate(FVector InHasUnitTileLocation, FVector InEmptyTileLocation)
{
	return true;
}

void AInGamePawn::Server_FightUnitActor_Implementation(const FString& InAttacker, const FString& InDefender)
{
}

bool AInGamePawn::Server_FightUnitActor_Validate(const FString& InAttacker, const FString& InDefender)
{
	return true;
}
