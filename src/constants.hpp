#pragma once

#include <chrono>
#include <string>

#define DEFAULT_LOCALE_ID "english"

#ifdef DEBUG_MODE
const std::string DEFAULT_PREFIX = "~";
#else
const std::string DEFAULT_PREFIX = "!";
#endif
const auto START_TIME = std::chrono::steady_clock::now();
