// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BattleData.generated.h"

class AUnitBase;
class ACardBase;
class ATile;

UENUM(BlueprintType)
enum class EWildMatchState : uint8
{
	Init,               // 게임 시작 전 (플레이어 접속 대기)
	Gameset,            // 성유물, 전술 스킬 비공개 선택
	Mulligan,           // 5장 드로우 및 교체할 카드 선택
	Mulligan_End,       // [신규] 교체 애니메이션 대기 및 서버에서 새 카드 꽂아주는 시간
	MainGame,           // [신규] 본 게임 진행 중 (이 상태일 때만 아래의 TurnPhase가 돕니다)
	Match_End           // 승패 결정 및 결과 화면
};

UENUM(BlueprintType)
enum class EWildCardPhase : uint8
{
	None UMETA(DisplayName = "페이즈 없음"),               
	Turn_Start UMETA(DisplayName = "턴 시작"),             
	Action_Private UMETA(DisplayName = "비공개 행동 결정"),
	Action_Public UMETA(DisplayName = "체인 대기 (공개)"), 
	Action_Resolution UMETA(DisplayName = "체인 처리"),   
	Turn_End UMETA(DisplayName = "턴 종료")
};

UENUM(BlueprintType)
enum class EWildActionCategory : uint8
{
	None,
	Main,   // 메인 행동 (비공개 제출, 동시 처리)
	Sub     // 서브 행동 (공개 체인 공방용)
};

UENUM(BlueprintType)
enum class EWildActionType : uint8
{
	None,
	
	SummonUnit,     
	CastSpell,

	CastArtifact,    
	CastTacticalSkill,
	
	TriggerTrap,    
	UnitMove,       
	UnitAttack
};

UENUM(BlueprintType)
enum class EWildEffectTriggerTiming : uint8
{
	Instant = 0,     
	TurnStart,       
	TurnEnd,         
	OnTakeDamage,    
	OnDeath          
};

USTRUCT(BlueprintType)
struct WILDCARD_API FWildCardDisplayData
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Card Display")
	int32 InstanceID; 

	UPROPERTY(BlueprintReadOnly, Category = "Card Display")
	FName CardID; 

	UPROPERTY(BlueprintReadOnly, Category = "Card Display")
	int32 CurrentManaCost;
	UPROPERTY(BlueprintReadOnly, Category = "Card Display")
	int32 Force;
	UPROPERTY(BlueprintReadOnly, Category = "Card Display")
	int32 Intelligence;
	UPROPERTY(BlueprintReadOnly, Category = "Card Display")
	int32 Attack;
	UPROPERTY(BlueprintReadOnly, Category = "Card Display")
	int32 Health;

	bool operator==(const FWildCardDisplayData& Other) const
	{
		return CardID == Other.CardID;
	}

	FWildCardDisplayData() : InstanceID(-1), CardID(NAME_None), CurrentManaCost(0),Force(0), Intelligence(0), Attack(0), Health(0) {}
	FWildCardDisplayData(int32 InID, FName InCardID, int32 InMana, int32 InForce, int32 InIntelligence, int32 InAttack, int32 InHealth) : InstanceID(InID), CardID(InCardID), CurrentManaCost(InMana),Force(InForce), Intelligence(InIntelligence), Attack(InAttack), Health(InHealth)  {}
};

USTRUCT(BlueprintType)
struct WILDCARD_API FWildArtifactDisplayData
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Artifact Display")
	FName ArtifactID; 

	UPROPERTY(BlueprintReadOnly, Category = "Artifact Display")
	int32 CurrentDurability;

	UPROPERTY(BlueprintReadOnly, Category = "Artifact Display")
	int32 MaxDurability;

	UPROPERTY(BlueprintReadOnly, Category = "Artifact Display")
	bool bIsActive;

	FWildArtifactDisplayData() : ArtifactID(NAME_None), CurrentDurability(0), MaxDurability(0), bIsActive(false) {}
	FWildArtifactDisplayData(FName InID, int32 InCurrent, int32 InMax, bool InActive) 
		: ArtifactID(InID), CurrentDurability(InCurrent), MaxDurability(InMax), bIsActive(InActive) {}
};

USTRUCT(BlueprintType)
struct WILDCARD_API FWildActionData
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Action")
	int32 PlayerID;

	UPROPERTY(BlueprintReadWrite, Category = "Action")
	EWildActionCategory ActionCategory;

	UPROPERTY(BlueprintReadWrite, Category = "Action")
	EWildActionType ActionType;

	UPROPERTY(BlueprintReadWrite, Category = "Action|From Hand")
	int32 CardInstanceID; 

	UPROPERTY(BlueprintReadWrite, Category = "Action|On Board")
	TObjectPtr<AUnitBase> SourceUnit;

	UPROPERTY(BlueprintReadWrite, Category = "Action|Target")
	int32 TargetPlayerID;

	UPROPERTY(BlueprintReadWrite, Category = "Action|On Board")
	TObjectPtr<AUnitBase> TargetUnit;

	UPROPERTY(BlueprintReadWrite, Category = "Action|Target")
	int32 TargetCardInstanceID;

	UPROPERTY(BlueprintReadWrite, Category = "Action|On Board")
	FIntPoint TargetGridPos;

	UPROPERTY(BlueprintReadWrite, Category = "Action|On Board")
	TObjectPtr<ACardBase> TESTSourceUnit;

	UPROPERTY(BlueprintReadWrite, Category = "Action|On Board")
	TObjectPtr<ACardBase> TESTTargetUnit;

	UPROPERTY(BlueprintReadWrite, Category = "Action|On Board")
	TObjectPtr<ATile> TargetTile;

	FWildActionData()
		: PlayerID(-1),
	ActionCategory(EWildActionCategory::None),
	ActionType(EWildActionType::None), CardInstanceID(-1),
	SourceUnit(nullptr),
	TargetPlayerID(-1),
	TargetUnit(nullptr),
	TargetCardInstanceID(-1),
	TargetGridPos(FIntPoint(-1, -1)),
	TESTSourceUnit(nullptr),
	TESTTargetUnit(nullptr),
	TargetTile(nullptr)
	{}
};

USTRUCT(BlueprintType)
struct FWildDeckData
{
	GENERATED_BODY()

public:
	// 덱의 이름 (예: "화염 어그로 덱")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck")
	FString DeckName;

	// 덱에 포함된 카드들의 데이터 테이블 Row Name 목록 (예: "NewRow_1", "NewRow_15"...)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck")
	TArray<FName> CardIDs;

	// 기본 생성자
	FWildDeckData()
	{
		DeckName = TEXT("새 덱");
	}

	// 초기화 생성자
	FWildDeckData(const FString& InName, const TArray<FName>& InCards)
		: DeckName(InName), CardIDs(InCards)
	{
	}
};

USTRUCT(BlueprintType)
struct FWildDeckList
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck")
	TArray<FWildDeckData> Decks;

	FWildDeckList() {}
};

UENUM(BlueprintType)
enum class ECardType : uint8
{
	// �÷��̾� ī���� ����
	CLASS,

	// �ڿ�
	NATURE,

	// ������
	ITEM,

	// �ֹ�
	SPELL,

	// ����
	TAO,

	// ����
	UNIT,

	// ...
};

// ����, ����, �̹�, �߼� �� �±� �Է�
UENUM(BlueprintType)
enum class ECardTag : uint8
{
	// ���� 
	CREATURE,

	// ����
	INSECT,

	// �̹�
	INSIGNIFICANT,

	// �߼�
	BEAST,

	// ����
	SMALL,

	// ��ο�
	DRAW,

	// ������
	MAGE,

	// ����
	SAGE,

	// ...

};

/// ī�尡 ������ Ű����
UENUM(BlueprintType)
enum class ECardKeyword : uint8
{
	// �� �Ͽ� �� ���� ��� ������ ȿ��
	ONCEPERTURN,

	// ����: ������ �����̳� ��ֹ��� �����ϸ� ���������� �� ĭ����ŭ �̵��� �� �ִ�.
	FLYING,

	// ��ȯ: ������ ���� ���� ������ �������� �ҷ���
	SUMMON,

	// ����: �ϳ��� ������ �ϳ��� ������ ������. ��� ���ֿ��� ���� ȿ���� �����μ� �ο�
	EQUIP,

	// ...
};

// ī�� ȿ���� ����(��, ������, ī�� �̵�(�Ҹ�, �߹�, ��ο�, ������ �̵� ��), ī���� ���� ��)
UENUM(BlueprintType)
enum class EEffectType : uint8
{
	// ī�� Health ȸ��
	HEAL,

	// ī�� Health ����
	DAMAGE,

	// ī�� �̵�(� �̵����� ECardMoveTo���� �ٽ� ����)
	MOVE,

	// ������ ����
	MUTANTATION,

	// ���� 
	ELIMINATE,

	// ī�� ��ο�
	DRAW,

	// ��ġ
	PLACE,


	// ...

};

// ��� ��ġ�� �ִ� ī������ 
UENUM(BlueprintType)
enum class ECardZone : uint8
{
	// ��
	DECK,
	// ��
	HAND,
	// �߹�����
	OBLITERATE,
	// �Ҹ�����
	EXILE,
	// ����
	BATTLEFIELD,
	// ����
	NONE
};

//UENUM(BlueprintType)
//enum class ECardMoveTo : uint8
//{
//	// ��(Deck), ��(Hand), �Ҹ�����(Obliterate), �߹�����(Exile), ����(Battlefield)
//	NONE,
//	D2H,
//	D2O,
//	D2E,
//	D2B,
//	H2D,
//	H2O,
//	H2E,
//	H2B
//};

/// ī�� ȿ���� �ߵ��Ǵ� Ÿ�̹�
UENUM(BlueprintType)
enum class EEffectTrigger : uint8
{
	// ī�尡 ��ȯ �Ǿ��� ��
	ONPLAYED,

	// ī�尡 ���ŵǾ��� ��
	ONDEATH,

	// ī�尡 �����Ǿ��� ��
	ONEQUIP,

	// ���� ���۵� ��
	ONTURNSTART, 

	// ���� ���� ��
	ONTURNEND, 

	// ī�尡 ������ ��
	ONATTACK, 

	// ī�尡 ���ظ� �Ծ��� ��
	ONDAMAGED, 
};



USTRUCT(BlueprintType)
struct FEffectData
{
	GENERATED_BODY();

	// ȿ�� ���� 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EEffectType Type;

	// ī�� �̵� ������
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECardZone MoveTo;

	// ī�� �̵� �����
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECardZone MoveFrom;

	// ȿ�� ���� Ÿ�̹�
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EEffectTrigger Trigger;

	// ���� ��
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int IntValue = 0;

	// �Ǽ� ��
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FloatValue = 0.0f;

	FEffectData() = default;

	
};

USTRUCT(BlueprintType)
struct FCardData : public FTableRowBase
{
	GENERATED_BODY()
	//#. DB���� ���� ī�� �⺻ ���� ����
	//#. ���� DB �������� ������ ������ �� ��

public:

	FCardData() = default;

	FCardData(int32 InAttack, int32 InHealth)
		: Attack(InAttack)
		, Health(InHealth)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UnitStat")
	uint8 Force = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UnitStat")
	uint8 Intelligence = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UnitStat")
	int32 Attack = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UnitStat")
	int32 Health = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UnitStat")
	int32 CardId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnitStat")
	FGameplayTagContainer CardType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnitStat")
	FGameplayTagContainer CardTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnitStat")
	ECardKeyword CardKeyword = ECardKeyword::ONCEPERTURN;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UnitStat")
	UTexture2D* CardImage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UnitStat")
	UTexture2D* CardFrameImage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UnitStat")
	FString CardName = "";

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UnitStat")
	FString Description = "";

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UnitStat")
	FEffectData EffectData;

	bool operator==(const FCardData& Other) const
	{
		// TODO: �ϴ��� ���� CardId�� ������ ���� ī��� ����ϰڴ�
		return this->CardId == Other.CardId;
	}
};

UENUM(BlueprintType)
enum class EWildPlayerClass : uint8
{
	Common UMETA(DisplayName = "공용"),
	Warrior UMETA(DisplayName = "전사"),
	Mage UMETA(DisplayName = "마법사"),
	Taoist UMETA(DisplayName = "도사")
};

USTRUCT(BlueprintType)
struct FArtifactData : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Artifact|Info")
	FName ArtifactID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Artifact|Info")
	FText ArtifactName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Artifact|Info")
	FText EffectText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Artifact|Stats")
	int32 MaxDurability;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Artifact|Condition")
	EWildPlayerClass RequiredClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Artifact|UI")
	TSoftObjectPtr<UTexture2D> ArtifactImage;

	// [수정됨] 카드처럼 성유물이 발동시킬 효과 클래스들의 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Artifact|System")
	TArray<TSubclassOf<class UEffectBase>> EffectClasses;
};
