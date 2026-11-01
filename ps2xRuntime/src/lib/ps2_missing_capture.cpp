#include "runtime/ps2_missing_capture.h"

#include <fstream>
#include <iomanip>
#include <limits>
#include <system_error>

namespace ps2_debug
{
    bool captureMissingTarget(const std::filesystem::path &directory,
                              std::span<const uint8_t> ram,
                              uint32_t sourcePc,
                              uint32_t targetPc,
                              std::string &error)
    {
        error.clear();
        if (directory.empty() || ram.empty() ||
            ram.size() > static_cast<size_t>(std::numeric_limits<std::streamsize>::max()))
        {
            error = "missing destination or guest memory";
            return false;
        }
        std::error_code ec;
        if (!std::filesystem::create_directory(directory, ec))
        {
            error = ec ? ec.message() : "destination already exists";
            return false;
        }

        std::ofstream dump(directory / "ram.bin", std::ios::binary);
        dump.write(reinterpret_cast<const char *>(ram.data()), static_cast<std::streamsize>(ram.size()));
        dump.close();
        if (!dump)
        {
            error = "failed to write guest memory";
            return false;
        }
        std::ofstream info(directory / "target.txt");
        info << "source_pc=0x" << std::hex << sourcePc << "\ntarget_pc=0x" << targetPc
             << "\nram_size=" << std::dec << ram.size() << '\n';
        info.close();
        if (!info)
        {
            error = "failed to write capture metadata";
            return false;
        }
        return true;
    }
}
