// Fill out your copyright notice in the Description page of Project Settings.

#include "WildLoginGameMode.h"
#include "WildLoginPlayerController.h"

AWildLoginGameMode::AWildLoginGameMode()
{
	PlayerControllerClass = AWildLoginPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
}
