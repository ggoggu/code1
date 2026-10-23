#include "InGame/Unit/UnitBase.h"
#include "WildCard.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "InGameState.h"
#include "ChainComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "UObject/ConstructorHelpers.h"
#include "InGame/UI/Widget/SelectActionWidget.h"
#include "InGame/Effect/TileSelectComponent.h"
#include "Tile.h"

// Sets default values
AUnitBase::AUnitBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	CardVisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CardVisualRoot"));
	CardVisualRoot->SetupAttachment(RootComponent);

	CardMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CardMeshComp"));
	CardMeshComp->SetupAttachment(CardVisualRoot);
	CardMeshComp->SetRelativeScale3D(CardMeshScale);
	CardMeshComp->SetRelativeLocation(CardMeshOffset);
	CardMeshComp->SetCollisionResponseToChannel(ECC_MouseClick, ECR_Ignore);
	CardMeshComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultCubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultCubeMesh.Succeeded())
	{
		CardMeshComp->SetStaticMesh(DefaultCubeMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultCardMat(TEXT("/Game/Mesh/AdvancedPortalsSystemVFX/Materials/M_Portal_Image.M_Portal_Image"));
	if (DefaultCardMat.Succeeded())
	{
		CardMaterialBase = DefaultCardMat.Object;
	}

	CardBackImage = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/InGame/UI/Texture/CardFrame/BackCardImage.BackCardImage")));

	ActionMenuWidgetSpace = EWidgetSpace::Screen;

	ActionMenuWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("ActionMenuWidgetComp"));
	ActionMenuWidgetComp->SetupAttachment(RootComponent);
	ActionMenuWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 110.f));
	ActionMenuWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	ActionMenuWidgetComp->SetDrawAtDesiredSize(true);
	ActionMenuWidgetComp->SetPivot(FVector2D(0.5f, 1.0f));
	ActionMenuWidgetComp->SetVisibility(false);
	ActionMenuWidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FClassFinder<USelectActionWidget> ActionWidgetBP(
		TEXT("/Game/InGame/UI/BP/Unit/WBP_SelectActionWidget.WBP_SelectActionWidget_C")
	);
	if (ActionWidgetBP.Succeeded())
	{
		ActionMenuWidgetClass = ActionWidgetBP.Class;
	}
	else
	{
		static ConstructorHelpers::FClassFinder<USelectActionWidget> FallbackBP(
			TEXT("/Game/InGame/UI/BP/Unit/BP_SelectWidget.BP_SelectWidget_C")
		);
		if (FallbackBP.Succeeded())
		{
			ActionMenuWidgetClass = FallbackBP.Class;
		}
		else
		{
			ActionMenuWidgetClass = USelectActionWidget::StaticClass();
		}
	}
}

void AUnitBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsVisualMoving)
	{
		UpdateVisualMove(DeltaTime);
	}

	if (bEnableFacingCamera)
	{
		UpdateFacingCamera();
		UpdateActionMenuFacingCamera();
	}
}

void AUnitBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	DisableMouseClickCollision();
}

void AUnitBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	DisableMouseClickCollision();

	if (CardMeshComp)
	{
		CardMeshComp->SetRelativeScale3D(CardMeshScale);
		CardMeshComp->SetRelativeLocation(CardMeshOffset);
	}
}

// Called when the game starts or when spawned
void AUnitBase::BeginPlay()
{
	Super::BeginPlay();
	DisableMouseClickCollision();
	UpdateCardVisual();
	LastKnownTileLocation = GetActorLocation();

	if (!IsRunningDedicatedServer())
	{
		BindToGameStateVisualCue();
	}

	if (ActionMenuWidgetComp)
	{
		ActionMenuWidgetComp->SetVisibility(false);
		ActionMenuWidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (ActionMenuWidgetClass && !ActionMenuWidgetComp->GetWidgetClass())
		{
			ActionMenuWidgetComp->SetWidgetClass(ActionMenuWidgetClass);
		}
	}
}

void AUnitBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bHasBoundVisualCue)
	{
		if (UWorld* World = GetWorld())
		{
			if (AInGameState* GS = World->GetGameState<AInGameState>())
			{
				GS->OnVisualCueReceivedDelegate.RemoveDynamic(this, &AUnitBase::HandleVisualCueReceived);
			}
		}
		bHasBoundVisualCue = false;
	}

	Super::EndPlay(EndPlayReason);
}

void AUnitBase::BindToGameStateVisualCue()
{
	if (bHasBoundVisualCue || IsRunningDedicatedServer()) return;

	if (UWorld* World = GetWorld())
	{
		if (AInGameState* GS = World->GetGameState<AInGameState>())
		{
			GS->OnVisualCueReceivedDelegate.RemoveDynamic(this, &AUnitBase::HandleVisualCueReceived);
			GS->OnVisualCueReceivedDelegate.AddDynamic(this, &AUnitBase::HandleVisualCueReceived);
			bHasBoundVisualCue = true;
			return;
		}

		// GameState가 아직 초기화되지 않은 클라이언트 접속 초기 시점이면 재시도 등록
		FTimerHandle RetryHandle;
		World->GetTimerManager().SetTimer(RetryHandle, this, &AUnitBase::BindToGameStateVisualCue, 0.2f, false);
	}
}


void AUnitBase::DisableActorMouseClickCollision(AActor* Actor)
{
	if (!Actor) return;

	// 액터의 모든 컴포넌트(StaticMesh, SkeletalMesh, Cube 메시, 스탯 위젯 등)는
	// 마우스 클릭 트레이스 채널(ECC_MouseClick)을 무시하여, 타일 위에 유닛이 있어도 타일만 클릭되도록 보장
	TArray<UPrimitiveComponent*> PrimComps;
	Actor->GetComponents<UPrimitiveComponent>(PrimComps);
	for (UPrimitiveComponent* PrimComp : PrimComps)
	{
		if (PrimComp)
		{
			PrimComp->SetCollisionResponseToChannel(ECC_MouseClick, ECR_Ignore);
		}
	}
}

void AUnitBase::DisableMouseClickCollision()
{
	DisableActorMouseClickCollision(this);
}

void AUnitBase::SetCardAttack(int32 InAttack)
{
	UnitCardData.Attack = InAttack;

	SetWidgetScore();
}

void AUnitBase::SetCardHealth(int32 InHealth)
{
	UnitCardData.Health = InHealth;

	if (UnitCardData.Health <= 0)
	{
		HandleDeath();
	}

	if(InHealth > MaxHealth)
	{
		UnitCardData.Health = MaxHealth;
	}

	SetWidgetScore();
}

void AUnitBase::HandleDeath()
{
	Destroy();
}

void AUnitBase::InitUnitCardData(FCardData InCardData, int32 InOwnerId, int32 InCardInstanceID)
{
	CardData = InCardData;
	UnitCardData = FUnitCardData(InCardData, InOwnerId);
	CardInstanceID = InCardInstanceID;
	MaxHealth = InCardData.Health;
	UE_LOG(LogTemp, Warning, TEXT("Card Initialized! Card Health: %d, OwnerID: %d"), UnitCardData.Health, UnitCardData.OwnerId);

	UpdateCardVisual();

	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (GameState == nullptr)
		return;

	UChainComponent* ChainComponent = GameState->FindComponentByClass<UChainComponent>();
	if (ChainComponent == nullptr)
		return;

	//#. ActionData 생성 후 ChainComponent에 추가
	FWildActionData ActionData;
	//
	ActionData.ActionCategory = EWildActionCategory::Main;
	ActionData.ActionType = EWildActionType::SummonUnit;

	ActionData.TargetTile = CurrentTile;
	ActionData.PlayerID = UnitCardData.OwnerId;
	//
	ChainComponent->AddActionToChain(ActionData);

	//#. 서버에서 widget도 설정
	SetWidgetScore();
}

void AUnitBase::OnRep_UnitCardDataChanged()
{
	UpdateCardVisual();
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AUnitBase::SetWidgetScore);
}

void AUnitBase::OnRep_CardDataChanged()
{
	UpdateCardVisual();
}

void AUnitBase::OnRep_CurrentTileChanged()
{
	if (!IsRunningDedicatedServer() && CurrentTile)
	{
		FVector NewLocation = CurrentTile->GetActorLocation();
		if (!LastKnownTileLocation.IsNearlyZero() && FVector::Dist2D(LastKnownTileLocation, NewLocation) > 10.0f)
		{
			StartVisualMove(LastKnownTileLocation, NewLocation);
		}
		LastKnownTileLocation = NewLocation;
	}
}

void AUnitBase::UpdateFacingCamera()
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

void AUnitBase::EnsureDynamicMaterial()
{
	if (CardDynMaterial || !CardMeshComp)
	{
		return;
	}

	UMaterialInterface* BaseMat = CardMaterialBase;
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
		CardDynMaterial = UMaterialInstanceDynamic::Create(BaseMat, this);
		CardMeshComp->SetMaterial(0, CardDynMaterial);

		// 뒷면 텍스처 바인딩
		if (CardBackImage.IsValid())
		{
			CardDynMaterial->SetTextureParameterValue(FName("BackCardTexture"), CardBackImage.Get());
			CardDynMaterial->SetTextureParameterValue(FName("CardBack"), CardBackImage.Get());
		}
		else if (!CardBackImage.IsNull())
		{
			if (UTexture2D* LoadedBack = CardBackImage.LoadSynchronous())
			{
				CardDynMaterial->SetTextureParameterValue(FName("BackCardTexture"), LoadedBack);
				CardDynMaterial->SetTextureParameterValue(FName("CardBack"), LoadedBack);
			}
		}
	}
}

UTexture2D* AUnitBase::GetCardIllustrationTexture() const
{
	return CardData.GetCardImage();
}

void AUnitBase::SetCardIllustration(UTexture2D* NewTexture)
{
	if (!NewTexture)
	{
		return;
	}

	EnsureDynamicMaterial();
	if (CardDynMaterial)
	{
		CardDynMaterial->SetTextureParameterValue(FName("Portal Texture"), NewTexture);
		CardDynMaterial->SetTextureParameterValue(FName("CardTexture"), NewTexture);
		CardDynMaterial->SetTextureParameterValue(FName("CardImage"), NewTexture);
		CardDynMaterial->SetTextureParameterValue(FName("BaseColorTexture"), NewTexture);
		CardDynMaterial->SetTextureParameterValue(FName("Texture"), NewTexture);
		CardDynMaterial->SetTextureParameterValue(FName("texture"), NewTexture);
		CardDynMaterial->SetTextureParameterValue(FName("Albedo"), NewTexture);
	}
}

void AUnitBase::UpdateCardVisual()
{
	if (UTexture2D* CardTexture = GetCardIllustrationTexture())
	{
		SetCardIllustration(CardTexture);
	}

	if (UTexture2D* FrameTexture = CardData.GetCardFrameImage())
	{
		EnsureDynamicMaterial();
		if (CardDynMaterial)
		{
			CardDynMaterial->SetTextureParameterValue(FName("CardFrameTexture"), FrameTexture);
			CardDynMaterial->SetTextureParameterValue(FName("CardFrame"), FrameTexture);
			CardDynMaterial->SetTextureParameterValue(FName("FrameTexture"), FrameTexture);
		}
	}
}

void AUnitBase::SetCardSelected(bool bInSelected)
{
	bIsCardSelected = bInSelected;
	EnsureDynamicMaterial();
	if (CardDynMaterial)
	{
		CardDynMaterial->SetScalarParameterValue(FName("IsSelected"), bInSelected ? 1.0f : 0.0f);
		if (bInSelected)
		{
			CardDynMaterial->SetVectorParameterValue(FName("HighlightColor"), SelectedHighlightColor);
			CardDynMaterial->SetScalarParameterValue(FName("HighlightIntensity"), 1.5f);
		}
		else if (bIsCardHovered)
		{
			CardDynMaterial->SetVectorParameterValue(FName("HighlightColor"), HoverHighlightColor);
			CardDynMaterial->SetScalarParameterValue(FName("HighlightIntensity"), 1.0f);
		}
		else
		{
			CardDynMaterial->SetScalarParameterValue(FName("HighlightIntensity"), 0.0f);
		}
	}
}

void AUnitBase::SetCardHovered(bool bInHovered)
{
	bIsCardHovered = bInHovered;
	if (bIsCardSelected)
	{
		return;
	}

	EnsureDynamicMaterial();
	if (CardDynMaterial)
	{
		CardDynMaterial->SetScalarParameterValue(FName("IsHovered"), bInHovered ? 1.0f : 0.0f);
		if (bInHovered)
		{
			CardDynMaterial->SetVectorParameterValue(FName("HighlightColor"), HoverHighlightColor);
			CardDynMaterial->SetScalarParameterValue(FName("HighlightIntensity"), 1.0f);
		}
		else
		{
			CardDynMaterial->SetScalarParameterValue(FName("HighlightIntensity"), 0.0f);
		}
	}
}

void AUnitBase::SetCardHighlightColor(const FLinearColor& InHighlightColor)
{
	EnsureDynamicMaterial();
	if (CardDynMaterial)
	{
		CardDynMaterial->SetVectorParameterValue(FName("HighlightColor"), InHighlightColor);
	}
}

void AUnitBase::PlayCardHitReaction()
{
	EnsureDynamicMaterial();
	if (CardDynMaterial)
	{
		CardDynMaterial->SetVectorParameterValue(FName("FlashColor"), FLinearColor(1.0f, 0.1f, 0.1f, 1.0f));
		CardDynMaterial->SetScalarParameterValue(FName("FlashIntensity"), 1.0f);

		GetWorld()->GetTimerManager().ClearTimer(HitReactionTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(HitReactionTimerHandle, [this]()
		{
			if (CardDynMaterial)
			{
				CardDynMaterial->SetScalarParameterValue(FName("FlashIntensity"), 0.0f);
			}
		}, 0.25f, false);
	}
}

void AUnitBase::PlayCardActivationEffect()
{
	EnsureDynamicMaterial();
	if (CardDynMaterial)
	{
		CardDynMaterial->SetVectorParameterValue(FName("FlashColor"), FLinearColor(0.2f, 0.9f, 1.0f, 1.0f));
		CardDynMaterial->SetScalarParameterValue(FName("FlashIntensity"), 1.5f);

		GetWorld()->GetTimerManager().ClearTimer(HitReactionTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(HitReactionTimerHandle, [this]()
		{
			if (CardDynMaterial)
			{
				CardDynMaterial->SetScalarParameterValue(FName("FlashIntensity"), 0.0f);
			}
		}, 0.35f, false);
	}
}

void AUnitBase::StartVisualMove(const FVector& FromWorld, const FVector& ToWorld)
{
	if (!CardVisualRoot) return;

	float Dist = FVector::Dist2D(FromWorld, ToWorld);
	if (Dist > 2000.0f || Dist < 5.0f)
	{
		CardVisualRoot->SetRelativeLocation(FVector::ZeroVector);
		return;
	}

	VisualMoveStartOffset = FromWorld - ToWorld;
	VisualMoveAlpha = 0.0f;
	bIsVisualMoving = true;
	CardVisualRoot->SetRelativeLocation(VisualMoveStartOffset);
}

void AUnitBase::UpdateVisualMove(float DeltaTime)
{
	if (!bIsVisualMoving || !CardVisualRoot) return;

	VisualMoveAlpha += DeltaTime / FMath::Max(0.01f, VisualMoveDuration);
	if (VisualMoveAlpha >= 1.0f)
	{
		VisualMoveAlpha = 1.0f;
		bIsVisualMoving = false;
		CardVisualRoot->SetRelativeLocation(FVector::ZeroVector);
	}
	else
	{
		float SmoothAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, VisualMoveAlpha, 2.0f);
		FVector CurrentOffset = FMath::Lerp(VisualMoveStartOffset, FVector::ZeroVector, SmoothAlpha);
		float ArcOffset = FMath::Sin(VisualMoveAlpha * PI) * VisualMoveArcHeight;
		CurrentOffset.Z += ArcOffset;
		CardVisualRoot->SetRelativeLocation(CurrentOffset);
	}
}

void AUnitBase::PlayAttackVisualInternal(AUnitBase* TargetDefender)
{
	if (IsRunningDedicatedServer()) return;

	PlayCardActivationEffect();

	if (TargetDefender)
	{
		TargetDefender->PlayCardHitReaction();

		if (CardVisualRoot && !bIsVisualMoving)
		{
			FVector LungeDir = (TargetDefender->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
			FVector LungeOffset = LungeDir * 35.0f;
			CardVisualRoot->SetRelativeLocation(LungeOffset);

			FTimerHandle LungeTimer;
			GetWorld()->GetTimerManager().SetTimer(LungeTimer, [this]()
			{
				if (CardVisualRoot && !bIsVisualMoving)
				{
					CardVisualRoot->SetRelativeLocation(FVector::ZeroVector);
				}
			}, 0.15f, false);
		}
	}
}

void AUnitBase::HandleVisualCueReceived(const FWildVisualCueData& CueData)
{
	if (IsRunningDedicatedServer()) return;

	// Visual.Action.Attack 큐 수신 시 본인이 공격자인 경우 유닛 런지 및 피격자 히트 리액션 실행
	if (CueData.CueTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Visual.Action.Attack"), false)))
	{
		if (CueData.SourceActor.Get() == this)
		{
			AUnitBase* TargetDefender = Cast<AUnitBase>(CueData.TargetActor.Get());
			PlayAttackVisualInternal(TargetDefender);
		}
	}
}

void AUnitBase::Multicast_PlayAttackVisual_Implementation(AUnitBase* TargetDefender)
{
	// 하위 호환성 유지: 레거시 NetMulticast RPC 호출 시에도 로컬 연출 실행
	PlayAttackVisualInternal(TargetDefender);
}


void AUnitBase::Multicast_PlayHitReaction_Implementation()
{
	if (IsRunningDedicatedServer()) return;

	PlayCardHitReaction();
}

void AUnitBase::Multicast_PlayEffectTriggerVisual_Implementation()
{
	if (IsRunningDedicatedServer()) return;

	PlayCardActivationEffect();
}

void AUnitBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AUnitBase, UnitCardData);
	DOREPLIFETIME(AUnitBase, CurrentTile);
	DOREPLIFETIME(AUnitBase, CardData);
}

bool AUnitBase::IsFlying() const
{
	if (CardData.CardKeyword == ECardKeyword::FLYING)
	{
		return true;
	}

	static const FGameplayTag FlyingTag = FGameplayTag::RequestGameplayTag(FName("Keyword.Flying"), false);
	if (FlyingTag.IsValid() && CardData.CardKeywords.HasTag(FlyingTag))
	{
		return true;
	}

	return false;
}

int32 AUnitBase::GetMaxMoveDistance() const
{
	if (IsFlying())
	{
		return 2;
	}

	return 1;
}

void AUnitBase::ShowActionMenu(EWildCardPhase CurrentPhase, bool bHasPriority, UTileSelectComponent* TileSelectComp)
{
	if (!ActionMenuWidgetComp) return;

	ActionMenuWidgetComp->SetWidgetSpace(ActionMenuWidgetSpace);

	UUserWidget* UserWidget = ActionMenuWidgetComp->GetUserWidgetObject();
	if (!UserWidget && ActionMenuWidgetClass)
	{
		ActionMenuWidgetComp->SetWidgetClass(ActionMenuWidgetClass);
		ActionMenuWidgetComp->InitWidget();
		UserWidget = ActionMenuWidgetComp->GetUserWidgetObject();
	}

	if (USelectActionWidget* ActionWidget = Cast<USelectActionWidget>(UserWidget))
	{
		ActionWidget->SetupForGeneralUnit(this, CurrentPhase, bHasPriority);

		if (TileSelectComp)
		{
			ActionWidget->OnMoveButtonPressed.RemoveDynamic(TileSelectComp, &UTileSelectComponent::SelectMoveAction);
			ActionWidget->OnMoveButtonPressed.AddDynamic(TileSelectComp, &UTileSelectComponent::SelectMoveAction);
			ActionWidget->OnCombatButtonPressed.RemoveDynamic(TileSelectComp, &UTileSelectComponent::SelectAttackAction);
			ActionWidget->OnCombatButtonPressed.AddDynamic(TileSelectComp, &UTileSelectComponent::SelectAttackAction);
			ActionWidget->OnEffectButtonPressed.RemoveDynamic(TileSelectComp, &UTileSelectComponent::SelectEffectAction);
			ActionWidget->OnEffectButtonPressed.AddDynamic(TileSelectComp, &UTileSelectComponent::SelectEffectAction);
		}
	}

	ActionMenuWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 110.f));
	ActionMenuWidgetComp->SetVisibility(true);
	SetCardSelected(true);
}

void AUnitBase::HideActionMenu()
{
	if (ActionMenuWidgetComp)
	{
		ActionMenuWidgetComp->SetVisibility(false);
	}
	SetCardSelected(false);
}

void AUnitBase::UpdateActionMenuFacingCamera()
{
	if (ActionMenuWidgetComp && ActionMenuWidgetComp->GetWidgetSpace() == EWidgetSpace::World && CardVisualRoot)
	{
		ActionMenuWidgetComp->SetWorldRotation(CardVisualRoot->GetComponentRotation());
	}
}
