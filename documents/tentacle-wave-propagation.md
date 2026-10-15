# 触手波形传播方式改进计划

## Summary

将 ConnectorSpline 的正弦波从当前模式改为"行波传播"模式，使波形从 mother 端向 tentacle 端传播，类似抖绳子的效果。同时添加开关让用户可以在行波/驻波之间切换。

## Current State Analysis

当前 `UpdateSineSpline` 中的相位公式（[MeetVRTentacleConnector.cpp:324](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Private/Connector/MeetVRTentacleConnector.cpp#L324)）：

```cpp
const float Phase = 2.0f * PI * Periods * Alpha + SineSpeed * Time;
```

- `+ SineSpeed * Time`：波形从 tentacle 端向 mother 端传播（反方向）
- 配合 `Envelope = Alpha`（mother 端幅度=0），导致 mother 端呈直线、tentacle 端呈曲线，过渡不自然

## Proposed Changes

### 1. 添加波形模式枚举 — [MeetVRTentacleConnector.h](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Public/Connector/MeetVRTentacleConnector.h)

在 `Sine Wave Parameters` 区域新增：

```cpp
/** Wave propagation mode. */
UENUM(BlueprintType)
enum class EMeetVRWaveMode : uint8
{
    /** Traveling wave: waveform propagates from mother to tentacle (like a rope). */
    Traveling UMETA(DisplayName = "Traveling Wave (rope)"),
    /** Standing wave: all points oscillate in place. */
    Standing UMETA(DisplayName = "Standing Wave"),
};

/** Wave propagation mode. Traveling = wave moves from mother to tentacle (rope-like). */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sine")
EMeetVRWaveMode WaveMode = EMeetVRWaveMode::Traveling;
```

### 2. 修改相位公式 — [MeetVRTentacleConnector.cpp](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Private/Connector/MeetVRTentacleConnector.cpp#L324)

将固定公式改为根据 `WaveMode` 切换：

```cpp
// Traveling wave: phase = k*x - ω*t  (wave propagates from mother→tentacle)
// Standing wave:  phase = k*x + ω*t  (all points oscillate in place)
const float Phase = (WaveMode == EMeetVRWaveMode::Traveling)
    ? (2.0f * PI * Periods * Alpha - SineSpeed * Time)
    : (2.0f * PI * Periods * Alpha + SineSpeed * Time);
```

**关键点**：`-` 号让波峰随时间从 Alpha=0（mother）向 Alpha=1（tentacle）移动，实现"向外传递"。

### 3. 保留幅度包络

`Envelope = Alpha` 保持不变，确保 mother 端固定。行波模式下效果：
- mother 端（Alpha=0）：幅度=0，固定不动（波源）
- 波形从 mother 端产生，沿连线向 tentacle 端传播
- tentacle 端（Alpha=1）：幅度最大，波动最明显（波到达终点）

## Assumptions & Decisions

1. **默认模式为 Traveling**：用户明确要求"向外传递，类似绳索"，所以默认设为行波
2. **保留 Standing 模式**：通过枚举让用户可以切换回驻波，不丢失现有功能
3. **不改变幅度包络**：`Envelope = Alpha` 确保 mother 端固定，这是之前已确认的需求
4. **SineSpeed 同时控制振荡频率和传播速度**：在行波 `sin(kx - ωt)` 中，传播速度 `v = ω/k = SineSpeed × SineWaveLength / (2π)`，用户可通过调大 SineSpeed 加快传播

## Verification Steps

1. 编译模块：`Build.bat LiaoBoVREditor Win64 Development -module=MeetVRTemplate -WaitMutex`
2. 在编辑器中放置 Connector，设置 Mother/Tentacle endpoints
3. `WaveMode = Traveling`：观察波形是否从 mother 端向 tentacle 端传播（像抖绳子）
4. `WaveMode = Standing`：观察是否回到所有点同时上下振荡的效果
5. 调整 `SineSpeed`：确认传播速度随之变化
6. 配合 GuideSpline：确认行波在弯曲路径上也能正确传播
