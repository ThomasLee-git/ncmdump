#include "cli_features.h"

#include <filesystem>
#include <iostream>
#include <vector>

#include "color.h"
#include "cxxopts.hpp"
#include "version.h"

#ifndef NCMDUMP_MACOS_PORT_VERSION
#define NCMDUMP_MACOS_PORT_VERSION "macos-arm64.dev"
#endif

namespace fs = std::filesystem;

int main(int argc, char **argv)
{
    cxxopts::Options options("ncmdump");

    options.add_options()
        ("h,help", "Print usage")
        ("d,directory", "Process files in a folder (requires folder path)", cxxopts::value<std::string>())
        ("r,recursive", "Process files recursively (requires -d option)", cxxopts::value<bool>()->default_value("false"))
        ("o,output", "Output folder (default: original file folder)", cxxopts::value<std::string>())
        ("v,version", "Print version information", cxxopts::value<bool>()->default_value("false"))
        ("m,remove", "Remove original file if done", cxxopts::value<bool>()->default_value("false"))
        ("filenames", "Input files", cxxopts::value<std::vector<std::string>>());

    options.positional_help("<files>");
    options.parse_positional({"filenames"});
    options.allow_unrecognised_options();

    cxxopts::ParseResult result;
    try
    {
        result = options.parse(argc, argv);
    }
    catch (cxxopts::exceptions::parsing const &)
    {
        std::cout << options.help() << std::endl;
        return 1;
    }

    if (!result.unmatched().empty())
    {
        std::cout << options.help() << std::endl;
        return 1;
    }

    if (result.count("help"))
    {
        std::cout << options.help() << std::endl;
        return 0;
    }

    if (result.count("version"))
    {
        std::cout << "ncmdump version " << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH
                  << " (" << NCMDUMP_MACOS_PORT_VERSION << ")" << std::endl;
        return 0;
    }

    if (result.count("directory") == 0 && result.count("filenames") == 0)
    {
        std::cout << options.help() << std::endl;
        return 1;
    }

    if (result.count("recursive") && result.count("directory") == 0)
    {
        std::cerr << BOLDRED << "[Error] " << RESET << "-r option requires -d option." << std::endl;
        return 1;
    }

    fs::path outputDir = fs::u8path("");
    const bool outputDirSpecified = result.count("output") > 0;

    if (outputDirSpecified)
    {
        outputDir = fs::u8path(result["output"].as<std::string>());
        if (fs::exists(outputDir) && !fs::is_directory(outputDir))
        {
            std::cerr << BOLDRED << "[Error] " << RESET << "'" << outputDir.u8string()
                      << "' is not a valid directory." << std::endl;
            return 1;
        }
        fs::create_directories(outputDir);
    }

    ncmdump::cli::ProcessOptions processOptions;
    processOptions.removeSource = result["remove"].as<bool>();
    processOptions.mirrorAudioFiles = outputDirSpecified;

    if (result.count("directory"))
    {
        const fs::path sourceDir = fs::u8path(result["directory"].as<std::string>());
        if (!fs::is_directory(sourceDir))
        {
            std::cerr << BOLDRED << "[Error] " << RESET << "'" << sourceDir.u8string()
                      << "' is not a valid directory." << std::endl;
            return 1;
        }

        const bool recursive = result["recursive"].as<bool>();

        if (recursive)
        {
            const fs::path rootOutputDir = outputDirSpecified ? outputDir : sourceDir;
            for (const auto &entry : fs::recursive_directory_iterator(sourceDir))
            {
                const auto &path = fs::u8path(entry.path().u8string());
                if (fs::is_regular_file(path))
                {
                    ncmdump::cli::processDirectoryFile(
                        path,
                        sourceDir,
                        rootOutputDir,
                        outputDirSpecified,
                        true,
                        processOptions);
                }
            }
        }
        else
        {
            for (const auto &entry : fs::directory_iterator(sourceDir))
            {
                const auto &path = fs::u8path(entry.path().u8string());
                if (entry.is_regular_file())
                {
                    ncmdump::cli::processDirectoryFile(
                        path,
                        sourceDir,
                        outputDir,
                        outputDirSpecified,
                        false,
                        processOptions);
                }
            }
        }
        return 0;
    }

    if (result.count("filenames"))
    {
        for (const auto &filePath : result["filenames"].as<std::vector<std::string>>())
        {
            const fs::path filePathU8 = fs::u8path(filePath);
            if (!fs::is_regular_file(filePathU8))
            {
                std::cerr << BOLDRED << "[Error] " << RESET << "'" << filePathU8.u8string()
                          << "' is not a valid file." << std::endl;
                continue;
            }

            ncmdump::cli::processInputFile(
                filePathU8,
                outputDirSpecified ? outputDir : fs::u8path(""),
                processOptions);
        }
    }

    return 0;
}
