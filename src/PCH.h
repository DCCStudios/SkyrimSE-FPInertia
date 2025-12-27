#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <SimpleIni.h>
#include <cmath>
#include <mutex>
#include <filesystem>
#include <chrono>

#define DLLEXPORT __declspec(dllexport)

// Macro for version-specific offsets (SE, AE, VR)
#define RELOCATION_OFFSET(SE, AE) REL::VariantOffset(SE, AE, 0).offset()

using namespace std::literals;

namespace logger = SKSE::log;
namespace stl = SKSE::stl;

