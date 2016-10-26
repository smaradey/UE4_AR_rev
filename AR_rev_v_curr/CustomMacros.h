#pragma once
#ifndef LOG_MSG
#define LOG_MSG 0
#endif
#if LOG_MSG == 1
#define LOG(message) UE_LOG(LogTemp, Log, TEXT(message));
#else
#define LOG(message)
#endif