#include "project.h"
#include <fstream>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <yaml-cpp/yaml.h>

#include "project/builder.h"
#include "project/group.h"
#include "project/groupparser.h"

#include "output/rompatcher.h"
#include "exceptions.h"
#include "data/addresslist.h"
#include "data/optionhelpers.h"
#include "data/missing_data.h"
#include "font/builder.h"

#include "wrapper/filesystem.h"
#include "project/helpers.h"
#include "project/folder.h"

namespace sable {

Project Project::from(const std::string &projectDir)
{
    if (!fs::exists(fs::path(projectDir) / "config.yml")) {
        throw ConfigError((fs::path(projectDir) / "config.yml").string() + " not found.");
    }
    auto configPath = (fs::path(projectDir) / "config.yml").string();

    auto self = ProjectSerializer::read(YAML::LoadFile(configPath), projectDir);

    for (auto &path: self.m_MappingPaths) {
        auto inFile = YAML::LoadFile(path);
        for (auto fontIt = inFile.begin(); fontIt != inFile.end(); ++fontIt) {
            self.fl[fontIt->first.Scalar()] =
                FontBuilder::make(
                    fontIt->second,
                    fontIt->first.Scalar(),
                    self.m_LocaleString
                );
        }
    }
    return self;
}

bool Project::parseText()
{
    fs::path mainDir(m_MainDir);
    {
        if (!fs::exists(mainDir / m_OutputDir / m_BinsDir)) {
            if (!fs::exists(mainDir / m_OutputDir)) {
                fs::create_directory(mainDir / m_OutputDir);
            }
            if (!fs::exists(mainDir / m_OutputDir / m_BinsDir)) {
                fs::create_directory(mainDir / m_OutputDir / m_BinsDir);
            }
        } else {
            fs::remove_all(mainDir / m_OutputDir / m_BinsDir / m_TextOutDir);
        }
        fs::create_directory(mainDir / m_OutputDir / m_BinsDir / m_TextOutDir);
    }

    auto baseDir = mainDir / m_OutputDir / m_BinsDir / m_TextOutDir;
    Handler handler(
        baseDir,
        std::cerr,
        std::move(fl),
        m_DefaultMode,
        m_LocaleString,
        options::ExportWidth::Off,
        exportAllAddresses
    );
    GroupParser gp{handler};

    {
        fs::path input = fs::path(m_MainDir) / m_InputDir;

        std::vector<fs::path> dirs;
        std::copy(fs::directory_iterator(input), fs::directory_iterator(), std::back_inserter(dirs));
        std::sort(dirs.begin(), dirs.end());

        for (auto& dir: dirs) {
            if (!fs::is_directory(dir)) {
                continue;
            }

            files::Folder f(dir, m_Mapper);
            if (f.table) {
                handler.addresses.addTable(f.group.getName(), f.releaseTable());
            }

            int dirIndex = 0;

            gp.processGroup(f.group, m_Mapper, f.group.getName(), dirIndex);
        }
    }
    AddressList addresses = handler.done();

    {
        std::ofstream mainText((mainDir / m_OutputDir / "text.asm").string());
        std::ofstream textDefines((mainDir / m_OutputDir / "textDefines.exp").string());
        if (!mainText || !textDefines) {
            throw ASMError("Could not open files in " + (mainDir / m_OutputDir).string() + " for writing.\n");
        }
        RomPatcher r(m_BaseType);
        try {
            r.writeParsedData(addresses, fs::path(m_BinsDir) / m_TextOutDir, mainText, textDefines);
        }  catch (sable::MissingData &e) {
            if (e.type == sable::MissingData::Type::Table) {
                // should not occur
                throw std::logic_error(e.what());
            }
            throw sable::ParseError(e.what());
        }

        textDefines.flush();
        textDefines.close();
        mainText.flush();
        mainText.close();
        for (Rom& romData: m_Roms) {
            std::ofstream mainFile((fs::path(m_MainDir) / (romData.name + ".asm")).string());
            mainFile << r.getMapperDirective(m_Mapper.getType()) + "\n\n";

            r.writeInclude("textDefines.exp", mainFile, fs::path(m_OutputDir));
            r.writeInclude("text.asm", mainFile, fs::path(m_OutputDir));

            r.writeIncludes(romData.includes.cbegin(), romData.includes.cend(), mainFile, fs::path(m_OutputDir));
            r.writeIncludes(m_Includes.cbegin(), m_Includes.cend(), mainFile, fs::path(m_OutputDir));
            r.writeIncludes(m_Extras.cbegin(), m_Extras.cend(), mainFile, fs::path(m_OutputDir) / m_BinsDir);

            r.writeInclude(m_FontDir + ".asm", mainFile, fs::path(m_OutputDir) / m_BinsDir / m_FontDir);

            mainFile.close();
        }
        fs::path fontFilePath = fs::path(m_MainDir) / m_OutputDir / m_BinsDir / m_FontDir / (m_FontDir + ".asm");
        if (!fs::exists(fontFilePath.parent_path())) {
            fs::create_directories(fontFilePath.parent_path());
        }
        std::ofstream output(fontFilePath.string());
        if (!output) {
            throw ASMError("Could not open " + fontFilePath.string() + " for writing.\n");
        }
        r.writeIncludes(m_FontIncludes.begin(), m_FontIncludes.end(), output);
        r.writeFontData(handler.getFonts(), output);
        output.close();
    }
    maxAddress = (addresses.end()-1)->address;
    return true;
}

void Project::writePatchData()
{
    fs::path mainDir(m_MainDir);

    bool changeSettings = false;
    if (m_OutputSize == 0) {
        m_OutputSize = m_Mapper.calculateFileSize(maxAddress);
        changeSettings = true;
    }
    for (Rom& romData: m_Roms) {
        RomPatcher r(m_BaseType);
        std::string patchFile = (mainDir / (romData.name + ".asm")).string();

        fs::path romFilePath = fs::path(m_RomsDir) / romData.file;
        std::string extension = romFilePath.extension().string();
        if (!r.loadRom(romFilePath.string(), romData.name, romData.hasHeader)) {
            std::cerr << fs::absolute(romFilePath).string() + " does not exist, or could not be opened.\n";
        } else {
            if (changeSettings && r.getRealSize() >= m_OutputSize) {
                changeSettings = false;
            }
            r.expand(m_OutputSize, m_Mapper);
            auto result = r.applyPatchFile(patchFile);
            if (result) {
                std::cout << "Assembly for " << romData.name << " completed successfully." << std::endl;
            }
            std::vector<std::string> messages;
            r.getMessages(std::back_inserter(messages));
            if (result) {
                for (auto& msg: messages) {
                    std::cout << msg << std::endl;
                }
                std::ofstream output(
                    (fs::path(m_RomsDir) / (romData.name + extension)).string(),
                    std::ios::out|std::ios::binary
                );
                output.write(reinterpret_cast<char*>(&r.at(0)), r.getRealSize());
                output.close();
                r.clear();
            } else {
                for (auto& msg: messages) {
                    std::ostringstream error;
                    error << msg << '\n';
                    throw ASMError(error.str());
                }
            }
        }
    }
    if (changeSettings) {
        YAML::Node configNode = YAML::LoadFile(m_ConfigPath);
        ProjectSerializer::write(configNode, *this);
        std::ofstream output(m_ConfigPath);
        if (output) {
            output << configNode << '\n';
        }
        output.close();
    }
}

Project::Project(util::Mapper&& mapper_): m_Mapper{mapper_}
{

}


std::string Project::MainDir() const
{
    return m_MainDir;
}

std::string Project::RomsDir() const
{
    return fs::absolute(m_RomsDir).string();
}

std::string Project::FontConfig() const
{
    return fs::absolute(fs::path(m_MainDir) / m_OutputDir / m_BinsDir / m_FontDir).string();
}

std::string Project::TextOutDir() const
{
    return fs::absolute(fs::path(m_MainDir) / m_OutputDir / m_BinsDir / m_TextOutDir).string();
}

int Project::getMaxAddress() const
{
    return maxAddress;
}

sable::Project::operator bool() const
{
    return !m_MainDir.empty();
}

util::Mapper Project::getMapper() const
{
    return m_Mapper;
}

bool Project::areAddressesExported() const
{
    return options::isEnabled(exportAllAddresses);
}

ConfigError::ConfigError(std::string message) : std::runtime_error(message) {}
ASMError::ASMError(std::string message) : std::runtime_error(message) {}
ParseError::ParseError(std::string message) : std::runtime_error(message) {}

}
