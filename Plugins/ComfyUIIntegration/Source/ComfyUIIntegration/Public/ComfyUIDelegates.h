#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "ComfyUITypes.h"
#include "Utils/ComfyUIFileManager.h"

// Forward declarations
class UStaticMesh;
class UMaterialInstanceDynamic;

// Delegate declarations - these must be in global scope

// 图像生成相关委托
DECLARE_DELEGATE_OneParam(FOnImageGenerated, UTexture2D*)

// 3D模型生成相关委托  
DECLARE_DELEGATE_OneParam(FOn3DModelGenerated, const TArray<uint8>& /* ModelData */)
DECLARE_DELEGATE_TwoParams(FOn3DModelImported, UStaticMesh* /* ImportedMesh */, bool /* bSuccess */)

// PBR纹理集生成相关委托
// 暂时简化为单个纹理回调，避免TMap模板问题
DECLARE_DELEGATE_OneParam(FOnTextureGenerated, UTexture2D* /* GeneratedTexture */)
DECLARE_DELEGATE_TwoParams(FOnMaterialCreated, UMaterialInstanceDynamic* /* Material */, bool /* bSuccess */)

// 通用错误和状态委托
DECLARE_DELEGATE_TwoParams(FOnConnectionTested, bool /* bSuccess */, FString /* ErrorMessage */)
DECLARE_DELEGATE_TwoParams(FOnImageGenerationFailed, const FComfyUIError& /* Error */, bool /* bCanRetry */)
DECLARE_DELEGATE_OneParam(FOnRetryAttempt, int32 /* AttemptNumber */)
DECLARE_DELEGATE_OneParam(FOnGenerationProgress, const FComfyUIProgressInfo& /* ProgressInfo */)
DECLARE_DELEGATE_OneParam(FOnGenerationStarted, const FString& /* PromptId */)
DECLARE_DELEGATE(FOnGenerationCompleted)