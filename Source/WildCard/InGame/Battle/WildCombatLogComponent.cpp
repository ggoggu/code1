// Fill out your copyright notice in the Description page of Project Settings.

#include "InGame/Battle/WildCombatLogComponent.h"
#include "Net/UnrealNetwork.h"
#include "InGameState.h"
#include "InGamePlayerState.h"
#include "InGameController.h"
#include "WildCardGameInstance.h"

UWildCombatLogComponent::UWildCombatLogComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	NextLogIndex = 1;
	LastPublicPhaseLogIndex = -1;
}

void UWildCombatLogComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

FString UWildCombatLogComponent::GetPlayerClassName(EWildPlayerClass InClass)
{
	switch (InClass)
	{
	case EWildPlayerClass::Warrior: return TEXT("전사");
	case EWildPlayerClass::Mage:    return TEXT("마법사");
	case EWildPlayerClass::Taoist:  return TEXT("도사");
	case EWildPlayerClass::Common:
	default:                        return TEXT("공용");
	}
}

FString UWildCombatLogComponent::GetCardDisplayName(const UWorld* World, FName CardID)
{
	if (!World || CardID.IsNone())
	{
		return TEXT("");
	}

	if (UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(World->GetGameInstance()))
	{
		if (GI->CardDataTable)
		{
			if (FCardData* FoundData = GI->CardDataTable->FindRow<FCardData>(CardID, TEXT("CombatLogCardName")))
			{
				if (!FoundData->CardName.IsEmpty())
				{
					return FoundData->CardName;
				}
			}
		}
	}

	return CardID.ToString();
}

FString UWildCombatLogComponent::GetArtifactDisplayName(const UWorld* World, FName ArtifactID)
{
	if (!World || ArtifactID.IsNone())
	{
		return TEXT("");
	}

	if (UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(World->GetGameInstance()))
	{
		if (GI->ArtifactDataTable)
		{
			if (FArtifactData* FoundData = GI->ArtifactDataTable->FindRow<FArtifactData>(ArtifactID, TEXT("CombatLogArtifactName")))
			{
				if (!FoundData->ArtifactName.IsEmpty())
				{
					return FoundData->ArtifactName.ToString();
				}
			}
		}
	}

	return ArtifactID.ToString();
}

FString UWildCombatLogComponent::GetTacticalSkillDisplayName(const UWorld* World, FName SkillID)
{
	if (!World || SkillID.IsNone())
	{
		return TEXT("");
	}

	if (UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(World->GetGameInstance()))
	{
		if (GI->TacticalSkillDataTable)
		{
			if (FTacticalSkillData* FoundData = GI->TacticalSkillDataTable->FindRow<FTacticalSkillData>(SkillID, TEXT("CombatLogSkillName")))
			{
				if (!FoundData->SkillName.IsEmpty())
				{
					return FoundData->SkillName.ToString();
				}
			}
		}
	}

	return SkillID.ToString();
}

FString UWildCombatLogComponent::FormatTileCoord(int32 Q, int32 R, int32 UnitOwnerID, int32 FirstPlayerID)
{
	if (Q < 0 || R < 0) return TEXT("(-,-)");

	TCHAR RowChar = TCHAR('A' + FMath::Clamp(Q, 0, 25));

	if (UnitOwnerID != -1)
	{
		TCHAR TeamChar = (FirstPlayerID != -1 && UnitOwnerID == FirstPlayerID) ? 'R' : 'B';
		return FString::Printf(TEXT("(%c,%d,%c)"), RowChar, R, TeamChar);
	}

	return FString::Printf(TEXT("(%c,%d)"), RowChar, R);
}

void UWildCombatLogComponent::FormatLogEntry(const FWildCombatLogEntry& Entry, int32 LocalPlayerID, int32 FirstPlayerID, FString& OutTitle, TArray<FString>& OutDetails)
{
	OutDetails.Empty();

	switch (Entry.LogType)
	{
	case EWildCombatLogType::TurnPriority:
		{
			if (Entry.InstigatorPlayerID == LocalPlayerID)
			{
				OutTitle = FString::Printf(TEXT("[%d] 당신이 선공입니다."), Entry.LogIndex);
			}
			else
			{
				OutTitle = FString::Printf(TEXT("[%d] 적이 선공입니다."), Entry.LogIndex);
			}
		}
		break;

	case EWildCombatLogType::Gameset:
		{
			OutTitle = FString::Printf(TEXT("[%d] Gameset"), Entry.LogIndex);

			EWildPlayerClass AllyClass = (Entry.Player1ID == LocalPlayerID) ? Entry.Player1Class : Entry.Player2Class;
			EWildPlayerClass EnemyClass = (Entry.Player1ID == LocalPlayerID) ? Entry.Player2Class : Entry.Player1Class;

			OutDetails.Add(FString::Printf(TEXT("-적의 직업 : %s"), *GetPlayerClassName(EnemyClass)));
			OutDetails.Add(FString::Printf(TEXT("-아군의 직업 : %s"), *GetPlayerClassName(AllyClass)));
		}
		break;

	case EWildCombatLogType::ArtifactSelect:
		{
			OutTitle = FString::Printf(TEXT("[%d] 성유물 선택"), Entry.LogIndex);
		}
		break;

	case EWildCombatLogType::ArtifactReveal:
		{
			OutTitle = FString::Printf(TEXT("[%d] 성유물 / 전술 스킬 공개"), Entry.LogIndex);

			FString AllyArtifact = (Entry.Player1ID == LocalPlayerID) ? Entry.Player1ArtifactName : Entry.Player2ArtifactName;
			FString AllySkill = (Entry.Player1ID == LocalPlayerID) ? Entry.Player1SkillName : Entry.Player2SkillName;
			FString EnemyArtifact = (Entry.Player1ID == LocalPlayerID) ? Entry.Player2ArtifactName : Entry.Player1ArtifactName;
			FString EnemySkill = (Entry.Player1ID == LocalPlayerID) ? Entry.Player2SkillName : Entry.Player1SkillName;

			OutDetails.Add(FString::Printf(TEXT("-적 :  {%s / %s}"), *EnemyArtifact, *EnemySkill));
			OutDetails.Add(FString::Printf(TEXT("-아군 : {%s / %s}"), *AllyArtifact, *AllySkill));
		}
		break;

	case EWildCombatLogType::Mulligan:
		{
			OutTitle = FString::Printf(TEXT("[%d] 각 플레이어 멀리건 실행"), Entry.LogIndex);
		}
		break;

	case EWildCombatLogType::TurnStart:
		{
			OutTitle = FString::Printf(TEXT("[%d] Turn start"), Entry.LogIndex);
			if (Entry.CardNameStr.IsEmpty())
			{
				OutDetails.Add(TEXT("-적 : 카드 1장 드로우"));
			}
			else
			{
				OutDetails.Add(FString::Printf(TEXT("-각 플레이어 드로우, {%s} 뽑음"), *Entry.CardNameStr));
			}
		}
		break;

	case EWildCombatLogType::ForceCharge:
		{
			OutTitle = FString::Printf(TEXT("[%d] 포스 충전 : %d"), Entry.LogIndex, Entry.ForceCharged);
		}
		break;

	case EWildCombatLogType::PhasePrivate:
		{
			OutTitle = FString::Printf(TEXT("[%d] Private"), Entry.LogIndex);
		}
		break;

	case EWildCombatLogType::PhasePublic:
		{
			OutTitle = FString::Printf(TEXT("[%d] Public : 메인 행동 공개"), Entry.LogIndex);

			bool bP1IsAlly = (Entry.Player1ID == LocalPlayerID);
			bool bAllyNoAction = bP1IsAlly ? Entry.bPlayer1NoAction : Entry.bPlayer2NoAction;
			bool bEnemyNoAction = bP1IsAlly ? Entry.bPlayer2NoAction : Entry.bPlayer1NoAction;

			const FString& AllyCardName = bP1IsAlly ? Entry.Player1ActionCardName : Entry.Player2ActionCardName;
			const FString& EnemyCardName = bP1IsAlly ? Entry.Player2ActionCardName : Entry.Player1ActionCardName;

			EWildActionType AllyActionType = bP1IsAlly ? Entry.Player1ActionType : Entry.Player2ActionType;
			EWildActionType EnemyActionType = bP1IsAlly ? Entry.Player2ActionType : Entry.Player1ActionType;

			int32 AllyForceBefore = bP1IsAlly ? Entry.Player1ForceBefore : Entry.Player2ForceBefore;
			int32 AllyForceAfter = bP1IsAlly ? Entry.Player1ForceAfter : Entry.Player2ForceAfter;
			int32 EnemyForceBefore = bP1IsAlly ? Entry.Player2ForceBefore : Entry.Player1ForceBefore;
			int32 EnemyForceAfter = bP1IsAlly ? Entry.Player2ForceAfter : Entry.Player1ForceAfter;

			int32 AllyTargetQ = bP1IsAlly ? Entry.Player1TargetQ : Entry.Player2TargetQ;
			int32 AllyTargetR = bP1IsAlly ? Entry.Player1TargetR : Entry.Player2TargetR;
			int32 EnemyTargetQ = bP1IsAlly ? Entry.Player2TargetQ : Entry.Player1TargetQ;
			int32 EnemyTargetR = bP1IsAlly ? Entry.Player2TargetR : Entry.Player1TargetR;

			int32 AllyID = LocalPlayerID;
			int32 EnemyID = bP1IsAlly ? Entry.Player2ID : Entry.Player1ID;

			// 아군 행동 라인
			if (bAllyNoAction)
			{
				OutDetails.Add(TEXT("=아군 : 행동 없음"));
			}
			else
			{
				FString Verb = (AllyActionType == EWildActionType::SummonUnit) ? TEXT("소환 시전") : TEXT("시전");
				OutDetails.Add(FString::Printf(TEXT("=아군 : 패에서 {%s} %s"), *AllyCardName, *Verb));
				OutDetails.Add(FString::Printf(TEXT("-포스 {%d} →{%d}"), AllyForceBefore, AllyForceAfter));
				OutDetails.Add(FString::Printf(TEXT("-대상 위치 : %s"), *FormatTileCoord(AllyTargetQ, AllyTargetR, AllyID, FirstPlayerID)));
				OutDetails.Add(TEXT("-우선권 제거됨"));
			}

			// 적 행동 라인
			if (bEnemyNoAction)
			{
				OutDetails.Add(TEXT("=적 : 행동 없음"));
			}
			else
			{
				FString Verb = (EnemyActionType == EWildActionType::SummonUnit) ? TEXT("소환 시전") : TEXT("시전");
				OutDetails.Add(FString::Printf(TEXT("=적 : 패에서 {%s} %s"), *EnemyCardName, *Verb));
				OutDetails.Add(FString::Printf(TEXT("-포스 {%d} →{%d}"), EnemyForceBefore, EnemyForceAfter));
				OutDetails.Add(FString::Printf(TEXT("-대상 위치 : %s"), *FormatTileCoord(EnemyTargetQ, EnemyTargetR, EnemyID, FirstPlayerID)));
				OutDetails.Add(TEXT("-우선권 제거됨"));
			}
		}
		break;

	case EWildCombatLogType::ActionResolve:
		{
			FString ActionVerb = (Entry.ActionType == EWildActionType::SummonUnit) ? TEXT("소환") : (Entry.ActionType == EWildActionType::CastSpell ? TEXT("주문") : TEXT("행동"));
			OutTitle = FString::Printf(TEXT("[%d] 로그 [%d]의 %s 처리 : %s"), Entry.LogIndex, Entry.ParentLogIndex, *ActionVerb, *Entry.ExtraMessage);
		}
		break;

	case EWildCombatLogType::TurnEnd:
		{
			OutTitle = FString::Printf(TEXT("[%d] End phase"), Entry.LogIndex);
		}
		break;

	case EWildCombatLogType::ChainCast:
		{
			bool bIsAlly = (Entry.InstigatorPlayerID == LocalPlayerID);
			FString PlayerLabel = bIsAlly ? TEXT("아군") : TEXT("적");

			OutTitle = FString::Printf(TEXT("[%d] %s이 {%d} 체인 시전"), Entry.LogIndex, *PlayerLabel, Entry.ChainStackCount);

			if (!Entry.ExtraMessage.IsEmpty())
			{
				OutDetails.Add(FString::Printf(TEXT("=%s : %s"), *PlayerLabel, *Entry.ExtraMessage));
			}
			OutDetails.Add(FString::Printf(TEXT("-포스 {%d} →{%d}"), Entry.ForceBefore, Entry.ForceAfter));

			FString TargetCoordStr = FormatTileCoord(Entry.TargetQ, Entry.TargetR, Entry.TargetUnitOwnerID, FirstPlayerID);
			if (!Entry.TargetUnitName.IsEmpty())
			{
				OutDetails.Add(FString::Printf(TEXT("-대상 위치 : %s의  {%s}"), *TargetCoordStr, *Entry.TargetUnitName));
			}
			else
			{
				OutDetails.Add(FString::Printf(TEXT("-대상 위치 : %s"), *TargetCoordStr));
			}
			OutDetails.Add(TEXT("-우선권 제거됨"));
		}
		break;

	case EWildCombatLogType::Combat:
		{
			OutTitle = FString::Printf(TEXT("[%d] 전투 발생!"), Entry.LogIndex);

			FString AttackerCoord = FormatTileCoord(Entry.SourceQ, Entry.SourceR, Entry.SourceUnitOwnerID, FirstPlayerID);
			FString DefenderCoord = FormatTileCoord(Entry.TargetQ, Entry.TargetR, Entry.TargetUnitOwnerID, FirstPlayerID);
			FString BattleCoord = FormatTileCoord(Entry.BattleQ, Entry.BattleR, -1, -1);

			OutDetails.Add(FString::Printf(TEXT("-%s의 {%s}"), *AttackerCoord, *Entry.SourceUnitName));
			OutDetails.Add(FString::Printf(TEXT("과 %s의 {%s}와  %s 에서의 {영역 다툼}으로 전투 발생"), *DefenderCoord, *Entry.TargetUnitName, *BattleCoord));
		}
		break;

	case EWildCombatLogType::ActionFailed:
		{
			OutTitle = FString::Printf(TEXT("[%d] 로그 [%d] 의 행동이 처리되지 못했습니다."), Entry.LogIndex, Entry.ParentLogIndex);
			if (!Entry.ExtraMessage.IsEmpty())
			{
				OutDetails.Add(FString::Printf(TEXT("사유: %s"), *Entry.ExtraMessage));
			}
		}
		break;

	default:
		OutTitle = FString::Printf(TEXT("[%d] 시스템 알림"), Entry.LogIndex);
		if (!Entry.ExtraMessage.IsEmpty())
		{
			OutDetails.Add(Entry.ExtraMessage);
		}
		break;
	}
}

// =========================================================================
// Server Dispatch Implementation
// =========================================================================

void UWildCombatLogComponent::DispatchPublicEntry(FWildCombatLogEntry& Entry)
{
	Entry.LogIndex = GenerateLogIndex();
	Multicast_AddLogEntry(Entry);
}

int32 UWildCombatLogComponent::LogTurnPriority(int32 FirstPlayerID)
{
	if (GetOwnerRole() != ROLE_Authority) return -1;

	FWildCombatLogEntry Entry;
	Entry.LogType = EWildCombatLogType::TurnPriority;
	Entry.InstigatorPlayerID = FirstPlayerID;
	DispatchPublicEntry(Entry);
	return Entry.LogIndex;
}

int32 UWildCombatLogComponent::LogGameset(int32 P1ID, EWildPlayerClass P1Class, int32 P2ID, EWildPlayerClass P2Class)
{
	if (GetOwnerRole() != ROLE_Authority) return -1;

	FWildCombatLogEntry Entry;
	Entry.LogType = EWildCombatLogType::Gameset;
	Entry.Player1ID = P1ID;
	Entry.Player1Class = P1Class;
	Entry.Player2ID = P2ID;
	Entry.Player2Class = P2Class;
	DispatchPublicEntry(Entry);
	return Entry.LogIndex;
}

int32 UWildCombatLogComponent::LogArtifactSelect()
{
	if (GetOwnerRole() != ROLE_Authority) return -1;

	FWildCombatLogEntry Entry;
	Entry.LogType = EWildCombatLogType::ArtifactSelect;
	DispatchPublicEntry(Entry);
	return Entry.LogIndex;
}

int32 UWildCombatLogComponent::LogArtifactAndSkillReveal(int32 P1ID, const FString& P1Artifact, const FString& P1Skill, int32 P2ID, const FString& P2Artifact, const FString& P2Skill)
{
	if (GetOwnerRole() != ROLE_Authority) return -1;

	FWildCombatLogEntry Entry;
	Entry.LogType = EWildCombatLogType::ArtifactReveal;
	Entry.Player1ID = P1ID;
	Entry.Player1ArtifactName = P1Artifact;
	Entry.Player1SkillName = P1Skill;
	Entry.Player2ID = P2ID;
	Entry.Player2ArtifactName = P2Artifact;
	Entry.Player2SkillName = P2Skill;
	DispatchPublicEntry(Entry);
	return Entry.LogIndex;
}

int32 UWildCombatLogComponent::LogMulligan()
{
	if (GetOwnerRole() != ROLE_Authority) return -1;

	FWildCombatLogEntry Entry;
	Entry.LogType = EWildCombatLogType::Mulligan;
	DispatchPublicEntry(Entry);
	return Entry.LogIndex;
}

int32 UWildCombatLogComponent::LogTurnStart(int32 P1ID, const FString& P1CardName, int32 P2ID, const FString& P2CardName)
{
	if (GetOwnerRole() != ROLE_Authority) return -1;

	int32 AssignedIndex = GenerateLogIndex();

	AInGameState* GS = Cast<AInGameState>(GetOwner());
	if (GS)
	{
		for (APlayerState* PS : GS->PlayerArray)
		{
			if (AInGamePlayerState* WildPS = Cast<AInGamePlayerState>(PS))
			{
				FWildCombatLogEntry PlayerEntry;
				PlayerEntry.LogIndex = AssignedIndex;
				PlayerEntry.LogType = EWildCombatLogType::TurnStart;

				if (WildPS->GetPlayerId() == P1ID)
				{
					PlayerEntry.CardNameStr = P1CardName;
				}
				else if (WildPS->GetPlayerId() == P2ID)
				{
					PlayerEntry.CardNameStr = P2CardName;
				}

				WildPS->Client_ReceiveCombatLogEntry(PlayerEntry);
			}
		}
	}

	return AssignedIndex;
}

int32 UWildCombatLogComponent::LogForceCharge(int32 PlayerID, int32 ChargedAmount)
{
	if (GetOwnerRole() != ROLE_Authority) return -1;

	FWildCombatLogEntry Entry;
	Entry.LogType = EWildCombatLogType::ForceCharge;
	Entry.InstigatorPlayerID = PlayerID;
	Entry.ForceCharged = ChargedAmount;
	DispatchPublicEntry(Entry);
	return Entry.LogIndex;
}

int32 UWildCombatLogComponent::LogPhasePrivate()
{
	if (GetOwnerRole() != ROLE_Authority) return -1;

	FWildCombatLogEntry Entry;
	Entry.LogType = EWildCombatLogType::PhasePrivate;
	DispatchPublicEntry(Entry);
	return Entry.LogIndex;
}

int32 UWildCombatLogComponent::LogPhasePublic(int32 P1ID, bool bP1NoAction, const FString& P1CardName, EWildActionType P1ActionType, int32 P1ForceBefore, int32 P1ForceAfter, int32 P1TargetQ, int32 P1TargetR,
                                              int32 P2ID, bool bP2NoAction, const FString& P2CardName, EWildActionType P2ActionType, int32 P2ForceBefore, int32 P2ForceAfter, int32 P2TargetQ, int32 P2TargetR)
{
	if (GetOwnerRole() != ROLE_Authority) return -1;

	FWildCombatLogEntry Entry;
	Entry.LogType = EWildCombatLogType::PhasePublic;
	Entry.Player1ID = P1ID;
	Entry.bPlayer1NoAction = bP1NoAction;
	Entry.Player1ActionCardName = P1CardName;
	Entry.Player1ActionType = P1ActionType;
	Entry.Player1ForceBefore = P1ForceBefore;
	Entry.Player1ForceAfter = P1ForceAfter;
	Entry.Player1TargetQ = P1TargetQ;
	Entry.Player1TargetR = P1TargetR;

	Entry.Player2ID = P2ID;
	Entry.bPlayer2NoAction = bP2NoAction;
	Entry.Player2ActionCardName = P2CardName;
	Entry.Player2ActionType = P2ActionType;
	Entry.Player2ForceBefore = P2ForceBefore;
	Entry.Player2ForceAfter = P2ForceAfter;
	Entry.Player2TargetQ = P2TargetQ;
	Entry.Player2TargetR = P2TargetR;

	DispatchPublicEntry(Entry);
	LastPublicPhaseLogIndex = Entry.LogIndex;
	return Entry.LogIndex;
}

int32 UWildCombatLogComponent::LogActionResolve(int32 ParentIndex, EWildActionType InActionType, int32 TargetQ, int32 TargetR, const FString& ResultDesc)
{
	if (GetOwnerRole() != ROLE_Authority) return -1;

	FWildCombatLogEntry Entry;
	Entry.LogType = EWildCombatLogType::ActionResolve;
	Entry.ParentLogIndex = (ParentIndex > 0) ? ParentIndex : LastPublicPhaseLogIndex;
	Entry.ActionType = InActionType;
	Entry.TargetQ = TargetQ;
	Entry.TargetR = TargetR;
	Entry.ExtraMessage = ResultDesc;
	DispatchPublicEntry(Entry);
	return Entry.LogIndex;
}

int32 UWildCombatLogComponent::LogTurnEnd()
{
	if (GetOwnerRole() != ROLE_Authority) return -1;

	FWildCombatLogEntry Entry;
	Entry.LogType = EWildCombatLogType::TurnEnd;
	DispatchPublicEntry(Entry);
	return Entry.LogIndex;
}

int32 UWildCombatLogComponent::LogChainCast(int32 ChainStackCount, int32 CasterPlayerID, const FString& CardOrEffectName, int32 BeforeForce, int32 AfterForce, int32 TargetQ, int32 TargetR, const FString& TargetUnitName, int32 TargetUnitOwnerID, bool bIsFromHand)
{
	if (GetOwnerRole() != ROLE_Authority) return -1;

	FWildCombatLogEntry Entry;
	Entry.LogType = EWildCombatLogType::ChainCast;
	Entry.ChainStackCount = ChainStackCount;
	Entry.InstigatorPlayerID = CasterPlayerID;
	Entry.ForceBefore = BeforeForce;
	Entry.ForceAfter = AfterForce;
	Entry.TargetQ = TargetQ;
	Entry.TargetR = TargetR;
	Entry.TargetUnitName = TargetUnitName;
	Entry.TargetUnitOwnerID = TargetUnitOwnerID;

	if (bIsFromHand)
	{
		Entry.ExtraMessage = FString::Printf(TEXT("패에서 {%s} 시전"), *CardOrEffectName);
	}
	else
	{
		Entry.ExtraMessage = CardOrEffectName;
	}

	DispatchPublicEntry(Entry);
	return Entry.LogIndex;
}

int32 UWildCombatLogComponent::LogCombat(const FString& AttackerName, int32 AttackerQ, int32 AttackerR, int32 AttackerOwnerID,
                                        const FString& DefenderName, int32 DefenderQ, int32 DefenderR, int32 DefenderOwnerID,
                                        int32 BattleQ, int32 BattleR)
{
	if (GetOwnerRole() != ROLE_Authority) return -1;

	FWildCombatLogEntry Entry;
	Entry.LogType = EWildCombatLogType::Combat;
	Entry.SourceUnitName = AttackerName;
	Entry.SourceQ = AttackerQ;
	Entry.SourceR = AttackerR;
	Entry.SourceUnitOwnerID = AttackerOwnerID;

	Entry.TargetUnitName = DefenderName;
	Entry.TargetQ = DefenderQ;
	Entry.TargetR = DefenderR;
	Entry.TargetUnitOwnerID = DefenderOwnerID;

	Entry.BattleQ = BattleQ;
	Entry.BattleR = BattleR;

	DispatchPublicEntry(Entry);
	return Entry.LogIndex;
}

int32 UWildCombatLogComponent::LogActionFailed(int32 ParentIndex, const FString& Reason)
{
	if (GetOwnerRole() != ROLE_Authority) return -1;

	FWildCombatLogEntry Entry;
	Entry.LogType = EWildCombatLogType::ActionFailed;
	Entry.ParentLogIndex = (ParentIndex > 0) ? ParentIndex : LastPublicPhaseLogIndex;
	Entry.ExtraMessage = Reason;
	DispatchPublicEntry(Entry);
	return Entry.LogIndex;
}

void UWildCombatLogComponent::AddLogEntryOnClient(const FWildCombatLogEntry& Entry)
{
	LogHistory.Add(Entry);
	OnCombatLogAdded.Broadcast(Entry);
}

void UWildCombatLogComponent::Multicast_AddLogEntry_Implementation(const FWildCombatLogEntry& NewEntry)
{
	AddLogEntryOnClient(NewEntry);
}

void UWildCombatLogComponent::Client_AddPrivateLogEntry_Implementation(APlayerController* TargetPC, const FWildCombatLogEntry& NewEntry)
{
	AddLogEntryOnClient(NewEntry);
}
