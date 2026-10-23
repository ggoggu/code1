#include "InGame/UI/ViewModel/WildHUDViewModel.h"
#include "InGame/UI/ViewModel/WildCombatLogEntryViewModel.h"

void UWildHUDViewModel::UpdateMyHandCards(const TArray<UWildCardViewModel*>& InCards)
{
	MyHandCards = InCards;
	
	// 배열 전체가 변경되었음을 UI에 알림 (Hand Widget에서 카드들을 재정렬하도록 유도)
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(MyHandCards);
}

void UWildHUDViewModel::AddCombatLogEntryViewModel(UWildCombatLogEntryViewModel* InEntryVM)
{
	if (!InEntryVM) return;

	CombatLogEntries.Add(InEntryVM);
	SetCombatLogCount(CombatLogEntries.Num());
	SetLatestLogSummary(InEntryVM->TitleText);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CombatLogEntries);
}

void UWildHUDViewModel::ClearCombatLogEntryViewModels()
{
	CombatLogEntries.Empty();
	SetCombatLogCount(0);
	SetLatestLogSummary(FText::GetEmpty());
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CombatLogEntries);
}
