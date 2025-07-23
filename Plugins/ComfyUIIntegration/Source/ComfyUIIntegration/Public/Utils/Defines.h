#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

#define LOG_AND_RETURN(LogType, ReturnValue, Format, ...)      \
    do {                                                       \
        UE_LOG(LogTemp, LogType, TEXT(Format), ##__VA_ARGS__); \
        return ReturnValue;                                    \
    } while(0)