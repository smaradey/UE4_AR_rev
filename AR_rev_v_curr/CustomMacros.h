#pragma once
#ifndef LOG_MSG
#define LOG_MSG 0
#endif
#if LOG_MSG == 1
#define LOG(message) UE_LOG(LogTemp, Log, TEXT(message));
#define LOGA(message,arg) UE_LOG(LogTemp, Log, TEXT(message),arg);
#define LOGA2(message,arg1,arg2) UE_LOG(LogTemp, Log, TEXT(message),arg1,arg2);
#else
#define LOG(message)
#define LOGA(message,arg)
#define LOGA2(message,arg1,arg2)
#endif