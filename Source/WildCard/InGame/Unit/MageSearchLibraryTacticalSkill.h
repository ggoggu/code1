// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InGame/Unit/TacticalSkillBase.h"
#include "MageSearchLibraryTacticalSkill.generated.h"

class AInGameController;
class APlayerUnit;
class ATile;

/**
 * 마법사 전술 스킬: 서고 탐색 (Mage Search Library)
 * 패에서 카드를 1장 선택하여 덱으로 되돌리고 1장 드로우
 * 
 * - Phase 1: 로컬 카드 선택 모달 활성화 (InitiateSkill & FWildCardSelectionContext)
 * - Phase 2: 서버 권한 검증 및 실행 (ExecuteAction & USearchLibraryEffect)
 * - Phase 3 & 4: Visual.TacticalSkill.Mage.SearchLibrary 연출 연동
 */
UCLASS(Blueprintable)
class WILDCARD_API UMageSearchLibraryTacticalSkill : public UTacticalSkillBase
{
	GENERATED_BODY()

public:
	UMageSearchLibraryTacticalSkill();

	// 클라이언트 측 전술 스킬 진입점: 카드 선택 모달(UI) 팝업 및 체인 발동 바인딩
	virtual void InitiateSkill(AInGameController* PC, APlayerUnit* SourceUnit) override;

	// 스킬 발동 가능 여부 (패에 카드가 1장 이상 존재하는지 검사)
	virtual bool CanInitiate(APlayerUnit* SourceUnit) const override;

	// 서버 측 활성화 조건 검증 (마나 및 패 확인)
	virtual bool CanActivate(APlayerUnit* SourceUnit, ATile* TargetTile, bool bCheckMana = true) const override;
	virtual bool CanActivateAction(APlayerUnit* SourceUnit, const FWildActionData& ActionData, bool bCheckMana = true) const override;

	// 실행 로직
	virtual bool Execute(APlayerUnit* SourceUnit, ATile* TargetTile) override;
	virtual bool ExecuteAction(APlayerUnit* SourceUnit, const FWildActionData& ActionData) override;

	virtual int32 GetSkillNumericValue() const override { return 1; }

	virtual bool RequiresPlayerPromptOnResolve(const FWildActionData& ActionData) const override;
	virtual void PromptPlayerOnResolve(AInGameController* PC, const FWildActionData& ActionData) override;
};
