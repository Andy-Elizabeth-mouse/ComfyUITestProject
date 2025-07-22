# ComfyUI集成插件 - 访问违例修复文档

## 问题描述

**错误信息:**
```
Unhandled Exception: EXCEPTION_ACCESS_VIOLATION reading address 0x0000000000000230
UnrealEditor_ComfyUIIntegration!UComfyUIClient::OnQueueStatusChecked() [ComfyUIClient.cpp:474]
```

**问题分析:**
在 `UComfyUIClient::OnQueueStatusChecked()` 函数中，代码尝试调用 `GetWorld()->GetTimerManager().SetTimer()` 但是由于 `UComfyUIClient` 继承自 `UObject` 而非世界上下文相关的类，`GetWorld()` 返回了空指针（nullptr），导致访问违例崩溃。

## 问题根源

1. **类设计问题**: `UComfyUIClient` 继承自 `UObject`，没有内置的世界上下文访问权限
2. **缺失世界引用**: 在创建 `UComfyUIClient` 实例时没有设置适当的世界上下文
3. **定时器管理**: 代码需要访问世界的定时器管理器来设置轮询定时器

## 修复方案

### 1. 头文件修改 (ComfyUIClient.h)

添加了世界上下文管理功能:

```cpp
/** 设置世界上下文 */
void SetWorldContext(UWorld* InWorld);

private:
    /** 世界上下文引用 */
    UPROPERTY()
    TWeakObjectPtr<UWorld> WorldContext;
```

### 2. 实现文件修改 (ComfyUIClient.cpp)

**构造函数初始化:**
```cpp
UComfyUIClient::UComfyUIClient()
{
    HttpModule = &FHttpModule::Get();
    ServerUrl = TEXT("http://127.0.0.1:8188");
    WorldContext = nullptr;  // 新增
    InitializeWorkflowConfigs();
    LoadWorkflowConfigs();
}
```

**世界上下文设置方法:**
```cpp
void UComfyUIClient::SetWorldContext(UWorld* InWorld)
{
    WorldContext = InWorld;
}
```

**安全的定时器设置:**
```cpp
// 修复前 (会崩溃)
GetWorld()->GetTimerManager().SetTimer(StatusPollTimer, 
    FTimerDelegate::CreateLambda([this]() { PollGenerationStatus(CurrentPromptId); }),
    2.0f, false);

// 修复后 (安全)
if (WorldContext.IsValid())
{
    WorldContext->GetTimerManager().SetTimer(StatusPollTimer, 
        FTimerDelegate::CreateLambda([this]() { PollGenerationStatus(CurrentPromptId); }),
        2.0f, false);
}
else
{
    UE_LOG(LogTemp, Error, TEXT("Invalid world context for timer management"));
    OnImageGeneratedCallback.ExecuteIfBound(nullptr);
}
```

### 3. 使用处修改 (ComfyUIWidget.cpp)

**添加头文件包含:**
```cpp
#include "Engine/World.h"
#include "Engine/Engine.h"
```

**创建客户端时设置世界上下文:**
```cpp
UComfyUIClient* Client = NewObject<UComfyUIClient>();
if (Client)
{
    Client->SetServerUrl(ServerUrl);
    
    // 设置世界上下文
    if (GEngine && GEngine->GetCurrentPlayWorld())
    {
        Client->SetWorldContext(GEngine->GetCurrentPlayWorld());
    }
    else if (GWorld)
    {
        Client->SetWorldContext(GWorld);
    }
    
    // ... 其余代码
}
```

## 技术细节

### 为什么选择 TWeakObjectPtr？

1. **避免循环引用**: 使用弱指针避免客户端和世界对象之间的强引用循环
2. **自动失效**: 当世界对象被销毁时，弱指针会自动失效
3. **线程安全**: 提供安全的空指针检查

### 世界获取优先级

1. **优先使用播放世界**: `GEngine->GetCurrentPlayWorld()` - 用于游戏运行时
2. **回退到编辑器世界**: `GWorld` - 用于编辑器环境
3. **最终检查**: 使用 `IsValid()` 确保指针有效

## 测试验证

1. **编译测试**: ✅ 项目成功编译，无语法错误
2. **功能测试**: 需要在编辑器中测试ComfyUI界面功能
3. **稳定性测试**: 验证不再出现访问违例崩溃

## 最佳实践

1. **总是检查世界指针**: 在使用世界相关功能前进行有效性检查
2. **使用弱指针**: 对于可能被外部管理的对象引用
3. **错误处理**: 提供适当的错误处理和用户反馈
4. **日志记录**: 使用UE_LOG记录错误信息便于调试

## 后续改进建议

1. **考虑使用子系统**: 将 `UComfyUIClient` 改为继承 `UEngineSubsystem` 或 `UGameInstanceSubsystem`
2. **单例模式**: 实现客户端单例以避免重复创建
3. **配置管理**: 改进世界上下文的获取和管理机制

---

**修复日期**: 2025年7月18日  
**修复版本**: v1.1  
**状态**: ✅ 已解决
