// Fill out your copyright notice in the Description page of Project Settings.

#include "InGame/Unit/PlayerUnit.h"
#include "WildCard.h"
#include "Net/UnrealNetwork.h"
#include "InGame/Unit/HeroAbilityBase.h"
#include "InGame/Unit/TacticalSkillBase.h"
#include "InGame/Map/Tile.h"
#include "InGame/Game/InGamePlayerState.h"
#include "InGame/Battle/PlayerStatComponent.h"
#include "InGame/Game/InGameState.h"
#include "InGame/Game/InGameMode.h"
#include "InGame/Game/InGameController.h"
#include "InGame/UI/Widget/SelectActionWidget.h"
#include "InGame/Effect/TileSelectComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Particles/ParticleSystem.h"

APlayerUnit::APlayerUnit()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	PlayerClass = EWildPlayerClass::Common;
	bHeroAbilityUsedThisTurn = false;
	HeroAbility = nullptr;
	TacticalSkillClass = nullptr;
	TacticalSkillInstance = nullptr;
	bTacticalSkillUsedThisTurn = false;
	bEnableFacingCamera = true;
	BillboardMode = EUnitBillboardMode::FullSpherical;

	// RootScene, CardVisualRoot, CardMeshComp는 Super(AUnitBase)에서 생성됨

	SkeletalMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComp"));
	SkeletalMeshComp->SetupAttachment(RootComponent);
	SkeletalMeshComp->SetCollisionResponseToChannel(ECC_MouseClick, ECR_Ignore);
	SkeletalMeshComp->SetVisibility(false);

	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComp"));
	StaticMeshComp->SetupAttachment(RootComponent);
	StaticMeshComp->SetCollisionResponseToChannel(ECC_MouseClick, ECR_Ignore);
	StaticMeshComp->SetVisibility(false);

	// 기본 클래스별 영웅 카드 일러스트/프레임 등록
	PlayerClassPortraits.Add(EWildPlayerClass::Warrior, TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/InGame/UI/Texture/CardFrame/Frame_Warrior.Frame_Warrior"))));
	PlayerClassPortraits.Add(EWildPlayerClass::Mage, TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/InGame/UI/Texture/CardFrame/Frame_Wizard.Frame_Wizard"))));
	PlayerClassPortraits.Add(EWildPlayerClass::Taoist, TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/InGame/UI/Texture/CardFrame/Frame.Frame"))));
	PlayerClassPortraits.Add(EWildPlayerClass::Common, TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/InGame/UI/Texture/CardFrame/Frame_Common_ver2.Frame_Common_ver2"))));

	// PlayerUnit 머리 위 높이로 오프셋 설정 (컴포넌트는 Base 클래스에서 생성)
	if (ActionMenuWidgetComp)
	{
		ActionMenuWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 140.f));
	}
}

void APlayerUnit::BeginPlay()
{
	Super::BeginPlay();

	if (ActionMenuWidgetComp)
	{
		ActionMenuWidgetComp->SetVisibility(false);
		ActionMenuWidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (AInGameState* GS = GetWorld()->GetGameState<AInGameState>())
	{
		GS->OnTurnStartedDelegate.AddDynamic(this, &APlayerUnit::OnTurnStarted);
	}

	BindToPlayerState();

	if (!HeroAbility)
	{
		CreateHeroAbility(CustomHeroAbilityClass);
	}

	if (!TacticalSkillInstance)
	{
		CreateTacticalSkill(TacticalSkillClass);
	}
}

void APlayerUnit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APlayerUnit::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AInGameState* GS = GetWorld()->GetGameState<AInGameState>())
	{
		GS->OnTurnStartedDelegate.RemoveDynamic(this, &APlayerUnit::OnTurnStarted);
	}

	if (AInGamePlayerState* PS = GetOwningPlayerState())
	{
		if (UPlayerStatComponent* Stat = PS->GetStatComponent())
		{
			Stat->OnHealthChanged.RemoveDynamic(this, &APlayerUnit::OnPlayerStateHealthChanged);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void APlayerUnit::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APlayerUnit, PlayerClass);
	DOREPLIFETIME(APlayerUnit, OwningPlayerState);
	DOREPLIFETIME(APlayerUnit, bHeroAbilityUsedThisTurn);
	DOREPLIFETIME(APlayerUnit, TacticalSkillClass);
	DOREPLIFETIME(APlayerUnit, bTacticalSkillUsedThisTurn);
}

void APlayerUnit::InitializePlayerUnit(AInGamePlayerState* InPlayerState, EWildPlayerClass InClass, TSubclassOf<UHeroAbilityBase> InAbilityClass, TSubclassOf<UTacticalSkillBase> InTacticalSkillClass)
{
	if (!HasAuthority()) return;

	OwningPlayerState = InPlayerState;
	if (InClass == EWildPlayerClass::Common && InPlayerState && InPlayerState->GetPlayerClass() != EWildPlayerClass::Common)
	{
		PlayerClass = InPlayerState->GetPlayerClass();
	}
	else
	{
		PlayerClass = InClass;
	}

	if (InPlayerState)
	{
		UnitCardData.OwnerId = InPlayerState->GetPlayerId();
		if (UPlayerStatComponent* Stat = InPlayerState->GetStatComponent())
		{
			UnitCardData.Health = Stat->Health;
		}
		UnitCardData.Attack = 1;
		UnitCardData.CardName = FString::Printf(TEXT("Player %d"), UnitCardData.OwnerId);

		InPlayerState->SetPlayerUnit(this);
	}

	BindToPlayerState();
	CreateHeroAbility(InAbilityClass);
	CreateTacticalSkill(InTacticalSkillClass);

	if (AInGameState* GameState = GetWorld()->GetGameState<AInGameState>())
	{
		GameState->HandleUnitSpawned(this, CurrentTile);
	}

	Multicast_PlayUnitSound(SpawnSound);
	Multicast_PlayUnitVFX(SpawnVFX);
	OnPlayerUnitSpawned();

	SetWidgetScore();
	UpdateCardVisual();
	ForceNetUpdate();
}

int32 APlayerUnit::GetCardHealth()
{
	AInGamePlayerState* PS = GetOwningPlayerState();
	if (PS && PS->GetStatComponent())
	{
		return PS->GetStatComponent()->Health;
	}
	return UnitCardData.Health;
}

void APlayerUnit::SetCardHealth(int32 InHealth)
{
	int32 OldHealth = UnitCardData.Health;
	UnitCardData.Health = InHealth;

	if (HasAuthority())
	{
		if (AInGamePlayerState* PS = GetOwningPlayerState())
		{
			if (UPlayerStatComponent* Stat = PS->GetStatComponent())
			{
				int32 Delta = OldHealth - InHealth;
				if (Delta > 0)
				{
					Stat->TakeDamage(Delta);
					Multicast_PlayUnitSound(DamagedSound);
					OnPlayerUnitDamaged(Delta, InHealth);
					PlayCardHitReaction();
				}
				else if (Delta < 0)
				{
					Stat->Heal(-Delta);
				}
			}
		}

		if (UnitCardData.Health <= 0)
		{
			HandleDeath();
		}
	}

	SetWidgetScore();
}

void APlayerUnit::HandleDeath()
{
	if (HasAuthority())
	{
		Multicast_PlayUnitSound(DeathSound);
		OnPlayerUnitDeath();

		UE_LOG(LogTemp, Warning, TEXT("[PlayerUnit] Player %d's unit died! Triggering defeat!"), GetOwnerId());
		if (AInGameMode* GameMode = GetWorld()->GetAuthGameMode<AInGameMode>())
		{
			GameMode->HandlePlayerDefeat(GetOwnerId());
		}
	}

	Super::HandleDeath();
}

void APlayerUnit::Multicast_PlayUnitSound_Implementation(USoundBase* InSound)
{
	if (InSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, InSound, GetActorLocation());
	}
}

void APlayerUnit::Multicast_PlayUnitVFX_Implementation(UParticleSystem* InVFX)
{
	if (InVFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), InVFX, GetActorLocation(), GetActorRotation());
	}
}

bool APlayerUnit::CanUseHeroAbility(ATile* TargetTile) const
{
	if (bHeroAbilityUsedThisTurn || !HeroAbility)
	{
		return false;
	}

	return HeroAbility->CanActivate(const_cast<APlayerUnit*>(this), TargetTile);
}

bool APlayerUnit::UseHeroAbility(ATile* TargetTile)
{
	if (HasAuthority())
	{
		if (!CanUseHeroAbility(TargetTile))
		{
			UE_LOG(LogTemp, Warning, TEXT("[PlayerUnit] Cannot activate hero ability."));
			return false;
		}

		AInGamePlayerState* PS = GetOwningPlayerState();
		if (!PS || !PS->GetStatComponent())
		{
			return false;
		}

		if (!PS->GetStatComponent()->ConsumeMana(HeroAbility->ManaCost))
		{
			UE_LOG(LogTemp, Warning, TEXT("[PlayerUnit] Insufficient mana to use hero ability."));
			return false;
		}

		if (HeroAbility->Activate(this, TargetTile))
		{
			bHeroAbilityUsedThisTurn = true;
			HeroAbility->bUsedThisTurn = true;
			Multicast_PlayUnitSound(AbilitySound);
			Multicast_PlayUnitVFX(AbilityVFX);
			PlayCardActivationEffect();
			OnHeroAbilityTriggered();
			UE_LOG(LogTemp, Log, TEXT("[PlayerUnit] Player %d activated Hero Ability %s!"), GetOwnerId(), *HeroAbility->AbilityName.ToString());
			return true;
		}

		return false;
	}
	else
	{
		Server_UseHeroAbility(TargetTile);
		return true;
	}
}

bool APlayerUnit::Server_UseHeroAbility_Validate(ATile* TargetTile)
{
	return true;
}

void APlayerUnit::Server_UseHeroAbility_Implementation(ATile* TargetTile)
{
	UseHeroAbility(TargetTile);
}

AInGamePlayerState* APlayerUnit::GetOwningPlayerState() const
{
	if (OwningPlayerState.IsValid())
	{
		return OwningPlayerState.Get();
	}

	if (UWorld* World = GetWorld())
	{
		if (AInGameState* GS = World->GetGameState<AInGameState>())
		{
			for (APlayerState* PS : GS->PlayerArray)
			{
				if (AInGamePlayerState* WildPS = Cast<AInGamePlayerState>(PS))
				{
					if (WildPS->GetPlayerId() == UnitCardData.OwnerId)
					{
						return WildPS;
					}
				}
			}
		}
	}

	return nullptr;
}

void APlayerUnit::BindToPlayerState()
{
	if (AInGamePlayerState* PS = GetOwningPlayerState())
	{
		if (UPlayerStatComponent* Stat = PS->GetStatComponent())
		{
			Stat->OnHealthChanged.RemoveDynamic(this, &APlayerUnit::OnPlayerStateHealthChanged);
			Stat->OnHealthChanged.AddDynamic(this, &APlayerUnit::OnPlayerStateHealthChanged);
			UnitCardData.Health = Stat->Health;
			SetWidgetScore();
		}
	}
}

void APlayerUnit::CreateHeroAbility(TSubclassOf<UHeroAbilityBase> InAbilityClass)
{
	TSubclassOf<UHeroAbilityBase> ClassToUse = InAbilityClass;
	if (!ClassToUse)
	{
		if (CustomHeroAbilityClass)
		{
			ClassToUse = CustomHeroAbilityClass;
		}
		else
		{
			ClassToUse = UHeroAbilityBase::GetDefaultAbilityClassForPlayerClass(PlayerClass);
		}
	}

	if (ClassToUse)
	{
		HeroAbility = NewObject<UHeroAbilityBase>(this, ClassToUse);
		if (HeroAbility)
		{
			HeroAbility->bUsedThisTurn = bHeroAbilityUsedThisTurn;
		}
	}
}

void APlayerUnit::OnRep_PlayerClass()
{
	CreateHeroAbility();
	CreateTacticalSkill(TacticalSkillClass);
	UpdateCardVisual();
}

void APlayerUnit::OnRep_OwningPlayerState()
{
	BindToPlayerState();
	if (AInGamePlayerState* PS = GetOwningPlayerState())
	{
		if (PS->GetPlayerClass() != EWildPlayerClass::Common && PlayerClass != PS->GetPlayerClass())
		{
			PlayerClass = PS->GetPlayerClass();
			CreateHeroAbility();
			CreateTacticalSkill(TacticalSkillClass);
		}
	}
	UpdateCardVisual();
}

void APlayerUnit::OnRep_HeroAbilityUsed()
{
	if (HeroAbility)
	{
		HeroAbility->bUsedThisTurn = bHeroAbilityUsedThisTurn;
	}
}

void APlayerUnit::OnPlayerStateHealthChanged(int32 NewHealth)
{
	int32 OldHealth = UnitCardData.Health;
	UnitCardData.Health = NewHealth;

	if (NewHealth < OldHealth)
	{
		int32 DamageAmount = OldHealth - NewHealth;
		OnPlayerUnitDamaged(DamageAmount, NewHealth);
		PlayCardHitReaction();
	}

	SetWidgetScore();

	if (NewHealth <= 0 && HasAuthority())
	{
		HandleDeath();
	}
}

void APlayerUnit::OnTurnStarted()
{
	if (HasAuthority())
	{
		bHeroAbilityUsedThisTurn = false;
		bTacticalSkillUsedThisTurn = false;
	}

	if (HeroAbility)
	{
		HeroAbility->ResetTurnUsage();
	}

	ResetTacticalSkillTurnUsage();
}

bool APlayerUnit::CanUseTacticalSkill(ATile* TargetTile, bool bCheckMana) const
{
	FWildActionData ActionData;
	ActionData.TargetTile = TargetTile;
	return CanUseTacticalSkillAction(ActionData, bCheckMana);
}

bool APlayerUnit::CanUseTacticalSkillAction(const FWildActionData& ActionData, bool bCheckMana) const
{
	if (!TacticalSkillInstance)
	{
		return false;
	}

	if (bCheckMana && bTacticalSkillUsedThisTurn)
	{
		return false;
	}

	return TacticalSkillInstance->CanActivateAction(const_cast<APlayerUnit*>(this), ActionData, bCheckMana);
}

bool APlayerUnit::ExecuteTacticalSkill(ATile* TargetTile, bool bConsumeMana)
{
	FWildActionData ActionData;
	ActionData.TargetTile = TargetTile;
	return ExecuteTacticalSkillAction(ActionData, bConsumeMana);
}

bool APlayerUnit::ExecuteTacticalSkillAction(const FWildActionData& ActionData, bool bConsumeMana)
{
	if (!CanUseTacticalSkillAction(ActionData, bConsumeMana))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerUnit] Cannot activate tactical skill."));
		return false;
	}

	AInGamePlayerState* PS = GetOwningPlayerState();
	if (!PS || !PS->GetStatComponent())
	{
		return false;
	}

	if (bConsumeMana)
	{
		if (!PS->GetStatComponent()->ConsumeMana(TacticalSkillInstance->ManaCost))
		{
			UE_LOG(LogTemp, Warning, TEXT("[PlayerUnit] Insufficient mana to use tactical skill."));
			return false;
		}
	}

	if (TacticalSkillInstance->ExecuteAction(this, ActionData))
	{
		bTacticalSkillUsedThisTurn = true;
		PlayCardActivationEffect();
		UE_LOG(LogTemp, Log, TEXT("[PlayerUnit] Player %d executed Tactical Skill: %s!"), GetOwnerId(), *TacticalSkillInstance->SkillName.ToString());
		return true;
	}

	return false;
}

void APlayerUnit::SetPlayerClass(EWildPlayerClass InClass)
{
	const bool bClassChanged = (PlayerClass != InClass);
	PlayerClass = InClass;
	if (bClassChanged || !HeroAbility)
	{
		CreateHeroAbility();
	}
	if (bClassChanged || !TacticalSkillInstance)
	{
		CreateTacticalSkill(TacticalSkillClass);
	}
	UpdateCardVisual();
	if (HasAuthority())
	{
		ForceNetUpdate();
	}
}

void APlayerUnit::SetTacticalSkill(TSubclassOf<UTacticalSkillBase> NewSkillClass)
{
	if (!HasAuthority()) return;

	TacticalSkillClass = NewSkillClass;
	CreateTacticalSkill(NewSkillClass);
	ForceNetUpdate();
}

void APlayerUnit::ResetTacticalSkillTurnUsage()
{
	bTacticalSkillUsedThisTurn = false;
}

void APlayerUnit::CreateTacticalSkill(TSubclassOf<UTacticalSkillBase> InSkillClass)
{
	TSubclassOf<UTacticalSkillBase> ClassToUse = InSkillClass;
	if (!ClassToUse)
	{
		if (TacticalSkillClass)
		{
			ClassToUse = TacticalSkillClass;
		}
		else if (OwningPlayerState.IsValid() && OwningPlayerState->GetSelectedTacticalSkillClass())
		{
			ClassToUse = OwningPlayerState->GetSelectedTacticalSkillClass();
		}
		else
		{
			ClassToUse = UTacticalSkillBase::GetDefaultSkillClassForPlayerClass(PlayerClass);
		}
	}

	if (HasAuthority())
	{
		TacticalSkillClass = ClassToUse;
	}

	if (ClassToUse)
	{
		TacticalSkillInstance = NewObject<UTacticalSkillBase>(this, ClassToUse);
		OnTacticalSkillChangedDelegate.Broadcast(TacticalSkillInstance);
	}
}

void APlayerUnit::OnRep_TacticalSkillClass()
{
	CreateTacticalSkill(TacticalSkillClass);
}

void APlayerUnit::OnRep_TacticalSkillUsed()
{
	// 클라이언트에서 턴 사용 상태 변경 시 UI 갱신 등에 활용 가능
}

void APlayerUnit::ShowActionMenu(EWildCardPhase CurrentPhase, bool bHasPriority, UTileSelectComponent* TileSelectComp)
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
		ActionWidget->SetupForPlayerUnit(this, CurrentPhase, bHasPriority);

		if (TileSelectComp)
		{
			ActionWidget->OnMoveButtonPressed.RemoveDynamic(TileSelectComp, &UTileSelectComponent::SelectMoveAction);
			ActionWidget->OnMoveButtonPressed.AddDynamic(TileSelectComp, &UTileSelectComponent::SelectMoveAction);
		}

		if (AInGameController* PC = Cast<AInGameController>(GetWorld()->GetFirstPlayerController()))
		{
			ActionWidget->OnTacticalSkillButtonPressed.RemoveDynamic(PC, &AInGameController::OnTacticalSkillActionSelected);
			ActionWidget->OnTacticalSkillButtonPressed.AddDynamic(PC, &AInGameController::OnTacticalSkillActionSelected);
		}
	}

	ActionMenuWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 140.f));
	ActionMenuWidgetComp->SetVisibility(true);
	SetCardSelected(true);
}

void APlayerUnit::HideActionMenu()
{
	Super::HideActionMenu();
}

UTexture2D* APlayerUnit::GetCardIllustrationTexture() const
{
	EWildPlayerClass EffectiveClass = PlayerClass;
	if (EffectiveClass == EWildPlayerClass::Common && OwningPlayerState.IsValid() && OwningPlayerState->GetPlayerClass() != EWildPlayerClass::Common)
	{
		EffectiveClass = OwningPlayerState->GetPlayerClass();
	}

	if (const TSoftObjectPtr<UTexture2D>* PortraitPtr = PlayerClassPortraits.Find(EffectiveClass))
	{
		if (PortraitPtr->IsValid())
		{
			return PortraitPtr->Get();
		}
		return PortraitPtr->LoadSynchronous();
	}

	return Super::GetCardIllustrationTexture();
}
