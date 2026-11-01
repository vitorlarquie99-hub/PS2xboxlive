#include "runtime/ps2_missing_capture.h"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

int main()
{
    const auto suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto directory = std::filesystem::temp_directory_path() / ("ps2x-capture-test-" + suffix);
    const std::array<uint8_t, 4> memory{0x01, 0x02, 0xfe, 0xff};
    std::string error;
    bool passed = ps2_debug::captureMissingTarget(directory, memory, 0x1000, 0x2000, error);
    std::ifstream dump(directory / "ram.bin", std::ios::binary);
    const std::string contents((std::istreambuf_iterator<char>(dump)), {});
    std::ifstream info(directory / "target.txt");
    const std::string metadata((std::istreambuf_iterator<char>(info)), {});
    passed = passed && contents == std::string("\x01\x02\xfe\xff", 4) &&
             metadata == "source_pc=0x1000\ntarget_pc=0x2000\nram_size=4\n";
    passed = passed && !ps2_debug::captureMissingTarget(directory, memory, 0, 0, error) &&
             error == "destination already exists";
    std::ifstream unchanged(directory / "ram.bin", std::ios::binary);
    passed = passed && std::string((std::istreambuf_iterator<char>(unchanged)), {}) == contents;
    std::error_code ec;
    std::filesystem::remove_all(directory, ec);
    passed = passed && !ps2_debug::captureMissingTarget(directory, {}, 0, 0, error) &&
             !std::filesystem::exists(directory);
    if (!passed || ec)
    {
        std::cerr << "missing target capture test failed: " << error << '\n';
        return 1;
    }
    return 0;
}
