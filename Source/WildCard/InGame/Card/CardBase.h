#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "BattleData.h"
#include "CardBase.generated.h"

struct FCardData;

USTRUCT(Blueprintable, BlueprintType)
struct FUnitCardData
{
	GENERATED_BODY()

	// 카드 ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CardID;

	// 카드 타입
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer CardType;

	// 카드 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer CardTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECardKeyword CardKeyword;

	// 카드를 내기 위한 비용
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 Force;

	// intelligence
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 intelligence;

	// 카드 공격력
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Attack;

	// 카드 체력
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Health;

	// 카드 Owner
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 OwnerId;

	// 카드 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString CardName;

	//#. 기본 생성자
	FUnitCardData() = default;

	//#. FCardData -> FUnitCardData 변환 생성자
	FUnitCardData(const FCardData& InCardData, int32 InOwnerId)
	{
		CardID = InCardData.CardId;
		CardType = InCardData.CardType;
		CardTag = InCardData.CardTag;
		CardKeyword = InCardData.CardKeyword;
		Force = InCardData.Force;
		intelligence = InCardData.Intelligence;
		Attack = InCardData.Attack;
		Health = InCardData.Health;
		OwnerId = InOwnerId;
		CardName = InCardData.CardName;
	}
};

UCLASS(BlueprintType)
class WILDCARD_API UCardBase : public UObject
{
	GENERATED_BODY()
	
public:	
	UCardBase();

	void InitializeCard(int32 InInstanceID, FName InCardID, int32 InCurrentManaCost, int32 InCurrentForce, int32 InCurrentIntelligence, int32 InCurrentAttack, int32 InCurrentHealth);

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	int32 InstanceID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	FName CardID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	int32 CurrentManaCost;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	int32 CurrentForce;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	int32 CurrentIntelligence;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	int32 CurrentAttack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Card")
	int32 CurrentHealth;
};
