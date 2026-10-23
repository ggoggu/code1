// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WildLoginGameMode.generated.h"

/**
 * 로그인/회원가입 레벨 전용 게임모드
 * 캐릭터 스폰을 비활성화하고 AWildLoginPlayerController를 기본 컨트롤러로 지정합니다.
 */
UCLASS()
class WILDCARD_API AWildLoginGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWildLoginGameMode();
};
