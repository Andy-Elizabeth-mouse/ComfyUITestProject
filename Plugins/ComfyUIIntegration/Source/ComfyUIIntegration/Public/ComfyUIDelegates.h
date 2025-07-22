#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "ComfyUITypes.h"

// Delegate declarations - these must be in global scope
DECLARE_DELEGATE_OneParam(FOnImageGenerated, UTexture2D*)
DECLARE_DELEGATE_TwoParams(FOnConnectionTested, bool /* bSuccess */, FString /* ErrorMessage */)
DECLARE_DELEGATE_TwoParams(FOnImageGenerationFailed, const FComfyUIError& /* Error */, bool /* bCanRetry */)
DECLARE_DELEGATE_OneParam(FOnRetryAttempt, int32 /* AttemptNumber */)