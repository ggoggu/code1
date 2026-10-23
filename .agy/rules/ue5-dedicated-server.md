# Rule: Unreal Engine 5.7 Dedicated Server & Networking Standard

## 1. Network Topology & Authority
- **Single Source of Truth**: The Dedicated Server (`NM_DedicatedServer`) is the sole authority for all game state mutations.
- **Authority Checks**: Always verify `HasAuthority()` or `GetNetMode() == NM_DedicatedServer` before executing critical gameplay mutations (mana reduction, card draw, damage, phase transitions).
- **Client Execution**: Clients are strictly thin display layers. Clients NEVER directly modify cards, health, mana, or turn states.

## 2. Remote Procedure Calls (RPCs)
- **Validation Scope**: All player intent must pass through `Server` RPCs with `WithValidation` (`Server_PlayCard_Validate`, `Server_PlayCard_Implementation`). `WithValidation`은 Client -> Server RPC에만 적용하며, Server -> Client (`Client`, `NetMulticast`) RPC에는 `WithValidation`을 사용하지 않는다.
- **Data Validation**: Reject any client request containing invalid card IDs, unowned cards, insufficient mana, or illegal targeting.
- **Client-Side Preview vs Server Authority**: 이동 가능 타일 하이라이트 등 단순 UI 피드백/프리뷰는 복제된 맵 데이터를 바탕으로 클라이언트 로컬에서 즉각 계산한다. 단순 조회 목적으로 매번 Server RPC -> Client RPC 왕복 질의를 발생시키지 않는다.
- **Multicast / Client RPCs**: Use `Client` RPCs for player-specific feedback (e.g., UI error prompts, private hand updates) and `NetMulticast` (with `Unreliable` where appropriate) only for cosmetic cues (VFX, audio, board animations).

## 3. Replication Best Practices
- **Property Replication**: Register replicated properties in `GetLifetimeReplicatedProps`.
- **Bandwidth Optimization**: Use `DOREPLIFETIME_CONDITION` with `COND_OwnerOnly` for private data (e.g., player's hidden hand cards) and `COND_SkipOwner` or default for public board states.
- **OnRep Callbacks**: `OnRep_` functions must only handle visual presentation, UI updates, and local audio on clients. Never run game logic inside `OnRep_`.

## 4. Unreal Engine 5.7 C++ Conventions
- `#include "FileName.generated.h"` must ALWAYS be the final `#include` statement in any header.
- Use `TObjectPtr<T>` for UObject member pointers instead of raw pointers.
- Strictly use UE containers (`TArray`, `TMap`, `TSet`, `FString`, `FName`, `FText`).
- Follow standard UE prefixes: `A` (Actor), `U` (UObject/Component), `F` (Struct), `E` (Enum), `I` (Interface).
- 타일 상태, 인터랙션 모드, 게임 단계 등에는 원시 정수(`int32`) 리터럴 대신 반드시 `UENUM()`으로 정의된 강타입 Enum을 사용한다.
