#include "InGame/Game/InGamePlayerState.h"
#include "Net/UnrealNetwork.h"
#include "InGameController.h"
#include "PhaseComponent.h"
#include "InGameMode.h"
#include "InGameState.h"
#include "PlayerStatComponent.h"
#include "WildCard.h"
#include "WildCardGameInstance.h"
#include "ArtifactBase.h"

AInGamePlayerState::AInGamePlayerState()
{
	bReplicates = true;
	NetUpdateFrequency = 10.0f;

	StatComponent = CreateDefaultSubobject<UPlayerStatComponent>(TEXT("StatComponent"));
	
	// [신규] 카드 로직 전담 컴포넌트 생성 및 네트워크 복제 설정
	CardComponent = CreateDefaultSubobject<UPlayerCardComponent>(TEXT("CardComponent"));
	CardComponent->SetIsReplicated(true);
}

void AInGamePlayerState::BeginPlay()
{
	Super::BeginPlay();
}

void AInGamePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AInGamePlayerState, PlayerCount);
	DOREPLIFETIME(AInGamePlayerState, MySpawnableTiles);

	DOREPLIFETIME(AInGamePlayerState, bIsGamesetReady);
	DOREPLIFETIME(AInGamePlayerState, bIsMulliganReady);
	DOREPLIFETIME(AInGamePlayerState, bIsActionReady);
	
	DOREPLIFETIME(AInGamePlayerState, ArtifactDisplay);
}

void AInGamePlayerState::OnRep_PlayerCount()
{
	if(GetPlayerController() == nullptr)
		return;

	UE_LOG(LogTemp, Warning, TEXT("Client | %s | PlayerCount now: %d"), *GetName(), PlayerCount);
}

void AInGamePlayerState::OnRep_MySpawnableTiles()
{
	WILD_SIMPLE(Warning);
}

// ==========================================================
// 멀리건 및 시작 데이터 처리
// ==========================================================

void AInGamePlayerState::Server_SubmitMulligan_Implementation(const TArray<int32>& CardsToSwap)
{
	UE_LOG(LogTemp, Warning, TEXT("[Server] Player %d 의 Server_SubmitMulligan RPC 수신 완료!"), GetPlayerId());

	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (!GameState || GameState->GetCurrentMatchState() != EWildMatchState::Mulligan) 
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] 멀리건 단계가 아니므로 요청을 무시합니다."));
		return;
	}

	if (CardsToSwap.Num() > 5)
	{
		UE_LOG(LogTemp, Error, TEXT("[Server] Player %d 가 비정상적인 개수(%d장)의 교체를 요청했습니다."), GetPlayerId(), CardsToSwap.Num());
		return;
	}

	// [수정] 교체할 카드 ID 목록을 컴포넌트에 안전하게 캐싱
	if (CardComponent)
	{
		CardComponent->CachedMulliganCards = CardsToSwap;
	}

	AInGameController* PC = Cast<AInGameController>(GetPlayerController());
	AInGameMode* GameMode = Cast<AInGameMode>(GetWorld()->GetAuthGameMode());
	
	if (PC && GameMode && GameMode->PhaseComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] Player %d 멀리건 데이터 캐싱 완료. 레디 상태로 전환합니다."), GetPlayerId());
		GameMode->PhaseComponent->ProcessPlayerMulliganReady(PC);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Server] 필요한 컴포넌트를 찾을 수 없어 멀리건 처리를 중단합니다."));
	}
}

bool AInGamePlayerState::Server_SubmitMulligan_Validate(const TArray<int32>& CardsToSwap)
{
	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (GameState && GameState->GetCurrentMatchState() != EWildMatchState::Mulligan)
	{
		return false;
	}
	return true;
}

void AInGamePlayerState::Client_ReceiveStartingHand_Implementation(const TArray<FWildCardDisplayData>& FinalHand)
{
	// 필요 시 추가 처리
}

// ==========================================================
// 비공개 메인 행동 (Action_Private) 상태 관리
// ==========================================================

void AInGamePlayerState::SetPendingMainAction(const FWildActionData& InActionData)
{
	if (HasAuthority())
	{
		PendingMainAction = InActionData;
		bIsActionReady = true; 

		UE_LOG(LogWildPhase, Log, TEXT("[Server] Player %d 의 비공개 행동이 성공적으로 캐싱되었습니다. ActionType: %d"), 
			GetPlayerId(), (int32)PendingMainAction.ActionType);
	}
}

const FWildActionData& AInGamePlayerState::GetPendingMainAction() const
{
	return PendingMainAction;
}

void AInGamePlayerState::ClearPendingMainAction()
{
	if (HasAuthority())
	{
		PendingMainAction = FWildActionData();
		bIsActionReady = false; 
	}
}

// ==========================================================
// 성유물 연동 로직
// ==========================================================

void AInGamePlayerState::Server_SubmitArtifact_Implementation(FName ArtifactRowName)
{
	UE_LOG(LogTemp, Warning, TEXT("[Server] Player %d 의 Server_SubmitArtifact RPC 수신! 선택 품목: %s"), GetPlayerId(), *ArtifactRowName.ToString());

	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (!GameState || GameState->GetCurrentMatchState() != EWildMatchState::Gameset) 
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] Gameset 단계가 아니므로 성유물 선택 요청을 무시합니다."));
		return;
	}

	UWildCardGameInstance* GI = Cast<UWildCardGameInstance>(GetGameInstance());
	AInGameMode* GameMode = Cast<AInGameMode>(GetWorld()->GetAuthGameMode());

	if (!GI || !GI->ArtifactDataTable || !GameMode)
	{
		UE_LOG(LogTemp, Error, TEXT("[Server] 성유물 생성을 위한 필수 시스템을 찾을 수 없습니다."));
		return;
	}

	FArtifactData* FoundArtifactInfo = GI->ArtifactDataTable->FindRow<FArtifactData>(ArtifactRowName, TEXT("GenerateArtifact"));
	if (!FoundArtifactInfo)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] %s 성유물을 데이터 테이블에서 찾을 수 없습니다."), *ArtifactRowName.ToString());
		return;
	}

	UArtifactBase* NewArtifact = NewObject<UArtifactBase>(this);
	if (NewArtifact)
	{
		NewArtifact->InitializeArtifact(
			ArtifactRowName, 
			FoundArtifactInfo->MaxDurability, 
			FoundArtifactInfo->EffectClasses
		);
		
		EquippedArtifact = NewArtifact;
		UE_LOG(LogTemp, Log, TEXT("[Server] Player %d 의 성유물 캐싱 완료 (비공개 상태)"), GetPlayerId());
	}
}

bool AInGamePlayerState::Server_SubmitArtifact_Validate(FName ArtifactRowName)
{
	AInGameState* GameState = GetWorld()->GetGameState<AInGameState>();
	if (GameState && GameState->GetCurrentMatchState() != EWildMatchState::Gameset)
	{
		return false;
	}
	return true;
}

void AInGamePlayerState::SyncArtifactToDisplay()
{
	if (!HasAuthority() || !EquippedArtifact) return;

	ArtifactDisplay = FWildArtifactDisplayData(
		EquippedArtifact->GetArtifactID(),
		EquippedArtifact->GetCurrentDurability(),
		EquippedArtifact->GetMaxDurability(),
		EquippedArtifact->IsArtifactActive()
	);
	
	ForceNetUpdate(); 
	UE_LOG(LogTemp, Warning, TEXT("[Server] Player %d 의 성유물 데이터가 클라이언트로 공개 동기화되었습니다!"), GetPlayerId());
}

void AInGamePlayerState::OnRep_ArtifactDisplay()
{
	UE_LOG(LogTemp, Log, TEXT("[Client] 성유물 정보 동기화 완료: %s (내구도: %d)"), 
		*ArtifactDisplay.ArtifactID.ToString(), ArtifactDisplay.CurrentDurability);
}