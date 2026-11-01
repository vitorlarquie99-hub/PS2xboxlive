#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>

namespace ps2_debug
{
    // The destination directory must not already exist. Failed writes leave it
    // in place for inspection; existing paths are never overwritten.
    bool captureMissingTarget(const std::filesystem::path &directory,
                              std::span<const uint8_t> ram,
                              uint32_t sourcePc,
                              uint32_t targetPc,
                              std::string &error);
}
