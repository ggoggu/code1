// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CardBase.h"
#include "BattleData.h"
#include "Components/WidgetComponent.h"
#include "UnitBase.generated.h"

class ATile;
struct FWildActionData;
class EffectBase;
class USceneComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTexture2D;
class USelectActionWidget;
class UTileSelectComponent;

UENUM(BlueprintType)
enum class EUnitBillboardMode : uint8
{
	FullSpherical  UMETA(DisplayName = "Full Spherical (Pitch + Yaw)"),
	CylindricalYaw UMETA(DisplayName = "Cylindrical (Yaw Only)"),
	CameraAligned  UMETA(DisplayName = "Camera Aligned (Screen Parallel)"),
	Disabled       UMETA(DisplayName = "Disabled")
};

UCLASS()
class WILDCARD_API AUnitBase : public AActor
{
	GENERATED_BODY()

public:
	AUnitBase();

	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitializeComponents() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** 액터의 모든 컴포넌트(메시, 위젯 등)의 마우스 클릭 트레이스 채널 충돌을 Ignore로 설정하는 통합 함수 */
	UFUNCTION(BlueprintCallable, Category = "Unit|Collision")
	static void DisableActorMouseClickCollision(AActor* Actor);

	/** 이 유닛의 모든 컴포넌트(메시, 위젯 등)의 마우스 클릭 트레이스 채널 충돌을 Ignore로 일괄 설정 */
	UFUNCTION(BlueprintCallable, Category = "Unit|Collision")
	void DisableMouseClickCollision();

	// ==== 3D Card Visual & Billboard ====
	UFUNCTION(BlueprintCallable, Category = "Unit|Visual")
	virtual void UpdateCardVisual();

	UFUNCTION(BlueprintCallable, Category = "Unit|Visual")
	virtual void SetCardIllustration(UTexture2D* NewTexture);

	UFUNCTION(BlueprintCallable, Category = "Unit|Visual")
	virtual UTexture2D* GetCardIllustrationTexture() const;

	UFUNCTION(BlueprintCallable, Category = "Unit|Billboard")
	void UpdateFacingCamera();

	// ==== Card Interaction & Feedback Visual ====
	UFUNCTION(BlueprintCallable, Category = "Unit|Visual")
	virtual void SetCardSelected(bool bInSelected);

	UFUNCTION(BlueprintCallable, Category = "Unit|Visual")
	virtual void SetCardHovered(bool bInHovered);

	UFUNCTION(BlueprintCallable, Category = "Unit|Visual")
	virtual void SetCardHighlightColor(const FLinearColor& InHighlightColor);

	UFUNCTION(BlueprintCallable, Category = "Unit|Visual")
	virtual void PlayCardHitReaction();

	UFUNCTION(BlueprintCallable, Category = "Unit|Visual")
	virtual void PlayCardActivationEffect();

	// ==== Visual Movement (Client-side smooth interpolation) ====
	void StartVisualMove(const FVector& FromWorld, const FVector& ToWorld);
	void UpdateVisualMove(float DeltaTime);

	// ==== Combat & Effect Visual (단일 NetMulticast 이벤트 버스 연동 및 로컬 연출) ====
	void PlayAttackVisualInternal(AUnitBase* TargetDefender);

	UFUNCTION()
	void HandleVisualCueReceived(const FWildVisualCueData& CueData);

	void BindToGameStateVisualCue();

	// ==== 하위 호환성 유지용 레거시 NetMulticast (신규 코드는 Multicast_BroadcastVisualCue 사용) ====
	UFUNCTION(NetMulticast, Unreliable, Category = "Unit|Combat")
	void Multicast_PlayAttackVisual(AUnitBase* TargetDefender);

	UFUNCTION(NetMulticast, Unreliable, Category = "Unit|Combat")
	void Multicast_PlayHitReaction();

	UFUNCTION(NetMulticast, Unreliable, Category = "Unit|Effect")
	void Multicast_PlayEffectTriggerVisual();


	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|Components")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|Components")
	TObjectPtr<USceneComponent> CardVisualRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|Components")
	TObjectPtr<UStaticMeshComponent> CardMeshComp;

	// Action Menu UI (WidgetComponent Benchmark & Alternative)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Unit|UI")
	TObjectPtr<UWidgetComponent> ActionMenuWidgetComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|UI")
	TSubclassOf<class USelectActionWidget> ActionMenuWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|UI|Benchmark")
	EWidgetSpace ActionMenuWidgetSpace = EWidgetSpace::Screen;

	UFUNCTION(BlueprintCallable, Category = "Unit|UI")
	virtual void ShowActionMenu(EWildCardPhase CurrentPhase, bool bHasPriority, class UTileSelectComponent* TileSelectComp);

	UFUNCTION(BlueprintCallable, Category = "Unit|UI")
	virtual void HideActionMenu();

	UFUNCTION(BlueprintCallable, Category = "Unit|UI")
	void UpdateActionMenuFacingCamera();

	// Visual & Billboard Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Billboard")
	bool bEnableFacingCamera = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Billboard")
	EUnitBillboardMode BillboardMode = EUnitBillboardMode::FullSpherical;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Billboard")
	FRotator BillboardRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Visual")
	FVector CardMeshScale = FVector(0.03f, 0.7f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Visual")
	FVector CardMeshOffset = FVector(0.0f, 0.0f, 50.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Unit|Visual")
	TObjectPtr<UMaterialInterface> CardMaterialBase;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Visual")
	TSoftObjectPtr<UTexture2D> CardBackImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Visual")
	FLinearColor SelectedHighlightColor = FLinearColor(1.0f, 0.84f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Visual")
	FLinearColor HoverHighlightColor = FLinearColor(0.2f, 0.7f, 1.0f, 1.0f);

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Unit|Visual")
	TObjectPtr<UMaterialInstanceDynamic> CardDynMaterial;

	bool bIsCardSelected = false;
	bool bIsCardHovered = false;
	bool bHasBoundVisualCue = false;
	FTimerHandle HitReactionTimerHandle;

	// Visual Movement state
	bool bIsVisualMoving = false;
	FVector VisualMoveStartOffset = FVector::ZeroVector;
	float VisualMoveAlpha = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Visual|Movement")
	float VisualMoveDuration = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Unit|Visual|Movement")
	float VisualMoveArcHeight = 35.0f;

	FVector LastKnownTileLocation = FVector::ZeroVector;

	// ==== getter ==== 
	virtual int32 GetCardHealth() { return UnitCardData.Health; }
	int32 GetCardAttack() { return UnitCardData.Attack; }
	ATile* GetCurrentTile() { return CurrentTile; }

	UFUNCTION(BlueprintCallable)
	int32 GetOwnerId() { return UnitCardData.OwnerId; }

	// ----- setter ----
	UFUNCTION(BlueprintCallable, Category = "Unit|Stat")
	void SetCardAttack(int32 InAttack);

	UFUNCTION(BlueprintCallable, Category = "Unit|Stat")
	virtual void SetCardHealth(int32 InHealth);

	UFUNCTION(BlueprintCallable, Category = "Unit|Tile")
	void SetCurrentTile(ATile* InTile) { CurrentTile = InTile; }

	// ----- Init -----
	void InitUnitCardData(FCardData InCardData, int32 InOwnerId, int32 InCardInstanceID);

	//#. Function in BP
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetWidgetScore();

	UFUNCTION(BlueprintCallable)
	FUnitCardData GetUnitCardData() { return UnitCardData; }

	FCardData GetCardData() { return CardData; }

	// ==== Movement & Abilities ====
	UFUNCTION(BlueprintCallable, Category = "Unit|Movement")
	virtual int32 GetMaxMoveDistance() const;

	UFUNCTION(BlueprintCallable, Category = "Unit|Movement")
	bool IsFlying() const;

	UFUNCTION()
	void OnRep_UnitCardDataChanged();

	UFUNCTION()
	void OnRep_CardDataChanged();

	UFUNCTION()
	void OnRep_CurrentTileChanged();

	virtual void HandleDeath();

	int32 CardInstanceID = -1;

	int32 MaxHealth = 0;
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void EnsureDynamicMaterial();

	UPROPERTY(ReplicatedUsing = OnRep_UnitCardDataChanged)
	FUnitCardData UnitCardData;

	//#. 카드의 기본 정보 (DB에서 받아오는 정보)
	//#. 후에 소멸,추방 후 부활시 다시 사용될 원본 데이터
	UPROPERTY(ReplicatedUsing = OnRep_CardDataChanged)
	FCardData CardData;

	FWildActionData CurrentActionData;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentTileChanged)
	TObjectPtr<ATile> CurrentTile;
};
