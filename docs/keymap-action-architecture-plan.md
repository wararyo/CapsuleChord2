# キーマップのアクション化計画 — キーマップを「コード」から「データ」へ

作成日: 2026-07-07

このドキュメントは、キーマップを QMK Firmware のキーマップのような
「物理キー → アクション」の**編集可能なデータ**に発展させるための目標アーキテクチャと、
段階的な移行計画をまとめたものです。

feature/custom-key-settings での変換/具現化2層リファクタリング
（`Scale::getDiatonicDegree()` / `Scale::realizeChord()` / `ChordKeyInput::resolveKey()` の導入）を
出発点とし、`docs/architecture-improvement-plan.md`（以下「改善計画」）との整合を前提に設計している。

---

## 動機

1. **KeyMap クラスの実質的な違いがテーブルだけになった。**
   2層リファクタリング後、`KantanChordKeyMap` と `CapsuleChordKeyMap` の差分は
   `numberKeyMap` の値のみ。クラスの違いがデータの違いに縮退したなら、
   キーマップそのものをデータにするのが自然な次の一手。
2. **将来、使わないキーを任意の動作に割り当てられるようにしたい。**
   例: BlackAdder キーをフラットナインスに変える、LT をドラムアプリの有効/無効トグルにする、など。
   「決められたアクションの語彙があり、その配列を設定画面で編集できる」形にすれば、
   自由度とソースコードの一貫性を両立できる。
3. **改善計画との噛み合わせが良い**（後述）。修飾キーのポーリング排除・押下時コマンドキャッシュ・
   スレッド安全性の集約が、この設計で同時に達成される。

---

## 目標アーキテクチャ

### KeyAction: アクションの語彙

```cpp
// trivially copyable にしておく（設定保存にも、将来の PerfEngine キューにも載せられる）
struct KeyAction {
    enum class Type : uint8_t {
        None            = 0,
        PlayDegree      = 1,  // param = ダイアトニック番号 0-6
        PlayCustomChord = 2,  // param = カスタムキースロット 0/1
        Modifier        = 3,  // param = ModifierId（下記）
        ToggleApp       = 4,  // param = アプリID
        // 将来: TransposeUp/Down, TapTempo, ...
        // 注意: 永続化されるため、値は列挙の順序ではなく明示的な数値で固定し、
        //       追加は末尾に、削除は欠番として扱う（詰め直さない）。
    } type = Type::None;
    uint8_t param = 0;
};

// ModifierId の例（同じく明示的な数値で固定する）
// Minor, Seventh(diatonic), FifthFlat, Aug, Sus4, Ninth, FlatNinth,
// Thirteenth, PitchUp, PitchDown, InversionUp, InversionDown, BlackAdder, ...
```

### キーマップ = ただの配列

```cpp
KeyAction keymap[KEY_COUNT];  // 物理キー（keyCode）でインデックス
```

- Kantan / CapsuleChord といった従来の「KeyMap クラス」は、**プリセット配列**に格下げされる。
- キーマップ切り替え（現在は Home 長押し）は配列の差し替えになる。
- Stage 2 でこの配列を設定として永続化し、メニューから編集可能にする。

### 実行側の分解: Mapper と Interpreter

| 役割 | 状態 | 責務 |
|---|---|---|
| **KeyMapper** | なし | keyCode → KeyAction の引き当てのみ。購読も発音もしない |
| **KeyActionInterpreter** | あり | `KeyEventListener` として購読し、アクションを実行する状態機械 |

Interpreter が持つ状態と挙動:

- **heldModifiers**: `Modifier` アクションのキー押下/解放で更新される修飾状態のビットセット。
  `PlayDegree` 押下時はこれを参照して DegreeChord を組み立てる
  （現在の `Keypad[KEY_RIGHT_x].isPressed()` ポーリング10連発を置き換える。
  isPressed 自体もイベント由来の状態なので意味的に等価）。
- **押下時アクションキャッシュ**: キー押下時に解決したアクション（と発音した Chord）を
  keyCode ごとに記録し、**release はキャッシュから処理する**。
  押下中にキーマップや設定が変わっても press/release の対が崩れない。
  改善計画 §3.3（KANTAN の押下時コマンドキャッシュ）がカスタムキー実装に対して
  明示的に要求している方式そのもの。現在の `currentPressingButton` はこの原始形。
- **音楽コアの利用**: `PlayDegree` の処理は
  「`Scale::getDiatonicDegree()` → heldModifiers を度数空間 Modifier テンプレートで適用 →
  `Scale::realizeChord()` → 発音」。2層リファクタリングで作った部品がそのまま使われる。

### ChordSink: 出力先の差し替え

```cpp
struct ChordSink {
    virtual void onChordResolved(const DegreeChord&) = 0;  // 具現化前の度数コード
    virtual void onChordOn(const Chord&) = 0;
    virtual void onChordOff() = 0;
};
```

- 通常演奏時のシンクは Pipeline へ発音する実装。
- `DegreeChordInputDialog` は「鳴らす代わりに DegreeChord を捕獲するシンク」を一時的に挿す。
  - これにより、ダイアログの入力配置が**ユーザーが使用中のキーマップに自動追従**する
    （現在は Kantan 固定 + `KantanChordKeyMap::numberKeyMap` 直参照という歪みがある）。

### スレッド安全性の集約

アクション実行が Interpreter の switch 文1箇所に集まるため、
アクション種別ごとの実行スレッドの振り分けをそこで一度だけ書けばよい。

- 演奏系（PlayDegree / PlayCustomChord / Modifier）→ Pipeline
  （改善計画フェーズ1.1 導入後は `Perf.post()` に置換。呼び出し箇所は1つ）
- UI/システム系（ToggleApp など）→ I2C タスクで実行してはいけない。
  メインループへのキュー投函（改善計画フェーズ2.1 の knock キューと同型）で配送する。

「リスナーを増やすたびにスレッド安全性を個別検討する」現構造からの脱却であり、
改善計画の規律1（データと通知でつなぎ、制御を渡さない）に沿う。

### 溶けて消えるもの / 残るもの

| 現在のコード | 行き先 |
|---|---|
| `KantanChordKeyMap` / `CapsuleChordKeyMap` | プリセット KeyAction 配列 |
| `ChordKeyInput::resolveKey()` / `KeyInputResult` | Interpreter の内部処理に吸収 |
| `numberKeyMap` テーブル | KeyAction 配列に変換 |
| `Scale::getDiatonicDegree()` / `realizeChord()` | そのまま利用（音楽コア） |
| 度数空間 Modifier テンプレート (`Modifier.h`) | そのまま利用（音楽コア） |
| `CustomKeyAssignment`（設定型） | そのまま利用（PlayCustomChord が参照） |
| `currentPressingButton` | 押下時アクションキャッシュに発展 |

---

## 改善計画（architecture-improvement-plan.md）との整合

- **フェーズ1.1（PerformanceEngine）**: Interpreter が Pipeline を叩く箇所は1つなので、
  `Perf.post()` への置換は1行で済む。
- **フェーズ1.2（キー解決の PerfEngine タスク移動）**: 修飾キーが heldModifiers という
  イベント駆動の状態になるため、キー解決に必要な情報がすべてイベント列だけから導出できる。
  Interpreter の駆動タスクを差し替えるだけで移行が成立する。
  改善計画側も「カスタムキー設定の実装スケジュールと合わせて判断する」としており、
  **Stage 3 = フェーズ1.2 として合流させるのが効率的**。
- **§3.3（押下時コマンドキャッシュ）**: Interpreter の設計に最初から織り込む。

---

## 段階的な移行計画

各段階は単独で価値を持ち、次の段階と矛盾しない。

### Stage 0（小・先行実施）: KeyMap クラスの統合 【実施済み 2026-07-08】

- `KantanChordKeyMap` + `CapsuleChordKeyMap` → `ChordKeyMap` 1クラスに統合し、
  `numberKeyMap` テーブルをコンストラクタ/setter で差し替え可能にする。
- `DegreeChordInputDialog` の `KantanChordKeyMap::numberKeyMap` 直参照を解消し、
  「現在のテーブル」を渡す形にする。
- `KeyMap::getAvailableKeyMaps()` はプリセットテーブルを持った `ChordKeyMap` を2つ返すだけになる。
- 統合された `ChordKeyMap` が Stage 1 の Interpreter の前身になる。

### Stage 1（中）: アクション化

- `KeyAction` 語彙と `KeyActionInterpreter` を導入。プリセットはハードコードされた KeyAction 配列。
- 修飾キーを heldModifiers 状態に移行（`Keypad[..].isPressed()` ポーリングの排除）。
- 押下時アクションキャッシュを導入。
- ChordSink を導入し、DegreeChordInputDialog を捕獲シンク方式に移行。
- この時点では設定UIなし・ユーザーから見た挙動は現状と同一。
- **着手タイミング: 「キー割り当て機能」がロードマップに載ったとき。**
  それより早くやると使われない抽象を抱えることになる。

### Stage 2（中）: 設定化

- KeyAction 配列を `/settings/controls.json` に永続化。
- メニューに「キーを選ぶ → アクションを選ぶ」UIを追加。
- スキーマ設計の注意:
  - 最初から `version` フィールドを入れる。
  - アクション種別・ModifierId は列挙の順序ではなく**明示的な数値**で保存する
    （将来の追加・削除で既存設定が壊れないように）。
  - 未知のアクション値を読んだ場合は `None` にフォールバックする。

### Stage 3: 改善計画フェーズ1.2 と合流

- キーイベントを `Perf.post({Type::KeyInput, ...})` で PerfEngine タスクへ転送し、
  Interpreter の駆動をそちらへ移す。
- Stage 1 で修飾がイベント駆動化されていれば、ここはほぼ配線変更のみ。

---

## 将来の拡張アイデア（スコープ外・メモ）

- QMK でいうレイヤー（モーメンタリ/トグル）: 必要になるまで実装しない。
- tap/hold の区別（短押しと長押しで別アクション）: Home 長押しのキーマップ切り替えが既に類例。
- キーマップデータに基づく LED フィードバック（LedLayer との統合。
  改善計画フェーズ2.3 の LedLayer コマンドキュー化の後に検討）。
