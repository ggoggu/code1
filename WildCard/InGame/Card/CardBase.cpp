#include "InGame/Card/CardBase.h"

UCardBase::UCardBase()
	: InstanceID(-1)
	, CardID(NAME_None)
	, CurrentManaCost(0)
	, CurrentForce(0)
	, CurrentIntelligence(0)
	, CurrentAttack(0)
	, CurrentHealth(0)
{
}

void UCardBase::InitializeCard(int32 InInstanceID, FName InCardID, int32 InCurrentManaCost, int32 InCurrentForce,
	int32 InCurrentIntelligence, int32 InCurrentAttack, int32 InCurrentHealth)
{
	InstanceID = InInstanceID;
	CardID = InCardID;
	CurrentManaCost = InCurrentManaCost;
	CurrentForce = InCurrentForce;
	CurrentIntelligence = InCurrentIntelligence;
	CurrentAttack = InCurrentAttack;
	CurrentHealth = InCurrentHealth;
}
