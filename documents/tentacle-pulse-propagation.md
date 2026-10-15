# 触手波包传播改进计划

## Summary

当前行波模式虽然公式正确（`sin(kx - ωt)`），但由于使用固定幅度包络 `Envelope = Alpha`，所有点持续振荡，波峰移动被掩盖，用户看不到"波在传递"的效果。

改为**移动波包**方案：一个有限宽度的高斯波包从 mother 端向 tentacle 端传播，波包范围内的点有波动，范围外的点保持直线。这样能产生明显的"绳索传递"视觉效果。

## Current State Analysis

当前 [MeetVRTentacleConnector.cpp:317-331](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Private/Connector/MeetVRTentacleConnector.cpp#L317-L331)：

```cpp
const float Phase = (WaveMode == EMeetVRWaveMode::Traveling)
    ? (2.0f * PI * Periods * Alpha - SineSpeed * Time)   // 行波
    : (2.0f * PI * Periods * Alpha + SineSpeed * Time);   // 驻波
const float SineValue = FMath::Sin(Phase);
const float Envelope = Alpha; // 固定包络：0→1 线性
Point += CurrentWaveDir * (SineValue * SineAmplitude * Envelope);
```

**问题**：固定包络 `Alpha` 让所有点持续振荡。行波的波峰虽然在移动，但整条线同时在波动，传播效果被掩盖。

## Proposed Changes

### 1. 新增波包参数 — [MeetVRTentacleConnector.h](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Public/Connector/MeetVRTentacleConnector.h)

在 `Sine Wave Parameters` 区域，`SineSpeed` 之后新增：

```cpp
/** Width of the traveling pulse (fraction of total length, 0.1-1.0).
 *  Only used in Traveling mode. Smaller = tighter pulse. */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sine", meta = (ClampMin = "0.1", ClampMax = "1.0"))
float PulseWidth = 0.3f;

/** Propagation speed of the pulse (cycles per second).
 *  Only used in Traveling mode. 1.0 = one full mother→tentacle pass per second. */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sine", meta = (ClampMin = "0.05"))
float PropagationSpeed = 0.5f;
```

### 2. 行波模式改用移动高斯波包 — [MeetVRTentacleConnector.cpp](file:///f:/LiaoBo_VR/Plugins/MeetVRTemplate/Source/MeetVRTemplate/Private/Connector/MeetVRTentacleConnector.cpp#L317-L331)

将当前的位移计算改为根据 `WaveMode` 分支：

```cpp
{
    const FVector CurrentWaveDir = ComputeWaveDir(Tangent, SineDirection);

    float Displacement;
    if (WaveMode == EMeetVRWaveMode::Traveling)
    {
        // --- Traveling pulse (rope-like) ---
        // A Gaussian envelope travels from mother(Alpha=0) to tentacle(Alpha=1).
        // PulseCenter cycles: -PulseWidth → 1+PulseWidth, then repeats.
        const float CycleLen = 1.0f + 2.0f * PulseWidth;
        const float PulseCenter = FMath::Fmod(Time * PropagationSpeed, CycleLen) - PulseWidth;

        // Gaussian envelope: peak at PulseCenter, fades to ~0 at ±PulseWidth
        const float Sigma = PulseWidth * 0.5f;
        const float GaussEnv = FMath::Exp(-FMath::Square((Alpha - PulseCenter) / Sigma));

        // Multiply by Alpha so mother endpoint stays fixed
        const float Envelope = GaussEnv * Alpha;

        // Phase: traveling sine inside the pulse
        const float Phase = 2.0f * PI * Periods * Alpha - SineSpeed * Time;
        const float SineValue = FMath::Sin(Phase);

        Displacement = SineValue * SineAmplitude * Envelope;
    }
    else
    {
        // --- Standing wave (all points oscillate in place) ---
        const float Phase = 2.0f * PI * Periods * Alpha + SineSpeed * Time;
        const float SineValue = FMath::Sin(Phase);
        const float Envelope = Alpha;
        Displacement = SineValue * SineAmplitude * Envelope;
    }

    Point += CurrentWaveDir * Displacement;
}
```

### 关键原理

| 概念 | 说明 |
|------|------|
| **PulseCenter** | 波包中心位置，从 `-PulseWidth`（mother 端外）移动到 `1+PulseWidth`（tentacle 端外），然后循环 |
| **GaussEnv** | 高斯函数，波包中心处=1，边缘衰减到≈0。波包范围外的点保持直线 |
| **Alpha 乘法** | 确保 mother 端（Alpha=0）幅度始终为 0，固定不动 |
| **CycleLen** | `1 + 2*PulseWidth`，让波包完全离开 tentacle 端后再从 mother 端重新进入 |

### 视觉效果

- **波包进入**：波包从 mother 端"进入"连线，mother 端附近开始出现波动
- **波包传播**：波动沿连线向 tentacle 端移动，波包经过的地方波动，未到的地方保持直线
- **波包离开**：波包到达 tentacle 端后"离开"，tentacle 端波动消失
- **循环**：波包从 mother 端重新进入，周而复始

## Assumptions & Decisions

1. **PropagationSpeed 含义**：每秒波包从 mother 到 tentacle 的循环次数。0.5 = 每 2 秒一次
2. **PulseWidth 含义**：波包占连线长度的比例。0.3 = 波包覆盖 30% 的连线长度
3. **SineSpeed 在行波模式下仍控制振荡频率**：波包内部的正弦波频率
4. **驻波模式不受影响**：保持原有固定包络逻辑
5. **高斯包络而非矩形窗**：高斯让波包边缘平滑衰减，避免突变

## Verification Steps

1. 编译：`Build.bat LiaoBoVREditor Win64 Development -module=MeetVRTemplate -WaitMutex`
2. `WaveMode = Traveling`：观察是否有明显的波包从 mother 端传到 tentacle 端
3. 调整 `PropagationSpeed`：确认传播速度随之变化（0.5→1.0 更快）
4. 调整 `PulseWidth`：确认波包大小变化（0.2 更紧凑，0.5 更宽松）
5. `WaveMode = Standing`：确认回到所有点同时振荡的效果
6. 配合 GuideSpline：确认波包在弯曲路径上也能正确传播
