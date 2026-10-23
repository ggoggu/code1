// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BattleData.generated.h"

class AUnitBase;
class UCardBase;
class ATile;
class UEffectBase;
class UTacticalSkillBase;

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
enum class EWildTimingNoticeType : uint8
{
	None = 0 UMETA(DisplayName = "없음"),
	TurnStart UMETA(DisplayName = "턴 시작"),
	ActionPhase UMETA(DisplayName = "행동 결정"),
	ChainStart UMETA(DisplayName = "체인 시작"),
	ChainResolution UMETA(DisplayName = "체인 처리"),
	TurnEnd UMETA(DisplayName = "턴 종료"),
	Gameset UMETA(DisplayName = "게임 준비"),
	Mulligan UMETA(DisplayName = "멀리건"),
	Victory UMETA(DisplayName = "승리"),
	Defeat UMETA(DisplayName = "패배"),
	MainActionReveal UMETA(DisplayName = "메인 행동 공개"),
	MainActionResolution UMETA(DisplayName = "메인 행동 처리"),
	Custom UMETA(DisplayName = "사용자 정의")
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
	UnitAttack,
	Draw
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
		return InstanceID == Other.InstanceID
			&& CardID == Other.CardID
			&& CurrentManaCost == Other.CurrentManaCost
			&& Force == Other.Force
			&& Intelligence == Other.Intelligence
			&& Attack == Other.Attack
			&& Health == Other.Health;
	}

	bool operator!=(const FWildCardDisplayData& Other) const
	{
		return !(*this == Other);
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

	UPROPERTY(BlueprintReadWrite, Category = "Action|From Hand")
	FName CardID;

	UPROPERTY(BlueprintReadWrite, Category = "Action|Target")
	int32 TargetPlayerID;

	UPROPERTY(BlueprintReadWrite, Category = "Action|Target")
	int32 TargetCardInstanceID;

	UPROPERTY(BlueprintReadWrite, Category = "Action|On Board")
	TObjectPtr<ATile> SourceTile;

	UPROPERTY(BlueprintReadWrite, Category = "Action|On Board")
	TObjectPtr<ATile> TargetTile;

	FWildActionData()
		: PlayerID(-1),
	ActionCategory(EWildActionCategory::None),
	ActionType(EWildActionType::None), CardInstanceID(-1),
	CardID(NAME_None),
	TargetPlayerID(-1),
	TargetCardInstanceID(-1),
	SourceTile(nullptr),
	TargetTile(nullptr)
	{}
};

USTRUCT(BlueprintType)
struct WILDCARD_API FWildVisualCueData
{
	GENERATED_BODY()

public:
	// 연출 식별 태그 (예: Visual.Effect.Damage.Fire, Visual.Card.Cutscene, Visual.Unit.Buff)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VisualCue")
	FGameplayTag CueTag;

	// 시전자 / 플레이어 정보
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VisualCue")
	int32 InstigatorPlayerID = -1;

	// 시전자 / 대상 액터 (유닛 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VisualCue")
	TWeakObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VisualCue")
	TWeakObjectPtr<AActor> TargetActor = nullptr;

	// 타일 좌표 (Q, R)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VisualCue")
	int32 SourceQ = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VisualCue")
	int32 SourceR = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VisualCue")
	int32 TargetQ = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VisualCue")
	int32 TargetR = -1;

	// 연출에 사용할 수치 (데미지량, 회복량, 스탯 변화 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VisualCue")
	int32 NumericValue = 0;

	// 식별자 (카드 ID, 스킬 ID 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VisualCue")
	FName SourceID = NAME_None;

	// 보조 텍스트 (스킬명, 안내 문구 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VisualCue")
	FText CueMessage = FText::GetEmpty();

	FWildVisualCueData() = default;
};

UENUM(BlueprintType)
enum class EWildPlayerClass : uint8
{
	Common UMETA(DisplayName = "공용"),
	Warrior UMETA(DisplayName = "전사"),
	Mage UMETA(DisplayName = "마법사"),
	Taoist UMETA(DisplayName = "도사")
};

UENUM(BlueprintType)
enum class ETacticalSkillTargetType : uint8
{
	Self UMETA(DisplayName = "자가 시전"),
	TargetTile UMETA(DisplayName = "대상 타일"),
	TargetEnemyUnit UMETA(DisplayName = "대상 적 유닛"),
	TargetAllyUnit UMETA(DisplayName = "대상 아군 유닛")
};

USTRUCT(BlueprintType)
struct FWildDeckData
{
	GENERATED_BODY()

public:
	// 덱의 이름 (예: "화염 어그로 덱")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck")
	FString DeckName;

	// 해당 덱의 직업
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck")
	EWildPlayerClass DeckClass = EWildPlayerClass::Warrior;

	// 선택된 전술 스킬 식별자 (데이터 테이블 행 이름 또는 스킬 ID)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck")
	FName SelectedTacticalSkillID = NAME_None;

	// 선택된 전술 스킬 클래스 (직접 지정 시)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck")
	TSubclassOf<class UTacticalSkillBase> SelectedTacticalSkillClass = nullptr;

	// 덱에 포함된 카드들의 데이터 테이블 Row Name 목록 (예: "NewRow_1", "NewRow_15"...)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deck")
	TArray<FName> CardIDs;

	// 기본 생성자
	FWildDeckData()
		: DeckName(TEXT("새 덱"))
		, DeckClass(EWildPlayerClass::Warrior)
		, SelectedTacticalSkillID(NAME_None)
		, SelectedTacticalSkillClass(nullptr)
	{
	}

	// 초기화 생성자
	FWildDeckData(const FString& InName, const TArray<FName>& InCards, EWildPlayerClass InClass = EWildPlayerClass::Warrior, FName InSkillID = NAME_None, TSubclassOf<class UTacticalSkillBase> InSkillClass = nullptr)
		: DeckName(InName)
		, DeckClass(InClass)
		, SelectedTacticalSkillID(InSkillID)
		, SelectedTacticalSkillClass(InSkillClass)
		, CardIDs(InCards)
	{
	}

	// 전술 스킬 장착 가능 여부 검증 (해당 직업 전용이거나 공용인 경우만 장착 가능)
	bool CanEquipTacticalSkill(EWildPlayerClass SkillRequiredClass) const
	{
		return SkillRequiredClass == EWildPlayerClass::Common || SkillRequiredClass == DeckClass;
	}

	// 덱 유효성 (26장 또는 30장 구성 지원)
	bool IsValidDeck() const
	{
		return CardIDs.Num() == 26 || CardIDs.Num() == 30;
	}

	// 현재 덱의 카드 장수
	int32 GetCardCount() const
	{
		return CardIDs.Num();
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
	INCREASE,

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

	NONE
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	FGameplayTag VisualCueTag;

	FEffectData() = default;

	
};

UENUM(BlueprintType)
enum class ECardRarity : uint8
{
	Common UMETA(DisplayName = "일반"),
	Rare UMETA(DisplayName = "희귀"),
	Epic UMETA(DisplayName = "영웅"),
	Legendary UMETA(DisplayName = "전설")
};

USTRUCT(BlueprintType)
struct FCardData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FCardData() = default;

	FCardData(int32 InAttack, int32 InHealth)
		: BaseManaCost(1)
		, Attack(InAttack)
		, Health(InHealth)
	{
	}

	// --- 1. 코스트 및 기본 스탯 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
	int32 BaseManaCost = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UnitStat")
	uint8 Force = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UnitStat")
	uint8 Intelligence = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UnitStat")
	int32 Attack = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UnitStat")
	int32 Health = 0;

	// --- 2. 덱 제작 및 메타데이터 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck")
	bool bIsCollectible = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck")
	EWildPlayerClass CardClass = EWildPlayerClass::Common;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck")
	ECardRarity Rarity = ECardRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deck")
	int32 MaxDeckCount = 2;

	// 기존 호환용 식별자
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UnitStat")
	int32 CardId = 0;

	// --- 3. 분류, 태그 및 키워드 ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnitStat")
	FGameplayTagContainer CardType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnitStat")
	FGameplayTagContainer CardTag;

	// 단일 키워드 (기존 호환)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnitStat")
	ECardKeyword CardKeyword = ECardKeyword::ONCEPERTURN;

	// 다중 키워드 확장용 (GameplayTags)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UnitStat")
	FGameplayTagContainer CardKeywords;

	// --- 4. 소환 액터 및 효과 시스템 (확장성) ---
	// 유닛 카드일 경우 스폰할 액터 클래스 (비동기 로드 지원)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning")
	TSoftClassPtr<class AUnitBase> UnitActorClass;

	// 다중 효과 클래스 (성유물 FArtifactData와 통일)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	TArray<TSubclassOf<class UEffectBase>> EffectClasses;

	// 단일 효과 데이터 (기존 호환용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UnitStat")
	FEffectData EffectData;

	// 카드 발동 시 재생할 시각/UI 연출 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	FGameplayTag VisualCueTag;

	// --- 5. UI 및 시각 에셋 (메모리 최적화) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSoftObjectPtr<UTexture2D> CardImage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSoftObjectPtr<UTexture2D> CardFrameImage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	FString CardName = "";

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	FString Description = "";

	// --- 6. 헬퍼 메서드 ---
	UTexture2D* GetCardImage() const
	{
		return CardImage.IsValid() ? CardImage.Get() : CardImage.LoadSynchronous();
	}

	UTexture2D* GetCardFrameImage() const
	{
		return CardFrameImage.IsValid() ? CardFrameImage.Get() : CardFrameImage.LoadSynchronous();
	}

	bool CanAddToDeck(EWildPlayerClass InDeckClass, int32 CurrentCountInDeck) const
	{
		if (!bIsCollectible) return false;
		if (CardClass != EWildPlayerClass::Common && CardClass != InDeckClass) return false;
		if (CurrentCountInDeck >= MaxDeckCount) return false;
		return true;
	}

	bool operator==(const FCardData& Other) const
	{
		return this->CardId == Other.CardId;
	}
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

USTRUCT(BlueprintType)
struct FTacticalSkillData : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TacticalSkill|Info")
	FName SkillID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TacticalSkill|Info")
	FText SkillName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TacticalSkill|Info")
	FText SkillDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TacticalSkill|Condition")
	EWildPlayerClass RequiredClass = EWildPlayerClass::Common;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TacticalSkill|Stats")
	int32 ManaCost = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TacticalSkill|Stats")
	int32 Range = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TacticalSkill|Stats")
	ETacticalSkillTargetType TargetType = ETacticalSkillTargetType::Self;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TacticalSkill|System")
	TSubclassOf<class UTacticalSkillBase> SkillClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TacticalSkill|UI")
	TSoftObjectPtr<UTexture2D> SkillIcon;

	// 전술 스킬 발동 시 재생할 시각/UI 연출 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TacticalSkill|Visual")
	FGameplayTag VisualCueTag;
};

UENUM(BlueprintType)
enum class EWildCombatLogType : uint8
{
	None = 0,
	TurnPriority,       // [1] 선공 결정
	Gameset,            // [2] Gameset (적/아군 직업)
	ArtifactSelect,     // [3] 성유물 선택
	ArtifactReveal,     // [4] 성유물 / 전술 스킬 공개
	Mulligan,           // [5] 각 플레이어 멀리건 실행
	TurnStart,          // [6] Turn start (드로우)
	ForceCharge,        // [7] 포스 충전
	PhasePrivate,       // [8] Private
	PhasePublic,        // [9] Public : 메인 행동 공개
	ActionResolve,      // [10] 로그 [X]의 처리 (소환, 주문, 이동 등)
	TurnEnd,            // [13] End phase
	ChainCast,          // [?] 적/아군이 {N} 체인 시전
	Combat,             // [?] 전투 발생!
	ActionFailed        // [?] 로그 [X]의 행동이 처리되지 못했습니다
};

USTRUCT(BlueprintType)
struct WILDCARD_API FWildCombatLogEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 LogIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 ParentLogIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	EWildCombatLogType LogType = EWildCombatLogType::None;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 InstigatorPlayerID = -1;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 TargetPlayerID = -1;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	EWildActionType ActionType = EWildActionType::None;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	FName CardID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	FString CardNameStr = "";

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	FName SkillID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	FString SkillNameStr = "";

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	FName ArtifactID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	FString ArtifactNameStr = "";

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 ForceBefore = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 ForceAfter = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 ForceCharged = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 ChainStackCount = 0;

	// 타일 좌표 (Q, R)
	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 SourceQ = -1;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 SourceR = -1;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 TargetQ = -1;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 TargetR = -1;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 BattleQ = -1;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 BattleR = -1;

	// 유닛 명칭 및 소유자 (전투 / 대상 지정용)
	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	FString SourceUnitName = "";

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 SourceUnitOwnerID = -1;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	FString TargetUnitName = "";

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 TargetUnitOwnerID = -1;

	// 직업 정보 (Gameset용)
	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	EWildPlayerClass Player1Class = EWildPlayerClass::Common;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 Player1ID = -1;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	EWildPlayerClass Player2Class = EWildPlayerClass::Common;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 Player2ID = -1;

	// 성유물/스킬 정보 (ArtifactReveal용)
	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	FString Player1ArtifactName = "";

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	FString Player1SkillName = "";

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	FString Player2ArtifactName = "";

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	FString Player2SkillName = "";

	// 메인 행동 공개용 세부 데이터 (Player1 / Player2)
	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	bool bPlayer1NoAction = false;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	FString Player1ActionCardName = "";

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	EWildActionType Player1ActionType = EWildActionType::None;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 Player1ForceBefore = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 Player1ForceAfter = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 Player1TargetQ = -1;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 Player1TargetR = -1;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	bool bPlayer2NoAction = false;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	FString Player2ActionCardName = "";

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	EWildActionType Player2ActionType = EWildActionType::None;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 Player2ForceBefore = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 Player2ForceAfter = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 Player2TargetQ = -1;

	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 Player2TargetR = -1;

	// 커스텀 설명 또는 실패 사유
	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	FString ExtraMessage = "";

	// 비공개 로그 여부
	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	bool bIsPrivate = false;

	FWildCombatLogEntry() = default;
};

