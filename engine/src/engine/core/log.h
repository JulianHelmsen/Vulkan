#ifndef ENGINE_CORE_LOG_H
#define ENGINE_CORE_LOG_H


#include <stdio.h>

#ifdef BUILD_ENGINE
#define LOG_PREFIX "[ENGINE] "
#else
#define LOG_PREFIX "[CLIENT] "
#endif //BUILD_ENGINE

#ifdef DEBUG
#define log(...) fprintf(stdout, LOG_PREFIX "LOG:\t" __VA_ARGS__)
#else
#define log(...)
#endif
#define err(...) fprintf(stderr, LOG_PREFIX "ERROR:\t" __VA_ARGS__)

#endif //ENGINE_CORE_LOG_H