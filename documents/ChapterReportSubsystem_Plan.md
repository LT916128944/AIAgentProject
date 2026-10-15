# 章节汇报蓝图接口（UMeetVRChapterReportSubsystem）实施计划

## 1. Summary

在现有基础上（沿用刚实现的 `UMeetVRHeartbeatSubsystem` 范式），新增一个 **DS 专用** 的 `UGameInstanceSubsystem` —— **`UMeetVRChapterReportSubsystem`**，向蓝图暴露章节汇报接口（`POST /ue/chapter/report`，`MpyApi::OpenAPIUeApi::UeChapterReportPost`）。蓝图只需传入 `ChapterId` / `TotalChapters` 与开始/结束语义，`TeamId` 与 DS token 由子系统从启动参数自动获取。

放置位置：`Plugins/MeetVRTemplate/Source/MeetVRTemplate/GameModes/MVL_Backend/`，与 `MeetVRHeartbeatSubsystem` / `MeetVRNetworkMetricSubsystem` 同目录同风格。

## 2. Current State Analysis

### 后端接口已就绪（无需改）
- `OpenAPIUeApi.h`：已生成 `UeChapterReportPost(Request, Delegate)`、`FUeChapterReportPostDelegate`、`SetURL()` / `AddHeaderParam()`。
- `OpenAPIUeApiOperations.h`：`OpenAPIUeApi::UeChapterReportPostRequest { TOptional<OpenAPIChapterReportRequest> OpenAPIChapterReportRequest; }`；响应 `UeChapterReportPostResponse { OpenAPIBooleanResponseDto Content; }`。
- `OpenAPIChapterReportRequest.h`：必填字段 `int32 TeamId; int32 ChapterId; int32 TotalChapters; FString EventType;`（非 Optional）。
- 当前除生成代码外，**无任何消费者调用 `UeChapterReportPost`**（已检索确认）。

### 现有范式（直接沿用）
- `UMeetVRHeartbeatSubsystem`（本次会话刚实现）：`UGameInstanceSubsystem` + `MpyApi::OpenAPIUeApi` 成员 + `TryAutoInitialize()` 懒初始化（`FMVLAppCoreModule::GetLaunchParams()` 取 Host/Token）+ 调用范式 `Request.OpenAPIXxxRequest = Body; Api.XxxPost(Request, ...CreateLambda(...))`。
- `UMeetVRNetworkMetricSubsystem`：DS 专用子系统的写法 —— `ShouldCreateSubsystem` 返回 `GameInstance->IsDedicatedServerInstance()`。
- 启动参数：`FMVLLaunchParams{ Host, Token, DeviceSN, DsToken, TeamId, DsAddr }`，`IsDSMode()` = `TeamId>0 && !DsToken.IsEmpty()`；DS 的 teamId 与 dsToken 由平台下发，天然匹配（docs 要求 teamId 必须与 DS token 所属队伍一致）。
- 游戏侧已有章节概念：`UMeetVR_MVLGameFlowComponent::ResolveMapByChapterId(int32)`，说明章节由游戏流程驱动 —— 汇报时机由蓝图在章节开始/结束时调用本接口。

### 依赖
- `MeetVRTemplate.Build.cs` 已依赖 `MpyApiClient / MVLAppCore / MVLBackend / HTTP / Json`，**无需改 Build.cs**。

## 3. Proposed Changes

### 3.1 新建头文件
`Plugins/MeetVRTemplate/Source/MeetVRTemplate/Public/GameModes/MVL_Backend/MeetVRChapterReportSubsystem.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OpenAPIUeApi.h"
#include "MeetVRChapterReportSubsystem.generated.h"

/**
 * DS 章节汇报子系统（仅 Dedicated Server 创建）。
 *
 * 包装 POST /ue/chapter/report：
 * - 每章开始 / 结束各上报一次（前端显示「章节 N/M」与状态标签；
 *   最后一章结束触发平台自动解散倒计时）。
 * - TeamId 与 DS token 自动取自启动参数（-teamId / -dsToken / -host），
 *   蓝图只需传入 ChapterId / TotalChapters 与开始/结束语义。
 *
 * 章节汇报关系到前端展示与自动解散，失败会 Error 日志提示（蓝图可自行重试）。
 */
UCLASS()
class MEETVRTEMPLATE_API UMeetVRChapterReportSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    /** 上报本章开始 */
    UFUNCTION(BlueprintCallable, Category = "MVL|Chapter")
    void ReportChapterStart(int32 ChapterId, int32 TotalChapters);

    /** 上报本章结束 */
    UFUNCTION(BlueprintCallable, Category = "MVL|Chapter")
    void ReportChapterEnd(int32 ChapterId, int32 TotalChapters);

private:
    /** 懒初始化：从启动参数取 Host / DsToken / TeamId，幂等 */
    bool TryAutoInitialize();

    /** 发送一次章节汇报（bStarted=true 表示 start，false 表示 end） */
    void SendChapterReport(int32 ChapterId, int32 TotalChapters, bool bStarted);

private:
    bool bIsInitialized = false;

    FString Host;
    FString AuthToken;
    int32 TeamId = 0;

    MpyApi::OpenAPIUeApi UeApi;
};
```

### 3.2 新建实现文件
`Plugins/MeetVRTemplate/Source/MeetVRTemplate/Private/GameModes/MVL_Backend/MeetVRChapterReportSubsystem.cpp`

- include：`GameModes/MVL_Backend/MeetVRChapterReportSubsystem.h`、`OpenAPIUeApiOperations.h`、`OpenAPIChapterReportRequest.h`、`MVLAppCore.h`、`Engine/GameInstance.h`。
- `Initialize()`：`UE_LOG` 初始化日志（无需依赖注入，DS 模式下自身具备全部参数来源）。
- `Deinitialize()`：`bIsInitialized = false`。
- `ShouldCreateSubsystem(UObject* Outer)`：`UGameInstance* GI = Cast<UGameInstance>(Outer); return GI && GI->IsDedicatedServerInstance();`（仿 `UMeetVRNetworkMetricSubsystem`）。
- `TryAutoInitialize()`（幂等，仿 Heartbeat）：
  1. `bIsInitialized` 已 true → return true。
  2. `const auto& Params = FMVLAppCoreModule::GetLaunchParams();`
  3. `if (!Params.IsDSMode()) return false;`（非 DS 直接跳过，客户端不创建本子系统）
  4. `Host = Params.Host; AuthToken = Params.DsToken; TeamId = Params.TeamId;`
  5. `Host` 为空 → Warning 并 return false。
  6. `UeApi.SetURL(Host); UeApi.AddHeaderParam("Authorization", FString::Printf(TEXT("Bearer %s"), *AuthToken)); bIsInitialized = true;` + Log。
- `ReportChapterStart(ChapterId, TotalChapters)` → `SendChapterReport(ChapterId, TotalChapters, true)`。
- `ReportChapterEnd(ChapterId, TotalChapters)` → `SendChapterReport(ChapterId, TotalChapters, false)`。
- `SendChapterReport(ChapterId, TotalChapters, bStarted)`：
  1. 参数校验（docs 约束，防 HTTP 400）：`ChapterId < 1 || TotalChapters < 1 || ChapterId > TotalChapters` → Warning + return。
  2. `if (!TryAutoInitialize()) return;`
  3. 组 Body：
     ```cpp
     MpyApi::OpenAPIChapterReportRequest Body;
     Body.TeamId        = TeamId;
     Body.ChapterId     = ChapterId;
     Body.TotalChapters = TotalChapters;
     Body.EventType     = bStarted ? TEXT("start") : TEXT("end");
     ```
  4. `MpyApi::OpenAPIUeApi::UeChapterReportPostRequest Request; Request.OpenAPIChapterReportRequest = Body;`
  5. `UeApi.UeChapterReportPost(Request, FUeChapterReportPostDelegate::CreateLambda([ChapterId, bStarted](const UeChapterReportPostResponse& Response){ ... }))`：HTTP 200 → Log；失败 → **Error** 日志（章节汇报影响前端展示与自动解散，需醒目提示；不内置重试，蓝图可视需要重新调用）。

### 3.3 不改动项
- 不改 `MpyApiClient` 生成代码、`MeetVRHeartbeatSubsystem`、`MeetVR_MVLServerSessionSubsystem` / `UMVLDsService`、`MeetVRTemplate.Build.cs`。

## 4. Assumptions & Decisions

### 为何用 Subsystem（初始化/生命周期考量，用户明确要求）
1. **异步回调安全**：生成的 `UeChapterReportPost` 内部用 `BindRaw(this, ...)` 绑定 `UeApi` 成员地址。若做成静态函数库，`UeApi` 只能是文件级静态对象，存在静态析构顺序风险与进程退出时回调悬垂的可能；而 Subsystem 把 `UeApi` 作为成员，生命周期由 `UGameInstance` 统一管理，天然规避该问题。
2. **初始化时序可控**：`Initialize()/Deinitialize()` 提供明确生命周期钩子；`TryAutoInitialize()` 懒初始化保证「DS 启动参数未就绪时静默跳过、就绪后自动生效」，与 `MeetVRHeartbeatSubsystem` / `MeetVR_MetricReportSubsystem` 一致。
3. **状态归属清晰**：token/Host/TeamId 缓存为子系统成员，多端各持实例，无静态全局状态污染。

| 决策点 | 决定 | 理由 |
| --- | --- | --- |
| 承载形式 | 独立子系统 `UMeetVRChapterReportSubsystem`，而非并入 Heartbeat | 章节汇报仅 DS、需明确语义与失败提示；Heartbeat 是客户端+DS 通用通道，职责不同 |
| 创建范围 | `ShouldCreateSubsystem` 仅 `IsDedicatedServerInstance()` | 与 `MeetVRNetworkMetricSubsystem` 一致；客户端永远不调用章节汇报 |
| 蓝图接口 | 两个明确函数：`ReportChapterStart` / `ReportChapterEnd`（参数 ChapterId + TotalChapters） | 比单个 bool 参数更清晰、防误用；对应 docs 的 eventType start/end |
| TeamId 来源 | 自动取自 `FMVLLaunchParams::TeamId`，蓝图不传 | 保证与 DS token 队伍一致（docs 硬性要求），蓝图零负担 |
| 失败处理 | Error 日志，不内置自动重试 | docs 建议重试但「给蓝图用」保持纯接口；蓝图可在失败回调后自行重调。日志级别高于心跳（重要性不同） |
| 参数校验 | 客户端侧先校验 ChapterId/TotalChapters 合法性 | 防 HTTP 400，与 docs 约束一致 |

## 5. Verification

> 注：编译验证暂不考虑（用户明确要求），仅保留运行时验证项。

1. **DS 运行验证**：以 `LiaoBoVRServer.exe -host=<host> -teamId=<n> -dsToken=<tok>` 启动，蓝图在章节开始调用 `ReportChapterStart(3, 5)` → 日志出现 `[ChapterReport] Reported: team=<n> chapter=3/5 event=start`；运营前端「启动队伍」页出现 `章节 3/5` 与状态标签。
2. **最后一章结束**：`ReportChapterEnd(5, 5)` → 前端出现「Ns 后解散队伍」倒计时（服务端 `Chapter:AutoDisbandDelaySeconds`，缺省 60s）。
3. **非法参数**：`ReportChapterStart(6, 5)` → Warning 日志，不发请求。
4. **客户端**：子系统不创建（`ShouldCreateSubsystem` 返回 false），蓝图获取为 null，属预期。
