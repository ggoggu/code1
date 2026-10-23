// Fill out your copyright notice in the Description page of Project Settings.

#include "WildLocalMatchmakingProvider.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"

FWildLocalMatchmakingProvider::FWildLocalMatchmakingProvider()
{
}

FWildLocalMatchmakingProvider::~FWildLocalMatchmakingProvider()
{
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
}

void FWildLocalMatchmakingProvider::Initialize()
{
	const FString Directory = FPaths::ProjectSavedDir() / TEXT("Matchmaking");
	IFileManager::Get().MakeDirectory(*Directory, true);
}

FString FWildLocalMatchmakingProvider::GetQueueFilePath() const
{
	return FPaths::ProjectSavedDir() / TEXT("Matchmaking/LocalMatchQueue.json");
}

void FWildLocalMatchmakingProvider::StartMatchmaking(const FWildMatchmakingRequest& Request,
                                                     const FOnMatchmakingStatusChanged& StatusCallback,
                                                     const FOnMatchmakingCompleted& CompleteCallback)
{
	CurrentRequest = Request;
	OnStatusChanged = StatusCallback;
	OnCompleted = CompleteCallback;

	ElapsedSeconds = 0.0f;
	TimeoutSeconds = FMath::Max(3.0f, Request.TimeoutSeconds);
	bIsSearching = true;

	// 1. 내 티켓을 JSON 파일에 등록
	FScopeLock Lock(&FileLock);
	TArray<TSharedPtr<FJsonValue>> Tickets;
	ReadQueueFile(Tickets);
	CleanExpiredTickets(Tickets);

	TSharedPtr<FJsonObject> MyTicketObj = MakeShared<FJsonObject>();
	MyTicketObj->SetStringField(TEXT("ticket_id"), CurrentRequest.TicketId);
	MyTicketObj->SetStringField(TEXT("username"), CurrentRequest.Username);
	MyTicketObj->SetStringField(TEXT("deck_name"), CurrentRequest.SelectedDeck.DeckName);
	MyTicketObj->SetNumberField(TEXT("deck_class"), static_cast<int32>(CurrentRequest.SelectedDeck.DeckClass));
	MyTicketObj->SetStringField(TEXT("tactical_skill_id"), CurrentRequest.SelectedDeck.SelectedTacticalSkillID.ToString());
	MyTicketObj->SetStringField(TEXT("status"), TEXT("Searching"));
	MyTicketObj->SetStringField(TEXT("created_time"), FDateTime::UtcNow().ToIso8601());
	MyTicketObj->SetStringField(TEXT("opponent_name"), TEXT(""));
	MyTicketObj->SetNumberField(TEXT("opponent_class"), 0);
	MyTicketObj->SetStringField(TEXT("opponent_skill_id"), TEXT(""));
	MyTicketObj->SetBoolField(TEXT("opponent_is_bot"), false);
	MyTicketObj->SetBoolField(TEXT("is_host"), false);
	MyTicketObj->SetStringField(TEXT("server_address"), TEXT(""));

	Tickets.Add(MakeShared<FJsonValueObject>(MyTicketObj));
	WriteQueueFile(Tickets);

	OnStatusChanged.ExecuteIfBound(EWildMatchmakingState::Searching);

	// 2. 주기적 틱 등록 (0.5초 주기)
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
	}
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateRaw(this, &FWildLocalMatchmakingProvider::Tick), 0.5f);

	UE_LOG(LogTemp, Log, TEXT("[WildLocalMatchmaking] 매칭 큐 등록 완료. Ticket: %s, User: %s (봇 타임아웃: %.1f초)"),
		*CurrentRequest.TicketId, *CurrentRequest.Username, TimeoutSeconds);
}

void FWildLocalMatchmakingProvider::CancelMatchmaking(const FString& TicketId, const FOnMatchmakingCancelled& Callback)
{
	bIsSearching = false;

	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}

	RemoveTicketFromFile(TicketId);

	OnStatusChanged.ExecuteIfBound(EWildMatchmakingState::Cancelled);
	Callback.ExecuteIfBound();

	UE_LOG(LogTemp, Log, TEXT("[WildLocalMatchmaking] 매칭 취소됨. Ticket: %s"), *TicketId);
}

bool FWildLocalMatchmakingProvider::Tick(float DeltaTime)
{
	if (!bIsSearching)
	{
		return false;
	}

	ElapsedSeconds += DeltaTime;

	FScopeLock Lock(&FileLock);
	TArray<TSharedPtr<FJsonValue>> Tickets;
	ReadQueueFile(Tickets);
	CleanExpiredTickets(Tickets);

	TSharedPtr<FJsonObject> MyTicket;
	int32 MyIndex = INDEX_NONE;

	for (int32 i = 0; i < Tickets.Num(); ++i)
	{
		TSharedPtr<FJsonObject> Obj = Tickets[i]->AsObject();
		if (Obj.IsValid() && Obj->GetStringField(TEXT("ticket_id")) == CurrentRequest.TicketId)
		{
			MyTicket = Obj;
			MyIndex = i;
			break;
		}
	}

	// 1. 이미 다른 클라이언트에 의해 매칭(Matched)된 경우
	if (MyTicket.IsValid() && MyTicket->GetStringField(TEXT("status")) == TEXT("Matched"))
	{
		FWildMatchResult Result;
		Result.TicketId = CurrentRequest.TicketId;
		Result.MatchId = MyTicket->HasField(TEXT("match_id")) ? MyTicket->GetStringField(TEXT("match_id")) : FGuid::NewGuid().ToString();
		Result.MyDeckData = CurrentRequest.SelectedDeck;
		Result.bIsHost = MyTicket->GetBoolField(TEXT("is_host"));
		Result.ServerAddress = MyTicket->GetStringField(TEXT("server_address"));

		Result.Opponent.PlayerName = MyTicket->GetStringField(TEXT("opponent_name"));
		Result.Opponent.PlayerClass = static_cast<EWildPlayerClass>(MyTicket->GetIntegerField(TEXT("opponent_class")));
		Result.Opponent.TacticalSkillID = FName(*MyTicket->GetStringField(TEXT("opponent_skill_id")));
		Result.Opponent.bIsBot = MyTicket->GetBoolField(TEXT("opponent_is_bot"));

		Tickets.RemoveAt(MyIndex);
		WriteQueueFile(Tickets);

		bIsSearching = false;
		OnStatusChanged.ExecuteIfBound(EWildMatchmakingState::MatchFound);
		OnCompleted.ExecuteIfBound(true, Result, TEXT(""));

		UE_LOG(LogTemp, Log, TEXT("[WildLocalMatchmaking] 상대 플레이어에 의해 매칭 성사! 상대: %s (직업: %d, 호스트: %d, MatchID: %s)"),
			*Result.Opponent.PlayerName, static_cast<int32>(Result.Opponent.PlayerClass), Result.bIsHost, *Result.MatchId);
		return false;
	}

	// 2. 다른 대기 중인 실제 플레이어 탐색
	TSharedPtr<FJsonObject> OpponentTicket;
	int32 OpponentIndex = INDEX_NONE;

	for (int32 i = 0; i < Tickets.Num(); ++i)
	{
		if (i == MyIndex) continue;
		TSharedPtr<FJsonObject> Obj = Tickets[i]->AsObject();
		if (Obj.IsValid() && Obj->GetStringField(TEXT("status")) == TEXT("Searching")
			&& Obj->GetStringField(TEXT("username")) != CurrentRequest.Username)
		{
			OpponentTicket = Obj;
			OpponentIndex = i;
			break;
		}
	}

	if (OpponentTicket.IsValid())
	{
		// 2인 매칭 성사!
		const FString OpponentName = OpponentTicket->GetStringField(TEXT("username"));
		const EWildPlayerClass OpponentClass = static_cast<EWildPlayerClass>(OpponentTicket->GetIntegerField(TEXT("deck_class")));
		const FName OpponentSkill = FName(*OpponentTicket->GetStringField(TEXT("tactical_skill_id")));
		const FString GeneratedMatchId = FGuid::NewGuid().ToString();

		// 상대 티켓을 Matched로 갱신 (상대는 Client로 접속)
		OpponentTicket->SetStringField(TEXT("status"), TEXT("Matched"));
		OpponentTicket->SetStringField(TEXT("match_id"), GeneratedMatchId);
		OpponentTicket->SetStringField(TEXT("opponent_name"), CurrentRequest.Username);
		OpponentTicket->SetNumberField(TEXT("opponent_class"), static_cast<int32>(CurrentRequest.SelectedDeck.DeckClass));
		OpponentTicket->SetStringField(TEXT("opponent_skill_id"), CurrentRequest.SelectedDeck.SelectedTacticalSkillID.ToString());
		OpponentTicket->SetBoolField(TEXT("opponent_is_bot"), false);
		OpponentTicket->SetBoolField(TEXT("is_host"), false);
		OpponentTicket->SetStringField(TEXT("server_address"), TEXT("127.0.0.1:7777"));

		// 내 티켓은 파일에서 제거
		if (MyIndex != INDEX_NONE)
		{
			Tickets.RemoveAt(MyIndex);
		}
		WriteQueueFile(Tickets);

		FWildMatchResult Result;
		Result.TicketId = CurrentRequest.TicketId;
		Result.MatchId = GeneratedMatchId;
		Result.MyDeckData = CurrentRequest.SelectedDeck;
		Result.bIsHost = false;
		Result.ServerAddress = TEXT("127.0.0.1:7777");
		Result.Opponent = FWildOpponentInfo(OpponentName, OpponentClass, OpponentSkill, false);

		bIsSearching = false;
		OnStatusChanged.ExecuteIfBound(EWildMatchmakingState::MatchFound);
		OnCompleted.ExecuteIfBound(true, Result, TEXT(""));

		UE_LOG(LogTemp, Log, TEXT("[WildLocalMatchmaking] 2인 매칭 성사! 둘 다 게스트(127.0.0.1:7777)로 전용 서버 접속. 상대: %s (직업: %d)"),
			*OpponentName, static_cast<int32>(OpponentClass));
		return false;
	}

	// 3. 타임아웃 판정 (봇 폴백)
	if (ElapsedSeconds >= TimeoutSeconds)
	{
		if (MyIndex != INDEX_NONE)
		{
			Tickets.RemoveAt(MyIndex);
			WriteQueueFile(Tickets);
		}

		FWildMatchResult BotResult;
		BotResult.TicketId = CurrentRequest.TicketId;
		BotResult.MatchId = FGuid::NewGuid().ToString();
		BotResult.MyDeckData = CurrentRequest.SelectedDeck;
		BotResult.bIsHost = true;
		BotResult.ServerAddress = TEXT("start_test");
		BotResult.Opponent = GenerateRandomBotOpponent();

		bIsSearching = false;
		OnStatusChanged.ExecuteIfBound(EWildMatchmakingState::MatchFound);
		OnCompleted.ExecuteIfBound(true, BotResult, TEXT(""));

		UE_LOG(LogTemp, Log, TEXT("[WildLocalMatchmaking] 타임아웃(%.1f초) 만료. 봇 매칭 생성: %s (직업: %d)"),
			ElapsedSeconds, *BotResult.Opponent.PlayerName, static_cast<int32>(BotResult.Opponent.PlayerClass));
		return false;
	}

	return true; // 계속 틱 수행
}

FWildOpponentInfo FWildLocalMatchmakingProvider::GenerateRandomBotOpponent() const
{
	const int32 RandClassInt = FMath::RandRange(1, 3);
	EWildPlayerClass BotClass = static_cast<EWildPlayerClass>(RandClassInt);

	FString BotName;
	FName BotSkill;

	switch (BotClass)
	{
	case EWildPlayerClass::Warrior:
		BotName = TEXT("수습 전사 가렌 (봇)");
		BotSkill = FName(TEXT("Skill_Warrior_Strike"));
		break;
	case EWildPlayerClass::Mage:
		BotName = TEXT("비전 마법사 제이나 (봇)");
		BotSkill = FName(TEXT("Skill_Mage_Intellect"));
		break;
	case EWildPlayerClass::Taoist:
		BotName = TEXT("신비 도사 일리단 (봇)");
		BotSkill = FName(TEXT("Skill_Taoist_Heal"));
		break;
	default:
		BotName = TEXT("모험가 도전자 (봇)");
		BotSkill = NAME_None;
		break;
	}

	return FWildOpponentInfo(BotName, BotClass, BotSkill, true);
}

void FWildLocalMatchmakingProvider::ReadQueueFile(TArray<TSharedPtr<FJsonValue>>& OutTickets)
{
	OutTickets.Empty();

	const FString FilePath = GetQueueFilePath();
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *FilePath) || JsonContent.TrimStartAndEnd().IsEmpty())
	{
		return;
	}

	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
	if (FJsonSerializer::Deserialize(Reader, RootObject) && RootObject.IsValid())
	{
		const TArray<TSharedPtr<FJsonValue>>* FoundArray = nullptr;
		if (RootObject->TryGetArrayField(TEXT("Tickets"), FoundArray) && FoundArray)
		{
			OutTickets = *FoundArray;
		}
	}
}

void FWildLocalMatchmakingProvider::WriteQueueFile(const TArray<TSharedPtr<FJsonValue>>& InTickets)
{
	TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetArrayField(TEXT("Tickets"), InTickets);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	if (FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer))
	{
		FFileHelper::SaveStringToFile(OutputString, *GetQueueFilePath(), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}
}

void FWildLocalMatchmakingProvider::CleanExpiredTickets(TArray<TSharedPtr<FJsonValue>>& InOutTickets)
{
	const FDateTime Now = FDateTime::UtcNow();

	InOutTickets.RemoveAll([&Now](const TSharedPtr<FJsonValue>& Value)
	{
		if (!Value.IsValid()) return true;
		TSharedPtr<FJsonObject> Obj = Value->AsObject();
		if (!Obj.IsValid()) return true;

		FString TimeStr = Obj->GetStringField(TEXT("created_time"));
		FDateTime CreatedTime;
		if (FDateTime::ParseIso8601(*TimeStr, CreatedTime))
		{
			if ((Now - CreatedTime).GetTotalSeconds() > 30.0)
			{
				return true; // 30초 이상 된 만료 티켓 삭제
			}
		}
		return false;
	});
}

void FWildLocalMatchmakingProvider::RemoveTicketFromFile(const FString& InTicketId)
{
	FScopeLock Lock(&FileLock);
	TArray<TSharedPtr<FJsonValue>> Tickets;
	ReadQueueFile(Tickets);

	Tickets.RemoveAll([&InTicketId](const TSharedPtr<FJsonValue>& Value)
	{
		if (!Value.IsValid()) return true;
		TSharedPtr<FJsonObject> Obj = Value->AsObject();
		return Obj.IsValid() && Obj->GetStringField(TEXT("ticket_id")) == InTicketId;
	});

	WriteQueueFile(Tickets);
}
