# ComfyUI Integration 错误处理机制指南

## 概述

我们为ComfyUI Integration插件实现了完善的错误处理和重试机制，可以智能地处理各种网络请求失败情况，为用户提供清晰的错误信息和自动重试功能。

## 主要功能

### 1. 错误类型分类

定义了多种错误类型枚举：
- `ConnectionFailed` - 连接失败
- `ServerError` - 服务器错误
- `InvalidWorkflow` - 工作流无效
- `JsonParsingError` - JSON解析错误
- `ImageDownloadFailed` - 图像下载失败
- `Timeout` - 请求超时
- `ServerUnavailable` - 服务器不可用
- `AuthenticationFailed` - 认证失败
- `InsufficientResources` - 资源不足
- `UnknownError` - 未知错误

### 2. 错误信息结构

`FComfyUIError` 结构体包含：
- `ErrorType` - 错误类型
- `ErrorMessage` - 错误消息
- `HttpStatusCode` - HTTP状态码
- `SuggestedSolution` - 建议解决方案
- `bCanRetry` - 是否可以重试

### 3. 自动重试机制

- **可配置的重试次数**：默认3次，可通过 `SetRetryConfiguration()` 调整
- **延迟重试**：默认2秒延迟，避免频繁请求
- **智能重试判断**：根据错误类型决定是否适合重试
- **重试状态追踪**：跟踪当前重试次数和状态

### 4. HTTP错误分析

系统可以智能分析HTTP响应：
- **状态码分析**：200, 400, 401, 404, 429, 500, 502, 503, 504等
- **连接状态检查**：区分网络连接错误、超时等
- **响应内容分析**：解析服务器返回的错误信息

## 使用方法

### 配置重试机制

```cpp
// 设置最大重试次数为5次，重试延迟为3秒
ComfyUIClient->SetRetryConfiguration(5, 3.0f);

// 设置请求超时为45秒
ComfyUIClient->SetRequestTimeout(45.0f);
```

### 错误回调处理

```cpp
// 设置错误处理回调
ComfyUIClient->OnImageGenerationFailedCallback.BindLambda(
    [](const FComfyUIError& Error, bool bCanRetry)
    {
        UE_LOG(LogTemp, Error, TEXT("Generation failed: %s"), *Error.ErrorMessage);
        
        if (!Error.SuggestedSolution.IsEmpty())
        {
            UE_LOG(LogTemp, Log, TEXT("Suggested solution: %s"), *Error.SuggestedSolution);
        }
    }
);

// 设置重试回调
ComfyUIClient->OnRetryAttemptCallback.BindLambda(
    [](int32 AttemptNumber)
    {
        UE_LOG(LogTemp, Warning, TEXT("Retrying... Attempt %d"), AttemptNumber);
    }
);
```

## 错误处理流程

1. **请求发送**：HTTP请求发送到ComfyUI服务器
2. **响应分析**：`AnalyzeHttpError()` 分析响应状态
3. **错误分类**：根据状态码和响应内容分类错误
4. **重试判断**：`ShouldRetryRequest()` 判断是否可以重试
5. **重试执行**：如果可以重试，延迟后重新发送请求
6. **错误报告**：如果不能重试或达到最大次数，报告最终错误

## 常见错误及解决方案

### 连接失败 (ConnectionFailed)
- **原因**：无法连接到ComfyUI服务器
- **解决方案**：检查服务器是否运行，URL是否正确
- **可重试**：是

### 请求超时 (Timeout)
- **原因**：服务器响应时间过长
- **解决方案**：增加超时时间或稍后重试
- **可重试**：是

### 工作流无效 (InvalidWorkflow)
- **原因**：发送的工作流JSON格式错误
- **解决方案**：检查工作流文件是否正确
- **可重试**：否

### 服务器错误 (ServerError)
- **原因**：ComfyUI服务器内部错误
- **解决方案**：稍后重试或联系管理员
- **可重试**：是

## 最佳实践

1. **合理设置重试参数**：避免过于频繁的重试
2. **监听错误回调**：及时处理错误信息
3. **提供用户反馈**：将用户友好的错误信息显示给用户
4. **日志记录**：记录详细的错误信息用于调试

## 性能优化

- 使用智能延迟重试，避免服务器过载
- 区分临时错误和永久错误，减少无效重试
- 提供详细的错误信息，帮助快速定位问题

## 扩展性

系统设计具有良好的扩展性：
- 可以轻松添加新的错误类型
- 支持自定义错误处理逻辑
- 可以集成到现有的监控和日志系统

---

*本文档记录了ComfyUI Integration插件错误处理机制的实现和使用方法。*
