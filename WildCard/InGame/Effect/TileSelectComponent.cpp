// Fill out your copyright notice in the Description page of Project Settings.


#include "InGame/Effect/TileSelectComponent.h"
#include "Tile.h"
#include "BattleData.h"
#include "UnitBase.h"
#include "InGame/Game/InGameController.h"
#include "InGame/Game/InGamePlayerState.h"

UTileSelectComponent::UTileSelectComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = true;

    // 생성 시 변수 초기화
    CurrentState = ETileInteractionState::Idle;
    SelectedTile = nullptr;
    SelectedUnit = nullptr;
}

void UTileSelectComponent::SelectAttackAction()
{
    CurrentState = ETileInteractionState::AwaitingAttackTarget;
    OnActionMenuClosed.Broadcast();
    OnCombatableTilesFound.Broadcast(SelectedTile);
}

void UTileSelectComponent::SelectMoveAction()
{
    CurrentState = ETileInteractionState::AwaitingMoveTarget;
    OnActionMenuClosed.Broadcast();
    OnMoveableTilesFound.Broadcast(SelectedTile);
}

void UTileSelectComponent::CancelAction()
{
    OnActionMenuClosed.Broadcast();
    ClearSelect();
}

// Called when the game starts
void UTileSelectComponent::BeginPlay()
{
    Super::BeginPlay();
    // ...

}

void UTileSelectComponent::ClearSelect()
{
    SelectedTile = nullptr;
    SelectedUnit = nullptr;
    CurrentState = ETileInteractionState::Idle;
}

void UTileSelectComponent::ProcessTileClick(ATile* ClickedTile)
{
    // 클릭된 타일이 유효한지 검증
    if(!IsValid(ClickedTile))
		return;

    UE_LOG(LogTemp, Warning, TEXT("%s"), *ClickedTile->GetName());

    FString State = UEnum::GetValueAsString(CurrentState);
    UE_LOG(LogTemp, Warning, TEXT("Current State: %s"), *State);

    switch (CurrentState)
    {

    case ETileInteractionState::Idle:
    {
        // 타일에 유닛이 있는지 확인
        if (ClickedTile->HasUnit())
        {
            SelectedTile = ClickedTile;
            SelectedUnit = ClickedTile->GetUnit();

            UE_LOG(LogTemp, Warning, TEXT("SELECTED TILE : %d / %d"), ClickedTile->Axial.Q, ClickedTile->Axial.R);

            // 상태를 '유닛 선택됨'으로 전환
            CurrentState = ETileInteractionState::ActionMenuOpen;

            //UE_LOG(LogTemp,  Warning, TEXT("유닛 선택! 이동할 타일이나 공격할 적을 선택하세요."));
            UE_LOG(LogTemp, Warning, TEXT("Success! Choose a tile to move or attack."));

            OnActionMenuRequested.Broadcast();
        }
        break;
    }

    case ETileInteractionState::ActionMenuOpen:
    {
        //#. 다른 빈 타일 클릭시 메뉴 닫기
        if (ClickedTile != SelectedTile)
        {
            CancelAction();
        }
        break;
    }

    case ETileInteractionState::AwaitingMoveTarget:
    {
        if (!(ClickedTile->HasUnit()) && (ClickedTile->GetTileState() == ETileState::Moveable))
        {
            UE_LOG(LogTemp, Warning, TEXT("해당 타일로 이동합니다."));
            ExecuteMove(ClickedTile);

            OnMoveableTilesCancled.Broadcast(SelectedTile);

            ClearSelect();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Can't Move. Change target tile."));
            OnMoveableTilesCancled.Broadcast(SelectedTile);

            ClearSelect();
        }
        break;
    }

    case ETileInteractionState::AwaitingAttackTarget:
    {
        // 공격 가능한 타일인지 검증 (예시에서는 생략)
        if ((ClickedTile->HasUnit()) && (ClickedTile != SelectedTile) && (ClickedTile->GetTileState() == ETileState::None))
        {
            UE_LOG(LogTemp, Warning, TEXT("전투 개시!"));
            ExecuteCombat(SelectedUnit, ClickedTile->GetUnit());

            OnCombatableTilesCancled.Broadcast(SelectedTile);

            ClearSelect();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("공격할 유닛이 없습니다. 행동을 취소합니다."));
            OnCombatableTilesCancled.Broadcast(SelectedTile);

            ClearSelect();
        }
        break;
    }
    }
}

void UTileSelectComponent::ExecuteMove(ATile* TargetTile)
{
    //TODO : ActionData 전달로 바꾸기 (Gamestate -> ChainComponent -> AddActionToChain)
    //TODO : ActionData에 Tile pointer 추가해보기


    AActor* OwnerActor = GetOwner();
    AInGameController* PC = Cast<AInGameController>(OwnerActor);

    FWildActionData ActionData;
    ActionData.PlayerID = PC->GetPlayerState<AInGamePlayerState>()->PlayerCount;
    ActionData.ActionType = EWildActionType::UnitMove;
    //ActionData.TESTSourceUnit = SelectedUnit;
    ActionData.TargetTile = TargetTile;
    ActionData.CardInstanceID = 0;

   //PC->SendActionToServer(ActionData);
}

void UTileSelectComponent::ExecuteCombat(AUnitBase* Attacker, AUnitBase* Defender)
{
    AActor* OwnerActor = GetOwner();
    AInGameController* PC = Cast<AInGameController>(OwnerActor);

    FWildActionData ActionData;
    ActionData.PlayerID = PC->GetPlayerState<AInGamePlayerState>()->PlayerCount;
    ActionData.ActionType = EWildActionType::UnitAttack;
    //ActionData.TESTSourceUnit = Attacker;
    //ActionData.TESTTargetUnit = Defender;
    ActionData.CardInstanceID = 0;

    //PC->SendActionToServer(ActionData);
}

void UTileSelectComponent::ExecuteEffect()
{
    //WILD_SIMPLE(Warning);
    AActor* OwnerActor = GetOwner();
    APlayerController* PC = Cast<APlayerController>(OwnerActor);
    //ADefaultPawnRestricted* MyPawn = Cast<ADefaultPawnRestricted>(PC->GetPawn());

    //  if (MyPawn)
    //  {
          //MyPawn->OnEffectTestPressed();
    //  }


    //  //TEST//
    //TArray<UWildProtoEffectBase*> HandleEffect = SelectedUnit->CardEffects;
    //for (UWildProtoEffectBase* Effect : HandleEffect)
    //{
    //    if (Effect == nullptr)
    //        return;

    //    FWildActionData ActionData = FWildActionData();
    //    ActionData.PlayerID = PC->GetPlayerState<AWildProtoPlayerState>()->PlayerCount;
    //    ActionData.Value = 5;
    //    ActionData.TESTSourceUnit = SelectedUnit;
    //    ActionData.ActionType = EWildActionType::CastSpell;

    //    MyPawn->Server_TestEffect(Effect, ActionData);
    //}
    ////TEST//
}

// Called every frame
void UTileSelectComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // ...
}

