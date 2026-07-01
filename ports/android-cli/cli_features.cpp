#include "cli_features.h"

#include "color.h"
#include "ncmcrypt.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace ncmdump::cli {
namespace {

bool existsNoThrow(const fs::path &path)
{
    std::error_code ec;
    return fs::exists(path, ec);
}

bool removeNoThrow(const fs::path &path)
{
    std::error_code ec;
    return fs::remove(path, ec);
}

std::string lowerExtension(const fs::path &path)
{
    auto extension = path.extension().u8string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension;
}

std::vector<fs::path> candidateOutputPaths(const fs::path &filePath, const fs::path &outputFolder)
{
    fs::path outputPath = outputFolder.empty()
                              ? filePath
                              : outputFolder / filePath.filename();

    fs::path mp3Path = outputPath;
    fs::path flacPath = outputPath;
    mp3Path.replace_extension("mp3");
    flacPath.replace_extension("flac");

    return {mp3Path, flacPath};
}

bool isLikelyGeneratedOutput(const fs::path &filePath, const fs::path &outputFolder, const fs::path &path)
{
    if (path.empty() || path == filePath)
    {
        return false;
    }

    const auto extension = lowerExtension(path);
    if (extension != ".mp3" && extension != ".flac")
    {
        return false;
    }

    for (const auto &candidate : candidateOutputPaths(filePath, outputFolder))
    {
        if (path == candidate)
        {
            return true;
        }
    }

    return false;
}

bool skipIfOutputExists(const fs::path &filePath, const fs::path &outputFolder)
{
    for (const auto &candidate : candidateOutputPaths(filePath, outputFolder))
    {
        if (existsNoThrow(candidate))
        {
            std::cout << BOLDYELLOW << "[Skip] " << RESET << "'" << filePath.u8string()
                      << "' output already exists: '" << candidate.u8string() << "'" << std::endl;
            return true;
        }
    }

    return false;
}

void cleanupFailedOutput(const fs::path &filePath, const fs::path &outputFolder, const fs::path &dumpFilepath)
{
    if (!isLikelyGeneratedOutput(filePath, outputFolder, dumpFilepath) || !existsNoThrow(dumpFilepath))
    {
        return;
    }

    if (removeNoThrow(dumpFilepath))
    {
        std::cerr << BOLDYELLOW << "[Clean] " << RESET << "removed failed output '"
                  << dumpFilepath.u8string() << "'" << std::endl;
    }
}

void copyFileForMirror(const fs::path &sourcePath, const fs::path &destinationPath)
{
    std::error_code ec;

    if (sourcePath == destinationPath)
    {
        return;
    }

    if (existsNoThrow(destinationPath))
    {
        std::cout << BOLDYELLOW << "[Skip] " << RESET << "'" << sourcePath.u8string()
                  << "' copy target already exists: '" << destinationPath.u8string() << "'" << std::endl;
        return;
    }

    fs::create_directories(destinationPath.parent_path(), ec);
    ec.clear();
    if (fs::copy_file(sourcePath, destinationPath, fs::copy_options::none, ec))
    {
        std::cout << BOLDGREEN << "[Copy] " << RESET << "'" << sourcePath.u8string()
                  << "' -> '" << destinationPath.u8string() << "'" << std::endl;
        return;
    }

    std::cerr << BOLDRED << "[Error] " << RESET << "failed to copy '" << sourcePath.u8string()
              << "' -> '" << destinationPath.u8string() << "'";
    if (ec)
    {
        std::cerr << ": " << ec.message();
    }
    std::cerr << std::endl;
}

} // namespace

bool isNcmFile(const fs::path &path)
{
    return path.has_extension() && lowerExtension(path) == ".ncm";
}

bool isAudioFile(const fs::path &path)
{
    const auto extension = lowerExtension(path);
    return extension == ".mp3" || extension == ".wav" || extension == ".flac";
}

void processInputFile(const fs::path &filePath, const fs::path &outputFolder, const ProcessOptions &options)
{
    if (!existsNoThrow(filePath))
    {
        std::cerr << BOLDRED << "[Error] " << RESET << "file '" << filePath.u8string()
                  << "' does not exist." << std::endl;
        return;
    }

    if (!isNcmFile(filePath))
    {
        return;
    }

    if (skipIfOutputExists(filePath, outputFolder))
    {
        return;
    }

    std::unique_ptr<NeteaseCrypt> crypt;

    try
    {
        crypt = std::make_unique<NeteaseCrypt>(filePath.u8string());
        crypt->Dump(outputFolder.u8string());
        crypt->FixMetadata();

        std::cout << BOLDGREEN << "[Done] " << RESET << "'" << filePath.u8string()
                  << "' -> '" << crypt->dumpFilepath().u8string() << "'";

        if (options.removeSource)
        {
            if (removeNoThrow(filePath))
            {
                std::cout << " with removed as required.";
            }
            else
            {
                std::cout << " but failed to remove source.";
            }
        }
        std::cout << std::endl;
    }
    catch (const std::exception &e)
    {
        if (crypt)
        {
            cleanupFailedOutput(filePath, outputFolder, crypt->dumpFilepath());
        }

        std::cerr << BOLDRED << "[Exception] " << RESET << RED << e.what() << RESET
                  << " '" << filePath.u8string() << "'" << std::endl;
    }
    catch (...)
    {
        if (crypt)
        {
            cleanupFailedOutput(filePath, outputFolder, crypt->dumpFilepath());
        }

        std::cerr << BOLDRED << "[Error] Unexpected exception while processing file: " << RESET
                  << filePath.u8string() << std::endl;
    }
}

void processDirectoryFile(
    const fs::path &filePath,
    const fs::path &sourceRoot,
    const fs::path &outputRoot,
    bool outputDirSpecified,
    bool preserveRelativePath,
    const ProcessOptions &options)
{
    if (isNcmFile(filePath))
    {
        if (outputDirSpecified && preserveRelativePath)
        {
            const auto relativePath = fs::relative(filePath, sourceRoot);
            const auto destinationPath = outputRoot / relativePath;
            fs::create_directories(destinationPath.parent_path());
            processInputFile(filePath, destinationPath.parent_path(), options);
        }
        else
        {
            processInputFile(filePath, outputDirSpecified ? outputRoot : fs::u8path(""), options);
        }
        return;
    }

    if (!options.mirrorAudioFiles || !outputDirSpecified || !isAudioFile(filePath))
    {
        return;
    }

    const auto destinationPath = preserveRelativePath
                                     ? outputRoot / fs::relative(filePath, sourceRoot)
                                     : outputRoot / filePath.filename();
    copyFileForMirror(filePath, destinationPath);
}

} // namespace ncmdump::cli
