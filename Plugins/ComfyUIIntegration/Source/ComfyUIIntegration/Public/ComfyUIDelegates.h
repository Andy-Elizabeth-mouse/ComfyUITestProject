#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "ComfyUITypes.h"

// Delegate declarations - these must be in global scope
DECLARE_DELEGATE_OneParam(FOnRetryAttempt, int32 /* AttemptNumber */)
DECLARE_DELEGATE_TwoParams(FOnConnectionTested, bool /* bSuccess */, FString /* ErrorMessage */)
DECLARE_DELEGATE_OneParam(FOnGenerationStarted, const FString& /* PromptId */)
DECLARE_DELEGATE_OneParam(FOnGenerationProgress, const FComfyUIProgressInfo& /* ProgressInfo */)
DECLARE_DELEGATE_TwoParams(FOnGenerationFailed, const FComfyUIError& /* Error */, bool /* bCanRetry */)
DECLARE_DELEGATE(FOnGenerationCompleted)

DECLARE_DELEGATE_OneParam(FOnImageGenerated, UTexture2D* /* Output */)
DECLARE_DELEGATE_OneParam(FOnMeshGenerated, UStaticMesh*)