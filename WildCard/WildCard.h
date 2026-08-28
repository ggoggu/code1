// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(WILD, Log, All);

DECLARE_LOG_CATEGORY_EXTERN(LogWildPhase, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogWildChain, Log, All);


#define WILD_CALLINFO (FString(__FUNCTION__) + TEXT("(") + FString::FromInt(__LINE__) + TEXT(")"))

#define WILD_SIMPLE(Verbosity) UE_LOG(WILD, Verbosity, TEXT("%s"), *WILD_CALLINFO)

#define WILD_UELOG(Verbosity, Format, ...) UE_LOG(WILD, Verbosity, TEXT("%s %s"), *WILD_CALLINFO, *FString::Printf(Format, ##__VA_ARGS__))

#define WILD_CHECK(Expr, ...) {if(!(Expr)) { WILD_UELOG(Error, TEXT("Assertion : %s"), TEXT("'"#Expr"'")); return __VA_ARGS__; }}

#define WILD_SIMPLE_CHECK(Expr, ...) {if(!(Expr)) { WILD_UELOG(Warning, TEXT("The function is pre-termination : %s"), TEXT("'"#Expr"'")); return __VA_ARGS__; }}

#define WILD_PRINT_A_FRAME(Color, Format, ...){if(GEngine){GEngine->AddOnScreenDebugMessage(-1, FApp::GetDeltaTime(), Color, FString::Printf(Format, ##__VA_ARGS__));}}