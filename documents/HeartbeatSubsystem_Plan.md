# 心跳上报子系统（UMeetVRHeartbeatSubsystem）实施计划

## 1. Summary

新增一个 `UGameInstanceSubsystem` —— **`UMeetVRHeartbeatSubsystem`**，客户端与 Dedicated Server **共用同一实现**，周期性调用 MpyApiClient 已生成的 `POST /ue/heartbeat`（`MpyApi::OpenAPIUeApi::UeHeartbeatPost`），按身份自动选用 token（DS 用 `-dsToken`，客户端用 `-token`）。

放置位置：`Plugins/MeetVRTemplate/Source/MeetVRTemplate/GameModes/MVL_Backend/`，与现有 `MeetVR_MetricReportSubsystem` / `MeetVRNetworkMetricSubsystem` / `MeetVR_MVLServerSessionSubsystem` 同目录同风格。

## 2. Current State Analysis

### 后端接口已就绪（无需改）

* `Plugins/MeetVRLBE/Source/MpyApiClient/` 已由 swagger 生成心跳接口：

  * `Public/OpenAPIUeApi.h`：`UeHeartbeatPost(Request, Delegate)`、`FUeHeartbeatPostDelegate`、`SetURL()` / `AddHeaderParam()`。

  * `Public/OpenAPIUeApiOperations.h`：`OpenAPIUeApi::UeHeartbeatPostRequest { TOptional<OpenAPIHeartbeatRequest> OpenAPIHeartbeatRequest; }`。

  * `Public/OpenAPIHeartbeatRequest.h`：`TOptional<FString> ActorType; TOptional<int32> TeamId; TOptional<FString> Sn;`（全可选）。

* 当前除生成代码外，**没有任何消费者调用** **`UeHeartbeatPost`**（已全局检索确认）。

### 现有范式（新代码直接沿用）

* `MeetVR_MetricReportSubsystem`（GameInstanceSubsystem）：

  * `TryAutoInitialize()` 从 `UMVLAppGameInstanceSubsystem` 取 Host/Token，`UeApi.SetURL(Host)` + `UeApi.AddHeaderParam("Authorization", FString::Printf(TEXT("Bearer %s"), *Token))`，懒初始化、幂等。

  * 调用方范式：`Request.OpenAPIXxxRequest = Body; Api.XxxPost(Request, ...CreateLambda(...))`。

* `MeetVRNetworkMetricSubsystem`（GameInstanceSubsystem，仅 DS）：

  * `GI->GetTimerManager().SetTimer(Handle, this, &Fn, Interval, true)` 周期上报，启动时立即执行一次；`ShouldCreateSubsystem` 控制创建范围。

* `MeetVR_MVLServerSessionSubsystem` / `UMVLDsService`：DS 已有自己的 `OpenAPIUeApi UeApi`，但只做 ready / online / offline / validate，**无心跳**——本次不动它。

### 身份与参数来源

* `MVLAppCore.h` 的 `FMVLLaunchParams`：`Host / Token / DeviceSN / DsToken / TeamId / DsAddr`，由 `FMVLAppCoreModule::GetLaunchParams()`（MVLAppCore.cpp，解析 `-host/-token/-sn/-dsToken/-teamId/-dsAddr`）提供；`IsDSMode()` = `TeamId > 0 && !DsToken.IsEmpty()`。

* 客户端连接参数：`GameMode::InitGame` → `UMVLAppGameInstanceSubsystem::SetConnectionParams(Host, Token, DeviceSN)`，经 `GetMVLHostValue() / GetMVLTokenValue() / GetMVLDeviceSNValue()` 读取。

* MeetVRTemplate.Build.cs 已依赖 `MVLBackend / MVLAppCore / MpyApiClient / HTTP / Json`，**无需改 Build.cs**。

## 3. Proposed Changes

### 3.1 新建头文件

`Plugins/MeetVRTemplate/Source/MeetVRTemplate/Public/GameModes/MVL_Backend/MeetVRHeartbeatSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OpenAPIUeApi.h"
#include "MeetVRHeartbeatSubsystem.generated.h"

// 心跳上报配置
USTRUCT(BlueprintType)
struct FMeetVRHeartbeatConfig
{
    GENERATED_BODY()

    // 上报间隔（秒）。文档建议 5–10s，默认 10s（服务端在线阈值 30s）
    UPROPERTY(BlueprintReadWrite, Category = "MVL|Heartbeat",
              meta = (ClampMin = "1.0", ClampMax = "60.0"))
    float HeartbeatIntervalSeconds = 10.0f;
};

/**
 * 客户端 / DS 心跳上报子系统（共用同一接口 POST /ue/heartbeat）。
 *
 * 身份自动识别：
 * - DS（-teamId + -dsToken 均存在）：使用 DsToken，Body 带 actorType=ds + teamId；
 * - 客户端：使用 UMVLAppGameInstanceSubsystem 的 Host/Token/SN，Body 带 actorType=client + sn。
 *
 * 心跳纯展示用途：失败仅记日志不重试，不影响任何业务。
 */
UCLASS()
class MEETVRTEMPLATE_API UMeetVRHeartbeatSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** 启动周期性心跳上报 */
    UFUNCTION(BlueprintCallable, Category = "MVL|Heartbeat",
              meta = (WorldContext = "WorldContextObject"))
    void StartHeartbeat(UObject* WorldContextObject, const FMeetVRHeartbeatConfig& Config);

    /** 使用默认配置启动 */
    UFUNCTION(BlueprintCallable, Category = "MVL|Heartbeat",
              meta = (WorldContext = "WorldContextObject"))
    void StartHeartbeatDefault(UObject* WorldContextObject);

    /** 停止周期上报 */
    UFUNCTION(BlueprintCallable, Category = "MVL|Heartbeat")
    void StopHeartbeat();

    UFUNCTION(BlueprintPure, Category = "MVL|Heartbeat")
    bool IsHeartbeating() const { return bIsHeartbeating; }

    /** 立即上报一次（不受定时器影响；参数未就绪时静默跳过） */
    UFUNCTION(BlueprintCallable, Category = "MVL|Heartbeat",
              meta = (WorldContext = "WorldContextObject"))
    void HeartbeatOnce(UObject* WorldContextObject);

private:
    /** 懒初始化：按身份确定 Host / Token / TeamId / SN，幂等 */
    bool TryAutoInitialize();

    /** 发送一次心跳 */
    void SendHeartbeat();

private:
    FMeetVRHeartbeatConfig CurrentConfig;
    FTimerHandle HeartbeatTimerHandle;
    bool bIsHeartbeating = false;
    bool bIsInitialized = false;
    bool bIsDSMode = false;

    FString Host;
    FString AuthToken;
    int32 TeamId = 0;
    FString DeviceSN;

    MpyApi::OpenAPIUeApi UeApi;
};
```

### 3.2 新建实现文件

`Plugins/MeetVRTemplate/Source/MeetVRTemplate/Private/GameModes/MVL_Backend/MeetVRHeartbeatSubsystem.cpp`

* `Initialize()`：`Collection.InitializeDependency(UMVLAppGameInstanceSubsystem::StaticClass())`；调用 `StartHeartbeat(this, FMeetVRHeartbeatConfig())` 自动启动（仿 `MeetVRNetworkMetricSubsystem::Initialize`）。

* `Deinitialize()`：`StopHeartbeat()`；`bIsInitialized = false`。

* `StartHeartbeat(World, Config)`：

  * `Interval = FMath::Clamp(Config.HeartbeatIntervalSeconds, 1.0f, 60.0f)`；

  * `GI->GetTimerManager().SetTimer(HeartbeatTimerHandle, this, &UMeetVRHeartbeatSubsystem::SendHeartbeat, Interval, true)`；

  * `bIsHeartbeating = true`；立即 `SendHeartbeat()` 一次（不等首个周期）。

* `StopHeartbeat()`：`ClearTimer` + `bIsHeartbeating = false`。

* `HeartbeatOnce(World)`：直接 `SendHeartbeat()`。

* `TryAutoInitialize()`（幂等，仿 `MetricReportSubsystem`）：

  1. 若 `bIsInitialized` 直接返回 true。
  2. `const auto& Params = FMVLAppCoreModule::GetLaunchParams();`
  3. 若 `Params.IsDSMode()`：`bIsDSMode = true; Host = Params.Host; AuthToken = Params.DsToken; TeamId = Params.TeamId;`
  4. 否则：`bIsDSMode = false;` 取 `UMVLAppGameInstanceSubsystem` 的 `GetMVLHostValue() / GetMVLTokenValue() / GetMVLDeviceSNValue()`；`Host` 为空 → 返回 false（客户端参数未就绪，下个周期再试）。
  5. `UeApi.SetURL(Host); UeApi.AddHeaderParam("Authorization", FString::Printf(TEXT("Bearer %s"), *AuthToken)); bIsInitialized = true;`（token 为空时不加 Header，与 MetricReport 行为一致）。

* `SendHeartbeat()`：

  1. `if (!TryAutoInitialize()) return;`
  2. 组 Body：

     ```cpp
     MpyApi::OpenAPIHeartbeatRequest Body;
     Body.ActorType = bIsDSMode ? TEXT("ds") : TEXT("client");
     if (bIsDSMode) { Body.TeamId = TeamId; }
     else if (!DeviceSN.IsEmpty()) { Body.Sn = DeviceSN; }
     ```
  3. `MpyApi::OpenAPIUeApi::UeHeartbeatPostRequest Request; Request.OpenAPIHeartbeatRequest = Body;`
  4. `UeApi.UeHeartbeatPost(Request, ...CreateLambda(...))`：HTTP 200 → Log（含 actorType）；否则仅 Warning 日志（**不重试**，符合文档「心跳失败可忽略」）。

### 3.3 不改动项

* 不改 `MpyApiClient` 生成代码；

* 不改 `MVLAppCore / MVLBackend / MeetVR_MVLServerSessionSubsystem / UMVLDsService`；

* 不改 `MeetVRTemplate.Build.cs`（依赖已齐）。

## 4. Assumptions & Decisions

| 决策点      | 决定                                                            | 理由                                                   |
| -------- | ------------------------------------------------------------- | ---------------------------------------------------- |
| 子系统位置    | MeetVRTemplate 的 `GameModes/MVL_Backend/`                     | 客户端与 DS 同一二进制均加载该插件；已有 Metric/Network 上报子系统同目录，依赖已具备 |
| 子系统类型    | `UGameInstanceSubsystem`，默认所有端创建（不覆写 `ShouldCreateSubsystem`） | 客户端、DS 都需要                                           |
| 身份识别     | 以 `FMVLLaunchParams::IsDSMode()` 区分 DS / 客户端                  | 与文档「DS 用 DS token、客户端用客户端 token，服务端按 token 区分身份」一致   |
| 客户端 Body | `actorType=client` + `sn`，不带 teamId                           | token 已隐含队伍；teamId 可选                                |
| DS Body  | `actorType=ds` + `teamId`                                     | 便于前端展示                                               |
| 周期       | 默认 10s（可配置，clamp 1–60s）                                       | 文档建议 5–10s；在线阈值 30s，留 3 倍余量                          |
| 启动方式     | `Initialize` 自动启动定时器 + 懒初始化                                   | 客户端参数在 GameMode::InitGame 后才注入，懒初始化天然兼容；零额外接线        |
| 失败策略     | 仅日志，不重试                                                       | 文档明确「心跳失败可忽略、无副作用」                                   |

## 5. Verification

1. **编译**：Win64 Editor 与 Server 目标编译通过（UBT/VS 构建），`GetDiagnostics` 无错误。
2. **DS 运行验证**：以 `LiaoBoVRServer.exe -host=<host> -teamId=<n> -dsToken=<tok>` 启动，日志出现 `[Heartbeat] Sent (ds)`；服务端 `GET /team/{id}/server-status` 的 `dsHeartbeat` 持续更新。
3. **客户端运行验证**：以 `-host=<host> -token=<tok> -sn=<sn>` 启动，日志出现 `[Heartbeat] Sent (client)`；前端「启动队伍」页设备列表显示「在线（Ns 前）」。
4. **无参数启动**：日志无报错（静默跳过）。
5. **停止/断网**：停止心跳 30s 后，前端对应卡片显示「离线 / N/A」（可选验证，依赖前端与服务端）。

