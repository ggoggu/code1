// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Battle/PlayerStatComponent.h"
#include "Net/UnrealNetwork.h"
#include "InGame/Game/InGameMode.h"

// Sets default values for this component's properties
UPlayerStatComponent::UPlayerStatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	Health = 100;
	Mana = 1;
	PlayerLevel = 1;
	MaxForce = 11;
	CurrentForce = 0;
}

void UPlayerStatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPlayerStatComponent, Health);
	DOREPLIFETIME(UPlayerStatComponent, Mana);
	DOREPLIFETIME(UPlayerStatComponent, PlayerLevel);
	DOREPLIFETIME(UPlayerStatComponent, MaxForce);
	DOREPLIFETIME(UPlayerStatComponent, CurrentForce);
}

void UPlayerStatComponent::TakeDamage(int32 DamageAmount)
{
	if (!GetOwner()->HasAuthority() || DamageAmount <= 0) return;

	Health = FMath::Max(0, Health - DamageAmount);
	OnHealthChanged.Broadcast(Health); // 서버 측 갱신

	if (Health <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] 플레이어의 생명력이 0이 되었습니다. 패배 처리 필요."));
		// 차후 패배 이벤트 브로드캐스트나 GameState 알림 추가
	}
}

void UPlayerStatComponent::Heal(int32 HealAmount)
{
	if (!GetOwner()->HasAuthority() || HealAmount <= 0) return;

	Health += HealAmount; // 최대 체력 제한이 있다면 FMath::Min(MaxHealth, Health + HealAmount) 사용
	OnHealthChanged.Broadcast(Health);
}

bool UPlayerStatComponent::ConsumeMana(int32 Amount)
{
	if (!GetOwner()->HasAuthority() || Amount < 0) return false;

	if (Mana >= Amount)
	{
		Mana -= Amount;
		OnManaChanged.Broadcast(Mana);
		return true;
	}
	return false;
}

void UPlayerStatComponent::AddMana(int32 Amount)
{
	if (!GetOwner()->HasAuthority() || Amount <= 0) return;

	Mana += Amount;
	OnManaChanged.Broadcast(Mana);
}

void UPlayerStatComponent::SetMana(int32 NewMana)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	Mana = FMath::Max(0, NewMana);
	OnManaChanged.Broadcast(Mana);
}

bool UPlayerStatComponent::ConsumeForce(int32 Amount)
{
	if (!GetOwner()->HasAuthority() || Amount < 0) return false;

	if (CurrentForce >= Amount)
	{
		CurrentForce -= Amount;
		OnForceChanged.Broadcast(CurrentForce, MaxForce);
		return true;
	}
	return false;
}

void UPlayerStatComponent::AddForce(int32 Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Amount <= 0) return;

	CurrentForce = FMath::Min(MaxForce, CurrentForce + Amount);
	OnForceChanged.Broadcast(CurrentForce, MaxForce);
}

void UPlayerStatComponent::RefreshForce()
{
	if (!GetOwner()->HasAuthority()) return;

	// 기획서: 상한은 10 + 플레이어 레벨 [cite: 21]
	MaxForce = 10 + PlayerLevel;
	// 기획서: 포스를 전부 충전 [cite: 21]
	CurrentForce = MaxForce;

	OnForceChanged.Broadcast(CurrentForce, MaxForce);
}

void UPlayerStatComponent::AddLevel(int32 Amount)
{
	if (!GetOwner()->HasAuthority() || Amount <= 0) return;

	PlayerLevel += Amount;
	OnLevelChanged.Broadcast(PlayerLevel);
}


// Called when the game starts
void UPlayerStatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		bool bEnableDevMana = bDevStartWithTenMana;
		int32 TargetStartingMana = DevStartingMana;

		if (AInGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AInGameMode>() : nullptr)
		{
			bEnableDevMana = GameMode->bDevStartWithTenMana;
			TargetStartingMana = GameMode->DevStartingMana;
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("DevMana10")))
		{
			bEnableDevMana = true;
			TargetStartingMana = 10;
		}
		else if (FParse::Param(FCommandLine::Get(), TEXT("NormalMana")))
		{
			bEnableDevMana = false;
		}

		if (bEnableDevMana)
		{
			SetMana(TargetStartingMana);
		}
	}
}

void UPlayerStatComponent::OnRep_Health()
{
	OnHealthChanged.Broadcast(Health);
}

void UPlayerStatComponent::OnRep_Mana()
{
	OnManaChanged.Broadcast(Mana);
}

void UPlayerStatComponent::OnRep_Level()
{
	OnLevelChanged.Broadcast(PlayerLevel);
}

void UPlayerStatComponent::OnRep_MaxForce()
{
	OnForceChanged.Broadcast(CurrentForce, MaxForce);
}

void UPlayerStatComponent::OnRep_CurrentForce()
{
	OnForceChanged.Broadcast(CurrentForce, MaxForce);
}