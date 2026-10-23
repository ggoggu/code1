# Rule: Turn-Based Card Game System Architecture

## 1. Core State Ownership
- **AGameModeBase / AGameMode**: Server-only. Governs match lifecycle, turn timeouts, deck generation, and win/loss arbitration.
- **AGameStateBase**: Replicated to all clients. Holds public match info (current turn player, turn phase, public board zones, graveyard count).
- **APlayerState**: Holds player-specific persistent state (current mana, max mana, score, deck count, hand count).
- **APlayerController**: Handles player input dispatching and acts as the network connection endpoint for RPCs. Does NOT directly manage UI layout coordinates, Slate construction, or cosmetic notice widgets.

## 2. Card Execution Pipeline
Every card play action MUST follow this sequence strictly on the Server:
1. **Request**: Client sends `Server_RequestPlayCard(CardInstanceID, TargetData)`.
2. **Verification**: Server checks:
   - Is it the requesting player's active turn?
   - Is the current turn phase allowing actions?
   - Does the player have the card in their hand?
   - Does the player have sufficient resources/mana?
   - Are the target parameters valid?
3. **Execution**: Deduct cost -> Move card from Hand to Active Zone/Graveyard -> Trigger Card Effect.
4. **Replication**: State changes replicate automatically via Fast Array or Replicated Properties.
5. **Cosmetics**: Server fires cosmetic multicast or triggers client visual queue.

## 3. Seeded RNG & Determinism
- Any deck shuffling, random discards, or RNG effects must utilize a server-controlled random stream (`FRandomStream`) with a synced match seed.

## 4. Extensibility & Polymorphism (OCP)
- `ChainComponent`, `PhaseComponent`, `GameState` 등 중앙 관리 컴포넌트에서 특정 카드나 스킬 클래스를 다운캐스팅(`IsA<UMageSearchLibraryTacticalSkill>()`)하여 특수 분기하는 것을 금지한다.
- 카드/스킬 해결에 사용자 비동기 선택(타겟팅, 카드 선택 등)이 필요한 경우, `UTacticalSkillBase` 또는 공용 인터페이스의 가상 함수(`RequiresClientPrompt()`, `ExecuteAction()`)로 위임하여 처리한다.
