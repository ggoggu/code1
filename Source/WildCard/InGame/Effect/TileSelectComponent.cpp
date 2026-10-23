// Fill out your copyright notice in the Description page of Project Settings.

#include "InGame/Effect/TileSelectComponent.h"
#include "Tile.h"
#include "BattleData.h"
#include "UnitBase.h"
#include "InGame/Unit/PlayerUnit.h"
#include "InGame/Unit/TacticalSkillBase.h"
#include "InGame/Game/InGameController.h"
#include "InGame/Game/InGamePlayerState.h"
#include "InGame/Game/InGameState.h"

UTileSelectComponent::UTileSelectComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	CurrentState = ETileInteractionState::Idle;
	SelectedTile = nullptr;
	SelectedUnit = nullptr;
}

void UTileSelectComponent::BeginPlay()
{
	Super::BeginPlay();
}

APlayerUnit* UTileSelectComponent::GetSelectedPlayerUnit() const
{
	return Cast<APlayerUnit>(SelectedUnit);
}

void UTileSelectComponent::SelectAttackAction()
{
	CurrentState = ETileInteractionState::AwaitingAttackTarget;
	OnActionMenuClosed.Broadcast();
	OnCombatableTilesFound.Broadcast(SelectedTile);
}

void UTileSelectComponent::SelectEffectAction()
{
	UE_LOG(LogTemp, Warning, TEXT("Select Effect Action"));

	AInGameState* GS = GetWorld() ? GetWorld()->GetGameState<AInGameState>() : nullptr;
	if (GS && GS->GetCurrentPhase() == EWildCardPhase::Action_Private)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TileSelect] Effect는 서브 행동이므로 Action_Private 페이즈에서 선택할 수 없습니다."));
		ClearSelect();
		OnActionMenuClosed.Broadcast();
		return;
	}

    const FCardData& CardData = SelectedUnit->GetCardData();

	UE_LOG(LogTemp, Warning, TEXT("Effect Type: %s"), *UEnum::GetValueAsString(CardData.EffectData.Type));
    // 대상 선택이 필요 없는 효과
    if (CardData.EffectData.Type == EEffectType::DRAW)
    {
		UE_LOG(LogTemp, Warning, TEXT("Trigger Draw Effect!"));
        ExecuteEffect(nullptr); // 서버에 Action 제출
        ClearSelect();
        OnActionMenuClosed.Broadcast();
        return;
    }

    // 대상 선택이 필요한 효과
    CurrentState = ETileInteractionState::AWaitingEffectTarget;
    OnActionMenuClosed.Broadcast();
    OnEffectableTilesFound.Broadcast(SelectedTile);
}

void UTileSelectComponent::SelectMoveAction()
{
	UE_LOG(LogTemp, Warning, TEXT("[TileSelect] Select Move Action"));
	CurrentState = ETileInteractionState::AwaitingMoveTarget;
	OnActionMenuClosed.Broadcast();
	OnMoveableTilesFound.Broadcast(SelectedTile);
}

void UTileSelectComponent::SelectTacticalSkillAction()
{
	APlayerUnit* PlayerUnit = GetSelectedPlayerUnit();
	if (!PlayerUnit)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TileSelect] ���õ� ������ PlayerUnit�� �ƴմϴ�."));
		CancelAction();
		return;
	}

	UTacticalSkillBase* Skill = PlayerUnit->GetTacticalSkill();
	if (!Skill)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TileSelect] PlayerUnit�� ���� ��ų�� �����Ǿ� ���� �ʽ��ϴ�."));
		CancelAction();
		return;
	}

	// �ڰ� ���� ��ų�� ��� ��� Ÿ�� ���� ���� ��� ü�� ����
	if (Skill->TargetType == ETacticalSkillTargetType::Self)
	{
		UE_LOG(LogTemp, Log, TEXT("[TileSelect] �ڰ� ���� ���� ��ų ��� ü�� ����"));
		ExecuteTacticalSkill(nullptr);
		ClearSelect();
		OnActionMenuClosed.Broadcast();
		return;
	}

	// Ÿ������ �ʿ��� ��ų�� ��� Ÿ�� Ÿ�� ��� ���·� ��ȯ
	UE_LOG(LogTemp, Log, TEXT("[TileSelect] ��� Ÿ���� ���� ��ų - Ÿ�� ���� ���"));
	CurrentState = ETileInteractionState::AwaitingTacticalSkillTarget;
	OnActionMenuClosed.Broadcast();
	OnTacticalSkillTilesFound.Broadcast(SelectedTile);
}

void UTileSelectComponent::CancelAction()
{
	if (CurrentState == ETileInteractionState::AwaitingMoveTarget)
	{
		OnMoveableTilesCancled.Broadcast(SelectedTile);
	}
	else if (CurrentState == ETileInteractionState::AwaitingAttackTarget)
	{
		OnCombatableTilesCancled.Broadcast(SelectedTile);
	}
	else if (CurrentState == ETileInteractionState::AWaitingEffectTarget)
	{
		OnEffectableTilesCancled.Broadcast(SelectedTile);
	}
	else if (CurrentState == ETileInteractionState::AwaitingTacticalSkillTarget)
	{
		OnTacticalSkillTilesCancled.Broadcast(SelectedTile);
	}

	ClearSelect();
	OnActionMenuClosed.Broadcast();
}

void UTileSelectComponent::ClearSelect()
{
	if (SelectedUnit)
	{
		SelectedUnit->HideActionMenu();
	}

	SelectedTile = nullptr;
	SelectedUnit = nullptr;
	CurrentState = ETileInteractionState::Idle;
}

void UTileSelectComponent::ProcessTileClick(ATile* ClickedTile)
{
	if (!IsValid(ClickedTile))
		return;

	UE_LOG(LogTemp, Warning, TEXT("[TileSelect] Clicked Tile: %s, Current State: %s"), 
		*ClickedTile->GetName(), *UEnum::GetValueAsString(CurrentState));

	switch (CurrentState)
	{
	case ETileInteractionState::Idle:
	{
		if (ClickedTile->HasUnit())
		{
			// ���� ���� �������� ����
			AInGameController* PC = Cast<AInGameController>(GetOwner());
			if (PC && PC->GetPlayerState<AInGamePlayerState>())
			{
				if (ClickedTile->GetUnit()->GetOwnerId() != PC->GetPlayerState<AInGamePlayerState>()->GetPlayerId())
				{
					UE_LOG(LogTemp, Warning, TEXT("[TileSelect] ���� ���� ������ �ƴմϴ�."));
					return;
				}
			}

			SelectedTile = ClickedTile;
			SelectedUnit = ClickedTile->GetUnit();

			CurrentState = ETileInteractionState::ActionMenuOpen;

			// PlayerUnit ���� ��ε�ĳ��Ʈ
			if (APlayerUnit* PlayerUnit = Cast<APlayerUnit>(SelectedUnit))
			{
				OnPlayerUnitActionMenuRequested.Broadcast(PlayerUnit);
			}

			OnActionMenuRequested.Broadcast();
		}
		break;
	}

	case ETileInteractionState::ActionMenuOpen:
	{
		if (ClickedTile == SelectedTile)
		{
			CancelAction();
		}
		else
		{
			CancelAction();
			if (ClickedTile->HasUnit())
			{
				ProcessTileClick(ClickedTile);
			}
		}
		break;
	}

	case ETileInteractionState::AwaitingMoveTarget:
	{
		if (!(ClickedTile->HasUnit()) && (ClickedTile->GetTileState() == ETileState::Moveable))
		{
			UE_LOG(LogTemp, Warning, TEXT("[TileSelect] �̵� Ÿ�� ���� �Ϸ�"));
			ExecuteMove(ClickedTile);
			OnMoveableTilesCancled.Broadcast(SelectedTile);
			ClearSelect();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[TileSelect] �̵� �Ұ� Ÿ��. ����մϴ�."));
			OnMoveableTilesCancled.Broadcast(SelectedTile);
			ClearSelect();
			if (ClickedTile->HasUnit())
			{
				ProcessTileClick(ClickedTile);
			}
		}
		break;
	}

	case ETileInteractionState::AwaitingAttackTarget:
	{
		if ((ClickedTile->HasUnit()) && (ClickedTile != SelectedTile) && (ClickedTile->GetTileState() == ETileState::None))
		{
			UE_LOG(LogTemp, Warning, TEXT("[TileSelect] ���� ��� Ÿ�� ���� �Ϸ�"));
			ExecuteCombat(SelectedUnit, ClickedTile->GetUnit());
			OnCombatableTilesCancled.Broadcast(SelectedTile);
			ClearSelect();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[TileSelect] ���� �Ұ� ���. ����մϴ�."));
			OnCombatableTilesCancled.Broadcast(SelectedTile);
			ClearSelect();
			if (ClickedTile->HasUnit())
			{
				ProcessTileClick(ClickedTile);
			}
		}
		break;
	}

	case ETileInteractionState::AwaitingTacticalSkillTarget:
	{
		APlayerUnit* PlayerUnit = GetSelectedPlayerUnit();
		if (PlayerUnit && PlayerUnit->CanUseTacticalSkill(ClickedTile))
		{
			UE_LOG(LogTemp, Log, TEXT("[TileSelect] 전술 스킬 대상 타일 선택 완료"));
			ExecuteTacticalSkill(ClickedTile);
			OnTacticalSkillTilesCancled.Broadcast(SelectedTile);
			ClearSelect();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[TileSelect] 유효하지 않은 전술 스킬 대상입니다. 취소합니다."));
			OnTacticalSkillTilesCancled.Broadcast(SelectedTile);
			ClearSelect();
			if (ClickedTile->HasUnit())
			{
				ProcessTileClick(ClickedTile);
			}
		}
		break;
	}

	case ETileInteractionState::AWaitingEffectTarget:
	{
		ExecuteEffect(ClickedTile);
		OnEffectableTilesCancled.Broadcast(SelectedTile);
		ClearSelect();
		break;
	}
	}
}

void UTileSelectComponent::ExecuteMove(ATile* TargetTile)
{
	AInGameController* PC = Cast<AInGameController>(GetOwner());
	if (!PC) return;

	FWildActionData ActionData;
	ActionData.PlayerID = PC->GetPlayerState<AInGamePlayerState>()->GetPlayerId();
	ActionData.ActionCategory = EWildActionCategory::Main;
	ActionData.ActionType = EWildActionType::UnitMove;
	ActionData.SourceTile = SelectedUnit ? SelectedUnit->GetCurrentTile() : SelectedTile.Get();
	ActionData.TargetTile = TargetTile;

	PC->CompletePrivateAction(ActionData);
}

void UTileSelectComponent::ExecuteCombat(AUnitBase* Attacker, AUnitBase* Defender)
{
	AInGameController* PC = Cast<AInGameController>(GetOwner());
	if (!PC) return;

	FWildActionData ActionData;
	ActionData.PlayerID = PC->GetPlayerState<AInGamePlayerState>()->GetPlayerId();
	ActionData.ActionCategory = EWildActionCategory::Main;
	ActionData.ActionType = EWildActionType::UnitAttack;
	ActionData.SourceTile = Attacker ? Attacker->GetCurrentTile() : SelectedTile.Get();
	ActionData.TargetTile = Defender ? Defender->GetCurrentTile() : nullptr;

	PC->CompletePrivateAction(ActionData);
}

void UTileSelectComponent::ExecuteTacticalSkill(ATile* TargetTile)
{
	AActor* OwnerActor = GetOwner();
	AInGameController* PC = Cast<AInGameController>(OwnerActor);

	FWildActionData ActionData;
	ActionData.PlayerID = PC->GetPlayerState<AInGamePlayerState>()->GetPlayerId();
	ActionData.ActionCategory = EWildActionCategory::Sub;
	ActionData.ActionType = EWildActionType::CastTacticalSkill;
	ActionData.SourceTile = SelectedUnit ? SelectedUnit->GetCurrentTile() : SelectedTile.Get();
	ActionData.TargetTile = TargetTile;
	

	UE_LOG(LogTemp, Log, TEXT("[TileSelect] 전술 스킬 SubAction 체인 제출 (PlayerID: %d)"), ActionData.PlayerID);
	PC->PlayChainCard(ActionData);
}

void UTileSelectComponent::ExecuteEffect(ATile* TargetTile)
{
	AInGameController* PC = Cast<AInGameController>(GetOwner());
	if (!PC) return;
    //WILD_SIMPLE(Warning);
    

    FWildActionData ActionData;
    ActionData.PlayerID = PC->GetPlayerState<AInGamePlayerState>()->GetPlayerId();
    ActionData.ActionCategory = EWildActionCategory::Sub;
    ActionData.ActionType = EWildActionType::TriggerTrap;

	ActionData.SourceTile = SelectedTile;
	ActionData.TargetTile = TargetTile;

	UE_LOG(LogTemp, Log, TEXT("[TileSelect] Effect SubAction 체인 제출 (PlayerID: %d)"), ActionData.PlayerID);
	PC->PlayChainCard(ActionData);
}

void UTileSelectComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
