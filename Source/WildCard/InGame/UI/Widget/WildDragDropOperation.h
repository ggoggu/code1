// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "WildDragDropOperation.generated.h"

class UWildCardViewModel;

UCLASS()
class WILDCARD_API UWildDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "DragDrop", meta = (ExposeOnSpawn = "true"))
	UWildCardViewModel* DraggedCardViewModel;
	
};
