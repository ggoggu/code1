// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InGame/Unit/UnitBase.h"
#include "InGame/Data/BattleData.h"
#include "PlayerUnit.generated.h"

class AInGamePlayerState;
class UHeroAbilityBase;
class UTacticalSkillBase;
class ATile;
class USoundBase;
class UParticleSystem;
class USceneComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class USelectActionWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTacticalSkillChanged, UTacticalSkillBase*, NewSkill);

UCLASS()
class WILDCARD_API APlayerUnit : public AUnitBase
{
	GENERATED_BODY()

public:
	APlayerUnit();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 초기화 함수 (서버 권한에서 호출)
	void InitializePlayerUnit(AInGamePlayerState* InPlayerState, EWildPlayerClass InClass, TSubclassOf<UHeroAbilityBase> InAbilityClass = nullptr, TSubclassOf<UTacticalSkillBase> InTacticalSkillClass = nullptr);

	// AUnitBase 가상 함수 오버라이드
	virtual int32 GetCardHealth() override;
	virtual void SetCardHealth(int32 InHealth) override;
	virtual void HandleDeath() override;

	// 영웅 능력 사용 가능 여부 확인
	UFUNCTION(BlueprintCallable, Category = "Hero Ability")
	bool CanUseHeroAbility(ATile* TargetTile = nullptr) const;

	// 영웅 능력 실행 (클라이언트/서버 공용 진입점)
	UFUNCTION(BlueprintCallable, Category = "Hero Ability")
	bool UseHeroAbility(ATile* TargetTile = nullptr);

	// 영웅 능력 서버 RPC
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Hero Ability")
	void Server_UseHeroAbility(ATile* TargetTile);

	UFUNCTION(BlueprintCallable, Category = "PlayerUnit")
	EWildPlayerClass GetPlayerClass() const { return PlayerClass; }

	UFUNCTION(BlueprintCallable, Category = "PlayerUnit")
	void SetPlayerClass(EWildPlayerClass InClass);

	UFUNCTION(BlueprintCallable, Category = "PlayerUnit")
	AInGamePlayerState* GetOwningPlayerState() const;

	UFUNCTION(BlueprintCallable, Category = "Hero Ability")
	UHeroAbilityBase* GetHeroAbility() const { return HeroAbility; }

	UFUNCTION(BlueprintCallable, Category = "Hero Ability")
	bool IsHeroAbilityUsedThisTurn() const { return bHeroAbilityUsedThisTurn; }

	// =========================================================================
	// 전술 스킬 (Tactical Skill) - 체인(SubAction) 전용 및 교체 가능한 전략 패턴
	// =========================================================================
	UFUNCTION(BlueprintCallable, Category = "PlayerUnit|TacticalSkill")
	UTacticalSkillBase* GetTacticalSkill() const { return TacticalSkillInstance; }

	UFUNCTION(BlueprintCallable, Category = "PlayerUnit|TacticalSkill")
	bool CanUseTacticalSkill(ATile* TargetTile = nullptr, bool bCheckMana = true) const;

	bool CanUseTacticalSkillAction(const FWildActionData& ActionData, bool bCheckMana = true) const;

	UFUNCTION(BlueprintCallable, Category = "PlayerUnit|TacticalSkill")
	bool ExecuteTacticalSkill(ATile* TargetTile = nullptr, bool bConsumeMana = false);

	bool ExecuteTacticalSkillAction(const FWildActionData& ActionData, bool bConsumeMana = false);

	UFUNCTION(BlueprintCallable, Category = "PlayerUnit|TacticalSkill")
	bool IsTacticalSkillUsedThisTurn() const { return bTacticalSkillUsedThisTurn; }

	UFUNCTION(BlueprintCallable, Category = "PlayerUnit|TacticalSkill")
	void SetTacticalSkill(TSubclassOf<class UTacticalSkillBase> NewSkillClass);

	void ResetTacticalSkillTurnUsage();

	// =========================================================================
	// 컴포넌트 (Mesh, Root & 머리 위 액션 메뉴 UI)
	// =========================================================================
	// RootScene, CardVisualRoot, CardMeshComp는 AUnitBase로부터 상속받아 3D 카드 메시로 작동합니다.

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PlayerUnit|Components", meta = (DeprecatedProperty, DeprecationMessage = "Use CardMeshComp from UnitBase instead"))
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PlayerUnit|Components", meta = (DeprecatedProperty, DeprecationMessage = "Use CardMeshComp from UnitBase instead"))
	TObjectPtr<UStaticMeshComponent> StaticMeshComp;

	// 머리 위에 표시되는 액션 메뉴 UI 오버라이드
	virtual void ShowActionMenu(EWildCardPhase CurrentPhase, bool bHasPriority, class UTileSelectComponent* TileSelectComp) override;
	virtual void HideActionMenu() override;

	// =========================================================================
	// 영웅 카드 일러스트레이션 (PlayerClass별)
	// =========================================================================
	virtual UTexture2D* GetCardIllustrationTexture() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerUnit|Visual")
	TMap<EWildPlayerClass, TSoftObjectPtr<UTexture2D>> PlayerClassPortraits;

	// =========================================================================
	// 사운드 에셋 (에디터 BP에서 커스터마이징 가능)
	// =========================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerUnit|Audio")
	TObjectPtr<USoundBase> SpawnSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerUnit|Audio")
	TObjectPtr<USoundBase> DamagedSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerUnit|Audio")
	TObjectPtr<USoundBase> AttackSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerUnit|Audio")
	TObjectPtr<USoundBase> DeathSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerUnit|Audio")
	TObjectPtr<USoundBase> AbilitySound;

	// =========================================================================
	// VFX 에셋 (에디터 BP에서 커스터마이징 가능)
	// =========================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerUnit|VFX")
	TObjectPtr<UParticleSystem> SpawnVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlayerUnit|VFX")
	TObjectPtr<UParticleSystem> AbilityVFX;

	// =========================================================================
	// 네트워크 멀티캐스트 사운드 및 VFX 재생
	// =========================================================================
	UFUNCTION(NetMulticast, Unreliable, BlueprintCallable, Category = "PlayerUnit|Audio")
	void Multicast_PlayUnitSound(USoundBase* InSound);

	UFUNCTION(NetMulticast, Unreliable, BlueprintCallable, Category = "PlayerUnit|VFX")
	void Multicast_PlayUnitVFX(UParticleSystem* InVFX);

	// =========================================================================
	// 블루프린트 커스텀 이벤트
	// =========================================================================
	UFUNCTION(BlueprintImplementableEvent, Category = "PlayerUnit|Events")
	void OnPlayerUnitSpawned();

	UFUNCTION(BlueprintImplementableEvent, Category = "PlayerUnit|Events")
	void OnPlayerUnitDamaged(int32 DamageAmount, int32 RemainingHealth);

	UFUNCTION(BlueprintImplementableEvent, Category = "PlayerUnit|Events")
	void OnPlayerUnitDeath();

	UFUNCTION(BlueprintImplementableEvent, Category = "PlayerUnit|Events")
	void OnHeroAbilityTriggered();

	UPROPERTY(BlueprintAssignable, Category = "PlayerUnit|TacticalSkill")
	FOnTacticalSkillChanged OnTacticalSkillChangedDelegate;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_PlayerClass, VisibleAnywhere, BlueprintReadOnly, Category = "PlayerUnit")
	EWildPlayerClass PlayerClass;

	UPROPERTY(ReplicatedUsing = OnRep_OwningPlayerState, VisibleAnywhere, BlueprintReadOnly, Category = "PlayerUnit")
	TWeakObjectPtr<AInGamePlayerState> OwningPlayerState;

	UPROPERTY(ReplicatedUsing = OnRep_HeroAbilityUsed, BlueprintReadOnly, Category = "Hero Ability")
	bool bHeroAbilityUsedThisTurn;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hero Ability")
	TObjectPtr<UHeroAbilityBase> HeroAbility;

	UPROPERTY(EditDefaultsOnly, Category = "Hero Ability")
	TSubclassOf<UHeroAbilityBase> CustomHeroAbilityClass;

	// --- Tactical Skill (전술 스킬) ---
	UPROPERTY(ReplicatedUsing = OnRep_TacticalSkillClass, EditDefaultsOnly, Category = "PlayerUnit|TacticalSkill")
	TSubclassOf<UTacticalSkillBase> TacticalSkillClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PlayerUnit|TacticalSkill")
	TObjectPtr<UTacticalSkillBase> TacticalSkillInstance;

	UPROPERTY(ReplicatedUsing = OnRep_TacticalSkillUsed, BlueprintReadOnly, Category = "PlayerUnit|TacticalSkill")
	bool bTacticalSkillUsedThisTurn;

	UFUNCTION()
	void OnRep_PlayerClass();

	UFUNCTION()
	void OnRep_OwningPlayerState();

	UFUNCTION()
	void OnRep_HeroAbilityUsed();

	UFUNCTION()
	void OnRep_TacticalSkillClass();

	UFUNCTION()
	void OnRep_TacticalSkillUsed();

	UFUNCTION()
	void OnPlayerStateHealthChanged(int32 NewHealth);

	UFUNCTION()
	void OnTurnStarted();

	void CreateHeroAbility(TSubclassOf<UHeroAbilityBase> InAbilityClass = nullptr);
	void CreateTacticalSkill(TSubclassOf<UTacticalSkillBase> InSkillClass = nullptr);
	void BindToPlayerState();
};
