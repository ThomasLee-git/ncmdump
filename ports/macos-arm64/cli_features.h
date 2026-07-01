#pragma once

#include <filesystem>

namespace ncmdump::cli {

struct ProcessOptions
{
    bool removeSource{false};
    bool mirrorAudioFiles{false};
};

bool isNcmFile(const std::filesystem::path &path);
bool isAudioFile(const std::filesystem::path &path);

void processInputFile(
    const std::filesystem::path &filePath,
    const std::filesystem::path &outputFolder,
    const ProcessOptions &options);

void processDirectoryFile(
    const std::filesystem::path &filePath,
    const std::filesystem::path &sourceRoot,
    const std::filesystem::path &outputRoot,
    bool outputDirSpecified,
    bool preserveRelativePath,
    const ProcessOptions &options);

} // namespace ncmdump::cli
