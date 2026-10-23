# Effect & Tactical Skill UI 연출 개발 가이드라인
**AI 에이전트 협업 및 Visual Cue 연출 구현 표준 가이드북**

- **문서 번호:** WC-GDL-2026-10-EFFECT-01  
- **프로젝트:** WildCard (Unreal Engine 5.7)  
- **작성일:** 2026-10-14  
- **적용 대상:** 게임플레이 프로그래머, 테크니컬 아티스트, 시스템 기획자, AI 코딩 에이전트  

---

## 1. 개요 및 설계 원칙

본 가이드라인은 마법사 전술스킬 '서고 탐색(Search Library)'과 같이 **[플레이어 모달 입력(선택창)]**과 **[서버 로직 검증 및 실행]**, 그리고 **[광역 시각적 연출(UI 애니메이션/VFX/SFX)]**이 결합된 복합 기능을 개발할 때 따라야 하는 표준 아키텍처와 구현 절차를 정의합니다.

### 핵심 3대 원칙
1. **Zero-Touch Main HUD (바이너리 충돌 원천 차단):**  
   메인 HUD 에셋(`WBP_HUD.uasset`)에는 개별 연출 로직을 절대 직접 구현하지 않습니다. 메인 HUD는 GameplayTag에 따라 독립 위젯을 생성하고 띄워주는 **라우터(Router)** 역할만 수행합니다.
2. **독립 연출 위젯 (1 연출 = 1 WBP):**  
   모든 연출 UI는 `UWildVisualEffectWidget`을 상속받아 독립된 위젯 블루프린트로 제작되며, 연출 종료 시 스스로 `FinishVisualEffect()`를 호출하여 자가 소멸합니다.
3. **단일 NetMulticast 이벤트 버스:**  
   스킬/이펙트별로 별도의 NetMulticast RPC를 추가하지 않고, `AInGameState::Multicast_BroadcastVisualCue` 단일 통로를 공유합니다.

---

## 2. 4단계 표준 라이프사이클 (4-Phase Lifecycle)

서고 탐색과 같은 복합 UI 연출 기능은 반드시 아래 4단계로 역할을 분리하여 구현합니다.

```
[Phase 1: Local Input] ─────────► [Phase 2: Server Execution]
플레이어 모달 선택 창 활성화           서버 RPC 전송 및 유효성 검증
(FCardSelectionContext)              비즈니스 로직(덱 조작/드로우) 수행
                                                │
                                                ▼
[Phase 4: Client Presentation] ◄── [Phase 3: Visual Cue Broadcast]
독립 위젯 동적 스폰 및 연출             FWildVisualCueData 패킹 후
(UWildVisualEffectWidget)             단일 Multicast_BroadcastVisualCue 발송
```

### Phase 1: 로컬 입력 모달 단계 (Local Input Phase)
- **실행 위치:** 로컬 클라이언트 (`InitiateSkill` 또는 로컬 트리거)
- **동작:** 플레이어에게 선택지를 보여주는 모달 UI(예: 카드 3장 중 1장 선택)를 띄웁니다.
- **표준 구조:** 하드코딩된 컨트롤러 전용 함수를 만들지 말고, 범용 `FCardSelectionContext` 구조체를 구성하여 `PC->RequestCardSelection(Context)`를 호출합니다.

### Phase 2: 서버 검증 및 실행 단계 (Server Execution Phase)
- **실행 위치:** 전용 서버 (Dedicated Server)
- **동작:** 클라이언트가 선택한 결과를 Server RPC로 수신하여 권한 검사(자원 소모, 사거리, 인덱스 유효성) 후 `UEffectBase` 인스턴스의 `ApplyEffect()`를 실행합니다.

### Phase 3: 시각적 큐 브로드캐스트 단계 (Cue Broadcast Phase)
- **실행 위치:** 전용 서버
- **동작:** 이펙트 로직 수행 직후, `FWildVisualCueData`에 GameplayTag와 시전자, 대상, 수치 등을 패킹하여 `BroadcastVisualCue(Context, CueData)`를 호출합니다.
- **네트워크:** `AInGameState::Multicast_BroadcastVisualCue` (Unreliable)가 호출되어 모든 접속 클라이언트의 `OnVisualCueReceivedDelegate`로 전파됩니다.

### Phase 4: 독립 연출 위젯 재생 단계 (Presentation Phase)
- **실행 위치:** 각 클라이언트
- **동작:** 클라이언트의 `UWildHUDWidget`이 델리게이트를 수신하여 `VisualEffectWidgetMap`에서 태그를 매칭합니다. 매칭된 독립 위젯(`UWildVisualEffectWidget`)을 화면에 생성하고 `PlayVisualEffect(CueData)`를 호출합니다.
- **소멸:** 위젯 블루프린트 내 애니메이션이 끝나면 `FinishVisualEffect()`를 호출하여 스스로 화면에서 제거됩니다.

---

## 3. 실전 구현 워크플로우 (Step-by-Step Guide)

### Step 1: GameplayTag 정의
`Config/DefaultGameplayTags.ini` 또는 에디터의 프로젝트 세팅에서 시각 연출용 태그를 등록합니다.
- 전술스킬 연출: `Visual.TacticalSkill.{클래스명}.{스킬명}` (예: `Visual.TacticalSkill.Mage.SearchLibrary`)
- 카드 이펙트 연출: `Visual.Effect.{이펙트유형}` (예: `Visual.Effect.Damage`, `Visual.Effect.PeekHand`)

### Step 2: C++ Effect 또는 TacticalSkill 작성
이펙트 실행 시점에 `BroadcastVisualCue`를 호출하도록 작성합니다.

```cpp
// YourEffect.cpp
bool UYourEffect::ApplyEffect(const FEffectContext& Context)
{
    // 1. 서버 비즈니스 로직 수행 (상태 변경, 수치 계산 등)
    // ...

    // 2. Visual Cue 브로드캐스트
    FWildVisualCueData CueData;
    CueData.CueTag = FGameplayTag::RequestGameplayTag(TEXT("Visual.TacticalSkill.Mage.SearchLibrary"));
    CueData.Instigator = Context.SourceUnit;
    CueData.Target = Context.TargetUnit;
    CueData.Value = ResultValue;
    BroadcastVisualCue(Context, CueData);

    return true;
}
```

### Step 3: DataTable 등록
- 전술스킬인 경우 `DT_TacticalSkillData`에 해당 스킬 행을 추가하고 `VisualCueTag` 필드에 Step 1의 태그를 지정합니다.
- 카드 이펙트인 경우 `DT_CardData` 또는 이펙트 설정에 해당 태그를 지정합니다.

### Step 4: 독립 연출 위젯 블루프린트 제작
1. 콘텐츠 브라우저에서 `UWildVisualEffectWidget`을 부모 클래스로 하는 위젯 블루프린트 생성 (예: `WBP_VisualEffect_SearchLibrary`).
2. UMG 디자이너에서 원하는 시각 연출(카드 드로우 모션, 셔플 파티클, 사운드 등) 구성.
3. Event Graph에서 `Event PlayVisualEffect(CueData)`를 오버라이드하여 애니메이션 재생.
4. 애니메이션 종료 이벤트(OnAnimationFinished)에서 `FinishVisualEffect()`를 반드시 호출.

### Step 5: HUD 위젯 매핑 등록
`WBP_HUD`의 클래스 디폴트(Class Defaults) -> `Visual Effect Widget Map`에 [GameplayTag &rarr; 위젯 클래스] 쌍을 등록합니다. (코드 수정 전혀 없음)

---

## 4. [필수 첨부] AI 에이전트 사전 프롬프트 템플릿

> **💡 사용법:** AI 에이전트(Antigravity, Cursor, Claude 등)에게 새로운 이펙트나 UI 연출 구현을 요청할 때, **반드시 아래 블록을 프롬프트 맨 앞에 그대로 복사하여 첨부**하십시오.

```markdown
================================================================================
[WILD_CARD_ARCHITECTURE_CONTEXT: Effect & UI Presentation Pipeline]
반드시 다음 아키텍처 원칙과 가이드라인을 준수하여 구현하십시오:

1. 아키텍처 표준 (Zero-Touch HUD & Event Bus):
   - WBP_HUD.uasset 또는 UWildHUDWidget 내부에 개별 이펙트의 연출 로직을 직접 하드코딩하지 마십시오.
   - 모든 UI 연출은 UWildVisualEffectWidget을 상속받는 독립 위젯으로 동작해야 합니다.
   - 서버에서 클라이언트로의 시각 연출 통지는 개별 RPC를 추가하지 말고 AInGameState::Multicast_BroadcastVisualCue를 통해서만 발송하십시오.
   - UEffectBase를 상속받은 C++ 이펙트에서는 UEffectBase::BroadcastVisualCue 헬퍼 함수를 사용하여 큐를 브로드캐스트하십시오.

2. 데이터 구조체 및 태그 규약:
   - FWildVisualCueData (Source/WildCard/InGame/Data/BattleData.h)
     * CueTag: FGameplayTag (예: Visual.TacticalSkill.Mage.SearchLibrary, Visual.Effect.Damage 등)
     * Instigator / Target: TWeakObjectPtr<AActor>
     * WorldLocation: FVector
     * Value: int32 (수치 또는 인덱스)
     * ContextPayload: FName
   - 태그는 계층형 Fallback을 지원하므로 구체적 태그(Visual.Effect.Damage.Fire)가 없을 경우 상위 태그(Visual.Effect.Damage) 위젯이 자동 선택됩니다.

3. 모달 입력과 연출의 분리 (서고 탐색 패턴):
   - 유저의 선택이 필요한 기능은 [Phase 1: Local Modal Input] -> [Phase 2: Server RPC & Effect] -> [Phase 3: Visual Cue Multicast] -> [Phase 4: UWildVisualEffectWidget 연출]의 4단계를 엄격히 준수하십시오.
   - 플레이어 컨트롤러에 스킬 전용 함수를 추가하지 말고, FCardSelectionContext와 PC->RequestCardSelection()을 재사용하십시오.

4. 컴파일 및 빌드 가드레일:
   - C++ 수정 후 반드시 WildCardEditor 타깃 컴파일을 검증하고, 오류 발생 시 자가 수정을 수행하십시오.
   - 엔진 소스(UE_5.7/Engine/)를 수정하지 마십시오.
================================================================================
```

---

## 5. 작업자 요구사항 작성 템플릿 및 실전 예시

### 5.1 요구사항 작성 템플릿 (작업자 작성 양식)

AI 에이전트에게 사전 프롬프트를 첨부한 후, 아래 양식의 빈칸을 채워 요구사항을 전달하십시오.

```markdown
# [기능명] 신규 이펙트 및 UI 연출 구현 요청

### 1. 기본 정보
- 기능 유형: (카드 이펙트 / 전술스킬 / 타일 상호작용 중 선택)
- 명칭: [예: 화염구 / 적 패 훔쳐보기 / 서고 탐색]
- 시각 연출 태그: `Visual.TacticalSkill.[Class].[SkillName]` 또는 `Visual.Effect.[EffectName]`

### 2. Phase 1: 로컬 입력 모달 (필요한 경우에만 작성, 선택창 없으면 "없음")
- 모달 유형: [예: 카드 선택 모달 (CardSelectionModal) / 타깃 지정 모달]
- 표시 조건 및 전달 컨텍스트: [예: 내 덱 상위 3장 표시, 1장 선택]
- 입력 취소 가능 여부: [예: 가능 / 불가]

### 3. Phase 2: 서버 비즈니스 로직 (Dedicated Server)
- 실행 조건 및 검증: [예: 마나 3 소모 검증, 사거리 검증]
- Effect 클래스명: [예: UFireballEffect / UPeekEnemyHandEffect]
- 상태 변경 내용: [예: 상대방 손패 1장을 무덤으로 이동, 대상 유닛 HP 5 감소]

### 4. Phase 3 & 4: Visual Cue 및 연출 위젯 (Presentation)
- FWildVisualCueData 페이로드:
  * Value: [예: 입힌 피해량 / 선택된 카드 ID]
  * Instigator & Target: [예: 시전자 유닛 -> 피격 유닛]
- 연출 위젯 명칭: `WBP_VisualEffect_[기능명]`
- 연출 상세 내용: [예: 화면 중앙에 대상 카드가 나타난 후 불타며 사라지는 2D UMG 애니메이션 및 사운드]
```

---

### 5.2 실전 작성 예시: "적 패 훔쳐보기(Peek Enemy Hand)" 구현 요청

```markdown
[WILD_CARD_ARCHITECTURE_CONTEXT: Effect & UI Presentation Pipeline 첨부]

# 도적 전술스킬 [적 패 훔쳐보기 (Peek Enemy Hand)] 구현 요청

### 1. 기본 정보
- 기능 유형: 전술스킬 (Tactical Skill)
- 명칭: 적 패 훔쳐보기 (Peek Enemy Hand)
- 시각 연출 태그: `Visual.TacticalSkill.Rogue.PeekHand`

### 2. Phase 1: 로컬 입력 모달
- 모달 유형: 카드 선택 모달 (RequestCardSelection 재사용)
- 표시 조건: 스킬 발동 시 서버로부터 상대방의 현재 손패 카드 목록을 받아 모달에 팝업
- 플레이어 동작: 상대 패 중 버리게 만들 카드 1장 선택 후 확인

### 3. Phase 2: 서버 비즈니스 로직
- Effect 클래스: `UPeekEnemyHandEffect` (신규 생성)
- 검증: 기력 40 소모 검증, 상대방 손패가 1장 이상인지 검증
- 실행: 플레이어가 선택한 상대 손패 카드를 묘지로 이동시키고 상대 손패 배열 갱신

### 4. Phase 3 & 4: Visual Cue 및 연출 위젯
- CueData 구성:
  * CueTag: `Visual.TacticalSkill.Rogue.PeekHand`
  * Instigator: 시전자 유닛
  * Target: 상대방 영웅 유닛
  * Value: 버려진 카드의 CardID
- 연출 위젯: `WBP_VisualEffect_PeekHand` (UWildVisualEffectWidget 상속)
- 연출 내용:
  * 화면 중앙에 상대의 해당 카드가 뒤집혀 공개되는 UMG 애니메이션 재생
  * 암살자 단검 이펙트와 함께 카드가 베어지며 사라지는 연출 후 FinishVisualEffect() 호출
```

---

## 6. Git 협업 및 바이너리 충돌 방지 체크리스트

작업 완료 및 커밋 전 반드시 아래 체크리스트를 점검하십시오.

- [ ] **WBP_HUD 무결성:** `WBP_HUD.uasset`의 내부 노드나 그래프를 직접 수정하지 않았는가?
- [ ] **독립 위젯 상속:** 신규 연출 위젯이 반드시 `UWildVisualEffectWidget`을 상속받았는가?
- [ ] **소멸 보장:** 위젯 애니메이션 종료 시점에 `FinishVisualEffect()`가 누락 없이 호출되는가?
- [ ] **단일 RPC 준수:** 새로운 NetMulticast 함수를 만들지 않고 `Multicast_BroadcastVisualCue`를 사용했는가?
- [ ] **GameplayTag 등록:** 사용한 GameplayTag가 `DefaultGameplayTags.ini`에 정상 등록되었는가?
- [ ] **빌드 검증:** `WildCardEditor` Win64 Development 빌드가 오류 없이 통과하는가?
