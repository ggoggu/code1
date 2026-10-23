#include "InGame/Unit/TrapBase.h"
#include "InGame/Unit/UnitBase.h"
#include "WildCard.h"

// Sets default values
ATrapBase::ATrapBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ATrapBase::BeginPlay()
{
	Super::BeginPlay();
	
	// 마우스 클릭 트레이스 채널(ECC_MouseClick)을 무시하여 아래의 타일이 클릭되도록 처리 (통합 함수 사용)
	AUnitBase::DisableActorMouseClickCollision(this);
}

// Called every frame
void ATrapBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

