# 以Java项目为例的Harness Engineering从0到1开发示例

## docs/ 目录：结构化知识库

```text
docs/
├── architecture/
│   ├── overview.md
│   ├── boundaries.md
│   └── data-flow.md
│
├── conventions/
│   ├── README.md
│   ├── naming.md
│   ├── error-handling.md
│   ├── testing.md
│   └── logging.md
│
├── design/
│   ├── feature-auth.md
│   └── feature-search.md
```

## 需求：基于 Harness Engineering 从0到1开发一个Java项目

技术栈：Spring Boot 2.7.x + Java 1.8 + Maven 3.6.3

| 组件 | 版本 | 说明 |
| --- | --- | --- |
| JDK | 1.8 | 不使用 record / var / text blocks 等 Java 9+ 语法 |
| Spring Boot | 2.7.x（如 2.7.18） | 最后一个支持 Java 8 的 LTS 主版本 |
| Spring Framework | 5.3.x | 由 Spring Boot 2.7 传递依赖 |
| Maven | 3.6.3 | 由 maven-enforcer-plugin 强制约束 |
| JUnit | 5.x（Jupiter） | 兼容 Java 8 |
| ArchUnit | 1.3.0 | 运行时要求 Java 8+ |
| Checkstyle | 9.3 | **最后一个支持 Java 8 运行的版本**，10.x 起要求 Java 11 |
| SpotBugs | 4.7.x | 兼容 Java 8 |
| JaCoCo | 0.8.11 | 兼容 Java 8 字节码 |

## 学习资源

| 资源 | 类型 | 适合谁 | 链接 |
| --- | --- | --- | --- |
| deusyu/harness-engineering | GitHub 学习指南 | 从零入门 | GitHub |
| harness-engineering.ai | 知识图谱（883 实体） | 快速了解生态全貌 | Website |
| OpenAI 原文 | 博客 | 理解原始思想 | OpenAI |

每个文档的头部都加上元信息：

```yaml
---
last_updated: 2026-03-28
status: active    # active | deprecated | draft
owner: @zhangsan
---
```

这样 doc-gardening Agent 可以自动扫描过期文档。

## 正确做法（直接用）

```markdown
# AGENTS.md

## 项目简介
[一句话] 这是一个面向中小企业的在线项目管理平台，基于 Spring Boot 2.7 + Java 1.8 + MySQL 5.7。

## 技术栈基线（不允许擅自升级）
- JDK: 1.8，不可使用 Java 9+ 语法（record/var/text blocks）
- Spring Boot: 2.7.x，不可升 3.x（Spring 6 要求 JDK 17）
- Maven: 3.6.3，由 enforcer 强制
- 数据库: MySQL 5.7（utf8mb4），不可升 8.x（生产环境为 5.7）
- 持久化: MyBatis-Plus 3.5.x（基于 MyBatis 3.5）+ Flyway（含 flyway-mysql 子模块），不引入 JP？

## 快速导航
| 你想做什么 | 去哪里看 |
|------------|----------|
| 了解系统架构 | docs/architecture/overview.md |
| 了解模块边界和依赖规则 | docs/architecture/boundaries.md |
| 了解编码规范 | docs/conventions/README.md |
| 了解当前迭代任务 | docs/plans/current-sprint.md |
```

```text
├── design/
│   ├── feature-auth.md
│   ├── feature-search.md
│   └── feature-billing.md
│
├── plans/
│   ├── current-sprint.md
│   └── backlog.md
│
└── reference/
    ├── api-spec.yaml
    └── error-codes.md
```

```markdown
| 了解 API 规范 | docs/reference/api-spec.yaml |
| 了解错误码 | docs/reference/error-codes.md |
| 了解测试规范 | docs/conventions/testing.md |

## 硬性规则（必须遵守，CI 会验证）
1. 依赖方向：domain → config → mapper → service → controller
2. 横切关注点（auth/log/telemetry）只能通过 Spring 注入，禁止 `new` 实例化
3. 单文件（.java）≤ 300 行；单方法 ≤ 50 行
4. 禁止 `System.out.println` / `e.printStackTrace()`，统一使用 SLF4J `Logger`
5. 禁止裸 `RestTemplate` / `HttpURLConnection`，统一通过 `ApiClient` 抽象
6. 禁止字段级 `@Autowired`，必须构造器注入（推荐 Lombok `@RequiredArgsConstructor`）
7. 新增代码必须有对应 JUnit 5 测试，行覆盖率 ≥ 80%

## 提交规范
- feat: 新功能
- fix: 修复
- refactor: 重构
- docs: 文档
- test: 测试
```

## 关键设计原则

- AGENTS.md 控制在 50-100 行。超过就说明你在写百科全书了
- "你想做什么 → 去哪里看" 比 "这是什么" 更有效——面向任务而非面向知识
- 硬性规则单独列出，这些是 CI 会强制验证的，不是 "建议"

## 落地路线图：三步走

在动手之前，先画清楚路线。Harness Engineering 的落地不是 "一把梭"，而是渐进式的：

```text
阶段1: 信息层 (1-2天)          阶段2: 约束层 (3-5天)          阶段3: 自动化层 (1-2周)
┌─────────────────────┐      ┌─────────────────────┐      ┌─────────────────────┐
│ AGENTS.md 地图模式   │  →   │ 分层架构 + Linter    │  →   │ Agent 自验证闭环     │
│ docs/ 结构化文档     │      │ CI 约束检查          │      │ 后台清理 Agent       │
│ 编码规范文档化       │      │ 错误信息含修复指令    │      │ JaCoCo/截图自验证    │
└─────────────────────┘      └─────────────────────┘      └─────────────────────┘
适合: 所有项目                适合: 中期项目                适合: 长期维护项目
收益: Agent 输出一致性 ↑      收益: 代码质量可控            收益: 人工审查量 ↓↓
```

**不要一步到位。** 很多人失败就在于想一次性搭完所有基础设施。阶段1 已经能带来显著提升，阶段2 是质变点，阶段3 是锦上添花。

## 落地清单

### Harness Engineering 落地清单

```text
□ 阶段1: 信息层
  □ 创建 AGENTS.md（50-100 行，含 JDK/SB/Maven 版本基线）
  □ 建立 docs/ 结构化目录
  □ 编写架构文档 + 编码规范文档
  □ 设计文档模板（含 Status 标记）

□ 阶段2: 约束层
  □ 配置 maven-enforcer-plugin（锁定 JDK 1.8 + Maven 3.6.3）
  □ 配置 ArchUnit（分层依赖 + 禁字段注入 + 禁裸 HTTP 客户端）
  □ 配置 Checkstyle 9.3 + 自定义错误信息（必须显式声明版本）
  □ 配置 SpotBugs + JaCoCo（行覆盖率 ≥ 80%）
  □ CI: setup-java@8 + setup-maven@3.6.3 + mvn -B clean verify
  □ 把团队口头约定翻译成机械规则

□ 阶段3: 自动化层（可选，长期项目推荐）
  □ Git Worktree 隔离验证脚本（含 spring-boot:run + actuator/health）
  □ 后台清理 Agent 定时任务
  □ 可观测性堆栈（Actuator + Micrometer + Loki + Prometheus）
  □ 文档新鲜度自动检查
  □ Dependabot 配置（防止误升级到 Java 11+ 工具版本）

□ 持续维护
  □ 每周 30 分钟"环境审查"
  □ 每月回顾并更新 ArchUnit / Checkstyle 规则
  □ 每两周运行 doc-gardening Agent
```

**记住：** Harness Engineering 的核心不是搭建复杂的基础设施，而是一个简单的闭环——约束 → 告知 → 验证 → 纠正。从 AGENTS.md 和一条 ArchUnit 规则开始，比什么都不做强一百倍。

## 需求设计文档模版

### 设计文档模板

Agent 执行复杂功能前，先写设计文档。以下是模板：

```markdown
# Feature: [功能名称]

## Status: Draft | Approved | In Progress | Implemented

## 目标
一句话描述这个功能要解决什么问题。

## 非目标
明确列出这次不做什么（防止 Agent 扩大范围）。

## 技术方案

### 涉及的模块
- domain/     : 新增 XXX 实体（@TableName）与 DTO
- mapper/     : 新增 XXXMapper extends BaseMapper<XXX>
- service/    : 新增 XXXService 业务逻辑
- controller/ : 新增 /api/xxx 端点

### 数据模型变更
```sql
-- 如有数据库变更，写在这里（Flyway 迁移脚本路径: src/main/resources/db/migration/V__xxx.sql）
```

### API 变更
```http
POST /api/xxx
  Request:  { ... }
  Response: { ... }
```

## 验收标准
* 标准 1: 具体的、可验证的
* 标准 2: 具体的、可验证的
* 测试覆盖率 ≥ 80%

## 依赖
* 依赖 feature-auth（已实现）
```

**为什么要这么做：** Agent 拿到一个功能需求后，先填写这个模板（或人工填写），审批通过后再动手写代码。这就是"明确意图"的工程化实现。

## 约束层

| 团队口头约定 | 机械化规则 | 实现方式 |
| --- | --- | --- |
| "测试要充分" | 行覆盖率 ≥ 80% | JaCoCo coverage check |
| "锁定构建工具版本" | Maven ≥ 3.6.3，JDK = 1.8 | maven-enforcer-plugin |

**经验法则：** 如果一条规则在 Code Review 中被提过 3 次以上，就应该写成 Linter 规则。

### 分层依赖检查

#### Java 1.8 + Spring Boot 项目：用 ArchUnit

新增依赖（`pom.xml`）：

```xml
<dependency>
  <groupId>com.tngtech.archunit</groupId>
  <artifactId>archunit-junit5</artifactId>
  <version>1.3.0</version>
  <scope>test</scope>
</dependency>
```

新增架构测试（`src/test/java/com/example/app/architecture/LayerDependencyTest.java`）：

### 一个万能公式

```text
❌ [什么错了]
✅ FIX: [怎么改，给出代码片段]
📖 See: [哪个文档有详细说明]
```

Agent 看到这种报错，不需要任何额外提示就能自动修复。你写的每一条 Linter 规则，本质上都是一个自动触发的 Prompt。

### 阶段2：约束层——让 Agent "不能犯错"

Spring Boot 2.7 工程上的标准包结构：

```text
src/main/java/com/example/app/
├── domain/          # 领域模型与 DTO（不依赖任何业务包；纯 POJO + MyBatis-Plus Entity）
│   ├── model/       # MyBatis-Plus Entity（@TableName / @TableId / @TableField）/ Value Obj？
│   └── dto/         # Request/Response/Command/Query（Java 8 用 Lombok @Value 模拟 record）
├── config/          # Spring 配置类、@ConfigurationProperties、@MapperScan、MybatisPlusInter？
├── mapper/          # MyBatis-Plus Mapper 接口（extends BaseMapper<T>；只依赖 domain、config？
├── service/         # 业务逻辑（依赖 domain、config、mapper）
├── controller/      # REST Controller、@ControllerAdvice 全局异常处理
└── infrastructure/  # 横切关注点：ApiClient、日志、指标、安全
```

下面给出具体的 Linter 配置。

### 自定义 Linter 规则：错误信息即 Prompt

这是 Harness Engineering 最有杠杆的实践之一。核心思路：**每条 Linter 报错都必须包含三要素——问题是什么、怎么修、去哪看文档。**

以 ArchUnit 自定义规则为例，禁止直接使用 `RestTemplate` / `HttpURLConnection`（要求统一通过 `ApiClient`）：

```java
@ArchTest
public static final ArchRule noRawHttpClient = noClasses()
    .should().dependOnClassesThat()
        .haveFullyQualifiedName("org.springframework.web.client.RestTemplate")
    .orShould().dependOnClassesThat()
        .haveFullyQualifiedName("java.net.HttpURLConnection")
    .orShould().dependOnClassesThat()
        .haveFullyQualifiedName("org.apache.http.client.HttpClient")
```

（该段在图中被截断，后续行看不清。）

```yaml
# GitHub Actions runner 默认 Maven 版本可能与基线不一致，强制对齐 3.6.3
- name: Set up Maven 3.6.3
  uses: stCarolas/setup-maven@v5
  with:
    maven-version: 3.6.3

- name: Verify tooling
  run: |
    java -version
    mvn -v

# 编译 + 单测 + Checkstyle + SpotBugs + JaCoCo (>= 80%) + ArchUnit
# 全部绑定到 verify 阶段，一次执行；enforcer 会顺带验证 Maven/JDK 版本
- name: Build & Verify
  run: mvn -B clean verify
```

### CI 管线配置：完整的 Agent 护栏

```yaml
name: Agent Guardrails

on: [pull_request]

jobs:
  quality-gates:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v4

    - name: Set up JDK 1.8
      uses: actions/setup-java@v4
      with:
        distribution: 'temurin'
        java-version: '8'
        cache: maven
```

### 把主观品味翻译成机械规则

| 团队口头约定 | 机械化规则 | 实现方式 |
| --- | --- | --- |
| "方法要短" | 单方法 ≤ 50 行 | Checkstyle MethodLength |
| "文件要短" | 单文件 ≤ 300 行 | Checkstyle FileLength |
| "日志要规范" | 禁止 `System.out` / `printStackTrace` | Checkstyle 正则规则 |
| "HTTP 调用要统一" | 禁止裸 `RestTemplate` / `HttpURLConnection` | ArchUnit 自定义规则 |
| "Controller 要纯" | Controller 不得直接调 Mapper | ArchUnit 分层规则 |
| "依赖要构造器注入" | 禁止字段级 `@Autowired` | ArchUnit 注解检查 |
| "不要污染全局" | 禁止 `public static` 非 final 字段 | SpotBugs `MS_*` 规则族 |

## 自动化层

```text
- PR 标题格式: `chore(cleanup): [具体描述]`
- 不允许使用 Java 9+ 语法（record/var/text blocks），保持 JDK 1.8 兼容
- 不允许升级 Spring Boot 主版本（保持 2.7.x）
- 如果不确定某个修改是否安全，跳过并在 PR 中标注原因
```

### 阶段3：自动化层——让 Agent 自我验证和自我修复

#### 后台清理 Agent：定时任务模板

```markdown
# 任务: 代码库卫生清理

请执行以下检查，对每个发现的问题生成独立的修复 PR:

## 检查清单
1. **超长文件**: 找出 src/main/java/ 下超过 300 行的 .java 文件，拆分为更小的类
2. **缺失测试**: 找出 src/main/java/ 下没有对应 *Test.java 的类，补充基础测试
3. **未使用的 import**: 清理所有未使用的 import 语句
4. **TODO/FIXME**: 列出所有 TODO 和 FIXME，超过 30 天未处理则生成清理 PR
5. **重复代码**: 找出高度相似的代码段（>10行），提取为共享工具类（infrastructure/）
6. **过时文档**: 检查 docs/design/ 中状态为 Draft 但已超过 30 天的文档
7. **Checkstyle/SpotBugs 历史告警**: 清理 mvn verify 中累积的非阻塞告警

## 约束
- 每个修复作为独立 PR，不要混在一起
- 每个 PR 修改后必须确保 `mvn -B clean verify` 通过
- PR 标题格式: `chore(cleanup): [具体描述]`
```

### Git Worktree 自动化脚本

```bash
#!/bin/bash

BRANCH=$1
WORKTREE_DIR="/tmp/agent-verify-$(date +%s)"

echo "创建 worktree: $WORKTREE_DIR"
git worktree add "$WORKTREE_DIR" "$BRANCH"

cd "$WORKTREE_DIR"

echo "校验工具链版本..."
java -version 2>&1 | grep '1.8' || { echo "必须使用 JDK 1.8"; exit 1; }
mvn -v | grep 'Apache Maven 3.6.3' || { echo "必须使用 Maven 3.6.3"; exit 1; }

echo "编译并跑全量校验 (enforcer + Checkstyle + SpotBugs + ArchUnit + 单测 + JaCoCo) ..."
mvn -B clean verify || { echo "mvn verify 失败"; exit 1; }
```

### 可观测性接入：让 Agent 能 "看日志"

在本地开发环境部署一个轻量级的可观测性堆栈：

```yaml
version: '3.8'

services:
  loki:
    image: grafana/loki:2.9.0
    ports: ["3100:3100"]

  promtail:
    image: grafana/promtail:2.9.0
    volumes:
      - ./logs:/var/log/app
      - ./promtail-config.yml:/etc/promtail/config.yml

  prometheus:
    image: prom/prometheus:latest
    ports: ["9090:9090"]
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml
```

B站视频画面（标题可见，播放器内正文过小，多数看不清）：

B站讲的最好的Harness Engineering教程（2026最新版）一节课讲透AgentHarness底层原理、核心组件、设计范式剖析，实战案例，项目开发

左侧可见：查找项目开发规范；已运行 26s；已创建 Codex 自定时任务：代码库卫生清理；工作区 harness-demo；模型 gpt-5.2；状态 ACTIVE。任务正文与右侧 `AGENTS.md` / `codex/AGENTS.md` 小字看不清，不转写。

## 总架构

```text
你的项目代码仓库（Spring Boot 2.7 / JDK8）
        |
        +------------------+------------------+
        |                  |                  |
  阶段1: 信息层      阶段2: 约束层      阶段3: 自动化层
  • AGENTS.md        • ArchUnit         • Git Worktree
  • docs/ 结构       • Checkstyle 9.3   • Actuator + Loki
  • Aider Repo Map   • SpotBugs/JaCoCo  • Prometheus
                     • maven-enforcer
        |                  |                  |
        +------------------+------------------+
                           |
                  Agent 工具（选一个）
        Aider | Cline | Claude Code |
        OpenHands | Cursor | Codex
```

### Doc-gardening 没人管，文档比不写还误导

**症状：** Agent 参考了过时文档，写出的代码基于错误的假设。

**解法：**

- 在 CI 中加文档新鲜度检查（见 3.5 节）
- 每两周跑一次 doc-gardening Agent
- 设计文档加 `status` 字段，过期的标记为 `deprecated`

### 过度依赖 Agent，忘了 "审查环境"

**解法：** 每周花 30 分钟做一次 "环境审查"：

- 最近一周的 CI 失败率是否上升？
- Checkstyle / ArchUnit 规则是否覆盖了新出现的 bad pattern？
- AGENTS.md 和 docs/ 是否跟代码库一致？
- 是否有人尝试升级 SB 3.x / Java 11+？

### 架构约束太严，阻碍合理的跨层调用

**症状：** 某些合理的代码模式被 ArchUnit 拦截，团队开始绕过规则。

**解法：** 设置 "豁免白名单" 机制：

```java
@ArchTest
public static final ArchRule controllerMustNotUseMapperExceptLegacy = noClasses()
    .that().resideInAPackage("..controller..")
    .and().resideOutsideOfPackage("..controller.legacy..")
    .should().dependOnClassesThat().resideInAPackage("..mapper..");
```

### 踩坑指南：容易犯的一些错误

#### AGENTS.md 写太长

**症状：** Agent 的输出质量反而下降了，经常忽略部分规则。

**原因：** 上下文窗口被指令文件占满，留给 "正事" 的空间不够。

**解法：** 砍到 50-100 行。如果超过，把内容移到 docs/ 并在 AGENTS.md 中只留链接。

#### Linter 规则太多，Agent 陷入死循环

**症状：** Agent 修了一个 Checkstyle 错误，又触发了另一个 ArchUnit 错误，反复循环。

**解法：**

- 逐条添加 Linter 规则，每加一条都让 Agent 试跑一遍
- 确保每条规则的 FIX 信息给出具体代码片段
- 互相冲突的规则只保留一条

### Agent 工具对比与选型

| 工具 | Stars | 类型 | 最适合的场景 | Harness 友好度 |
| --- | --- | --- | --- | --- |
| Aider | 30k+ | CLI 结对编程 | 个人开发者，终端党 | ★★★★★ |
| Cline | 40k+ | VS Code Agent | 小团队，需要 Plan/Act 模式 | ★★★★ |
| Claude Code | - | CLI Agent | 深度自主任务（6h+ 连续工作） | ★★★★★ |
| OpenHands | 50k+ | 平台 | 团队级部署，需要沙箱隔离 | ★★★★★ |
| SWE-agent | 15k+ | CLI Agent | 自动修复 GitHub Issue | ★★★ |
| Superpowers | 127k | 技能框架 | 强制 TDD + 子 Agent 模式 | ★★★★ |

### 实战案例：从零搭建一个 Harness 环境

以一个 **Spring Boot 2.7.18 + Java 1.8 + Maven 3.6.3 + MySQL 5.7** 的项目为例，走完阶段1 + 阶段2 的核心步骤。

#### Step 1：初始化项目结构（5 分钟）

```bash
# 用 Spring Initializr 生成 Spring Boot 2.7.18 + JDK 1.8 + Maven 工程
# https://start.spring.io/  Project: Maven; Language: Java; SB: 2.7.18; Java: 8
# Dependencies: Web, Validation, Actuator, Lombok（MyBatis-Plus 与 mysql-connector-java 通？

mkdir -p docs/{architecture,conventions,design,plans,reference}

cat > AGENTS.md << 'EOF'
# AGENTS.md

## 项目简介
任务管理 Web 应用，基于 Spring Boot 2.7 + Java 1.8 + MySQL 5.7 + MyBatis-Plus 3.5.x。
```

#### Step 5：验证（2 分钟）

让 Agent 执行一个简单任务，验证 Harness 是否生效：

```text
任务: 在 service/ 中添加一个 UserService.getById(Long id) 方法。
要求:
- 在 domain/dto 中定义 UserResponse（用 Lombok @Value，因 Java 8 无 record）
- 通过 UserMapper（extends MyBatis-Plus BaseMapper<User>）访问数据，必要时配合 LambdaQueryWr？
- 找不到时抛出 UserNotFoundException，由 GlobalExceptionHandler 映射为 404
- UserService 用 @RequiredArgsConstructor 构造器注入 UserMapper
- 编写 JUnit 5 + Mockito 单元测试

预期:
✅ Agent 在 domain/dto 用 @Value 定义 UserResponse（不能用 record）
✅ Agent 在 service/ 中实现，构造器注入 UserMapper
✅ Agent 写了对应单元测试，覆盖率 ≥ 80%
❌ 如果 Agent 用了 record / var / text blocks，maven-compiler-plugin 报错
❌ 如果本地 Maven 不是 3.6.3+ 或 JDK 不是 1.8，maven-enforcer-plugin 报错
❌ 如果 Agent 在 controller/ 中直接 @Autowired UserMapper，ArchUnit 报错
❌ 如果 Agent 引入 spring-boot-starter-data-jpa / @Entity / EntityManager，违反技术栈基线
❌ 如果 Agent 用 System.out.println 打日志，Checkstyle 报错
```

#### Step 3：配置约束（10 分钟）

1. 在 `pom.xml` 中加入 3.3 节给出的完整插件配置（enforcer + checkstyle 9.3 + spotbugs + jacoco + archunit）
2. 创建 `config/checkstyle/checkstyle.xml`（参考 3.4 节）
3. 创建 `src/test/java/com/example/taskapp/architecture/LayerDependencyTest.java`（参考 3.1 节）

#### Step 4：配置 CI（8 分钟）

```yaml
mkdir -p .github/workflows
cat > .github/workflows/harness-checks.yml << 'EOF'
name: Harness Checks

on: [pull_request]

jobs:
  checks:
    ？
```

（图中第 8 行起被截断。）

### Java 版本与依赖版本踩坑（Java 8 专属）

**症状：** 升级了 Checkstyle 或某个依赖后，本地 `mvn verify` 抛 `UnsupportedClassVersionError`。

**原因：** 很多工具的最新版已不再支持 Java 8 运行时（Checkstyle 10+、Spring 6+、Maven 4+ 等）。

**解法：**

- `pom.xml` 中显式锁定 Java 8 兼容版本（参考 3.3 节版本表）
- `dependabot.yml` 配置 `ignore` 阻止误升级：

```yaml
version: 2
updates:
  - package-ecosystem: maven
    directory: "/"
    schedule:
      interval: weekly
    ignore:
      - dependency-name: "com.puppycrawl.tools:checkstyle"
        update-types: ["version-update:semver-major"]
      - dependency-name: "org.springframework.boot:*"
        update-types: ["version-update:semver-major"]
```

#### Step 2：写架构文档（5 分钟）

```text
cat > docs/architecture/overview.md << 'EOF'
# 架构总览

## 包结构

...

src/main/java/com/example/taskapp/
├── domain/          # 领域模型与 DTO（不依赖任何业务包）
│   ├── model/       # MyBatis-Plus Entity（@TableName / @TableId / @TableField）/ Value Ob？
│   └── dto/         # Request / Response（Lombok @Value 不可变 POJO）
├── config/          # Spring 配置类、@ConfigurationProperties、@MapperScan、MybatisPlusInte？
├── mapper/          # MyBatis-Plus Mapper 接口（extends BaseMapper<T>）
├── service/         # 业务逻辑（@RequiredArgsConstructor 构造器注入）
├── controller/      # REST Controller、@ControllerAdvice 全局异常处理
└── infrastructure/  # 横切关注点：ApiClient、日志、指标、安全

...

## 依赖规则
- domain          → 不依赖任何业务包
  ？
```

（图中依赖规则后续行被截断。）
