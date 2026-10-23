// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BattleData.h"
#include "WildCombatLogComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatLogAddedSignature, const FWildCombatLogEntry&, NewEntry);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class WILDCARD_API UWildCombatLogComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWildCombatLogComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category = "CombatLog|Events")
	FOnCombatLogAddedSignature OnCombatLogAdded;

	// 클라이언트에서 로그 항목 직접 추가 및 브로드캐스트
	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	void AddLogEntryOnClient(const FWildCombatLogEntry& Entry);

	// 클라이언트에서 전체 로그 기록 확인
	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	const TArray<FWildCombatLogEntry>& GetLogHistory() const { return LogHistory; }

	// 클라이언트용 포맷터: Entry를 로컬 플레이어 시점에 맞게 타이틀과 상세 줄들로 변환
	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	static void FormatLogEntry(const FWildCombatLogEntry& Entry, int32 LocalPlayerID, int32 FirstPlayerID, FString& OutTitle, TArray<FString>& OutDetails);

	// 타일 좌표 포맷팅 유틸리티: (Q, R, OwnerID) -> "(D,7,R)" 또는 "(F,12)"
	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	static FString FormatTileCoord(int32 Q, int32 R, int32 UnitOwnerID = -1, int32 FirstPlayerID = -1);

	// 직업명 한글 변환
	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	static FString GetPlayerClassName(EWildPlayerClass InClass);

	// 카드명 테이블 조회 (Display Name)
	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	static FString GetCardDisplayName(const UWorld* World, FName CardID);

	// 성유물 명칭 테이블 조회
	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	static FString GetArtifactDisplayName(const UWorld* World, FName ArtifactID);

	// 전술 스킬 명칭 테이블 조회
	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	static FString GetTacticalSkillDisplayName(const UWorld* World, FName SkillID);

	// =========================================================================
	// Server Logging API (Authority Only)
	// =========================================================================
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "CombatLog|Server")
	int32 LogTurnPriority(int32 FirstPlayerID);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "CombatLog|Server")
	int32 LogGameset(int32 P1ID, EWildPlayerClass P1Class, int32 P2ID, EWildPlayerClass P2Class);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "CombatLog|Server")
	int32 LogArtifactSelect();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "CombatLog|Server")
	int32 LogArtifactAndSkillReveal(int32 P1ID, const FString& P1Artifact, const FString& P1Skill, int32 P2ID, const FString& P2Artifact, const FString& P2Skill);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "CombatLog|Server")
	int32 LogMulligan();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "CombatLog|Server")
	int32 LogTurnStart(int32 P1ID, const FString& P1CardName, int32 P2ID, const FString& P2CardName);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "CombatLog|Server")
	int32 LogForceCharge(int32 PlayerID, int32 ChargedAmount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "CombatLog|Server")
	int32 LogPhasePrivate();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "CombatLog|Server")
	int32 LogPhasePublic(int32 P1ID, bool bP1NoAction, const FString& P1CardName, EWildActionType P1ActionType, int32 P1ForceBefore, int32 P1ForceAfter, int32 P1TargetQ, int32 P1TargetR,
	                     int32 P2ID, bool bP2NoAction, const FString& P2CardName, EWildActionType P2ActionType, int32 P2ForceBefore, int32 P2ForceAfter, int32 P2TargetQ, int32 P2TargetR);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "CombatLog|Server")
	int32 LogActionResolve(int32 ParentIndex, EWildActionType InActionType, int32 TargetQ, int32 TargetR, const FString& ResultDesc);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "CombatLog|Server")
	int32 LogTurnEnd();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "CombatLog|Server")
	int32 LogChainCast(int32 ChainStackCount, int32 CasterPlayerID, const FString& CardOrEffectName, int32 BeforeForce, int32 AfterForce, int32 TargetQ, int32 TargetR, const FString& TargetUnitName, int32 TargetUnitOwnerID, bool bIsFromHand = true);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "CombatLog|Server")
	int32 LogCombat(const FString& AttackerName, int32 AttackerQ, int32 AttackerR, int32 AttackerOwnerID,
	                const FString& DefenderName, int32 DefenderQ, int32 DefenderR, int32 DefenderOwnerID,
	                int32 BattleQ, int32 BattleR);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "CombatLog|Server")
	int32 LogActionFailed(int32 ParentIndex, const FString& Reason = TEXT(""));

	// 최근 메인 행동 공개 로그 번호 캐싱
	int32 GetLastPublicPhaseLogIndex() const { return LastPublicPhaseLogIndex; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CombatLog")
	TArray<FWildCombatLogEntry> LogHistory;

	// 전체 클라이언트 브로드캐스트
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_AddLogEntry(const FWildCombatLogEntry& NewEntry);

	// 특정 클라이언트(플레이어) 전용 개인 로그
	UFUNCTION(Client, Reliable)
	void Client_AddPrivateLogEntry(class APlayerController* TargetPC, const FWildCombatLogEntry& NewEntry);

private:
	int32 NextLogIndex = 1;
	int32 LastPublicPhaseLogIndex = -1;

	int32 GenerateLogIndex() { return NextLogIndex++; }
	void DispatchPublicEntry(FWildCombatLogEntry& Entry);
};
