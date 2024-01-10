#include "builder.h"

#include <yaml-cpp/yaml.h>
#include <stdexcept>

#include "exceptions.h"
#include "wrapper/filesystem.h"
#include "localecheck.h"
#include "mapperconv.h"

bool sable::ProjectSerializer::validateConfig(const YAML::Node &configYML)
{
    using sable::Project;
    std::ostringstream errorString;
    bool isValid = true;
    if (!configYML[Project::Project::FILES_SECTION].IsDefined() || !configYML[Project::FILES_SECTION].IsMap()) {
        isValid = false;
        errorString << "files section is missing or not a map.\n";
    } else {
        if (!configYML[Project::FILES_SECTION][Project::DIR_MAIN].IsDefined()
                || !configYML[Project::FILES_SECTION][Project::DIR_MAIN].IsScalar()) {
            isValid = false;
            errorString << "main directory for project is missing from files config.\n";
        }
        if (!configYML[Project::FILES_SECTION][Project::INPUT_SECTION].IsDefined()
                || !configYML[Project::FILES_SECTION][Project::INPUT_SECTION].IsMap()) {
            isValid = false;
            errorString << "input section is missing or not a map.\n";
        } else {
            if (!configYML[Project::FILES_SECTION][Project::INPUT_SECTION][Project::DIR_VAL].IsDefined()
                    || !configYML[Project::FILES_SECTION][Project::INPUT_SECTION][Project::DIR_VAL].IsScalar()) {
                isValid = false;
                errorString << "input directory for project is missing from files config.\n";
            }
        }
        if (!configYML[Project::FILES_SECTION][Project::OUTPUT_SECTION].IsDefined() || !configYML[Project::FILES_SECTION][Project::OUTPUT_SECTION].IsMap()) {
            isValid = false;
            errorString << "output section is missing or not a map.\n";
        } else {
            YAML::Node outputConfig = configYML[Project::FILES_SECTION][Project::OUTPUT_SECTION];
            if (!outputConfig[Project::DIR_VAL].IsDefined() || !outputConfig[Project::DIR_VAL].IsScalar()) {
                isValid = false;
                errorString << "output directory for project is missing from files config.\n";
            }
            if (!outputConfig[Project::OUTPUT_BIN].IsDefined() || !outputConfig[Project::OUTPUT_BIN].IsMap()) {
                isValid = false;
                errorString << "output binaries subdirectory section is missing or not a map.\n";
            } else {
                if (!outputConfig[Project::OUTPUT_BIN][Project::DIR_MAIN].IsDefined() || !outputConfig[Project::OUTPUT_BIN][Project::DIR_MAIN].IsScalar()) {
                    isValid = false;
                    errorString << "output binaries subdirectory mainDir value is missing from files config.\n";
                }
                if (!outputConfig[Project::OUTPUT_BIN][Project::DIR_TEXT].IsDefined() || !outputConfig[Project::OUTPUT_BIN][Project::DIR_TEXT].IsScalar()) {
                    isValid = false;
                    errorString << "output binaries subdirectory textDir value is missing from files config.\n";
                }
                if (outputConfig[Project::OUTPUT_BIN][Project::EXTRAS].IsDefined() && !outputConfig[Project::OUTPUT_BIN][Project::EXTRAS].IsSequence()) {
                    isValid = false;
                    errorString << "extras section for output binaries must be a sequence.\n";
                }
                if (!outputConfig[Project::OUTPUT_BIN][Project::FONT_SECTION].IsDefined()
                        || !outputConfig[Project::OUTPUT_BIN][Project::FONT_SECTION].IsMap())
                {
                    isValid = false;
                    errorString << "fonts section for output binaries is missing or not a map.\n";
                } else {
                    if (!outputConfig[Project::OUTPUT_BIN][Project::FONT_SECTION][Project::DIR_FONT].IsDefined() ||
                        !outputConfig[Project::OUTPUT_BIN][Project::FONT_SECTION][Project::DIR_FONT].IsScalar()) {
                        isValid = false;
                        errorString << "fonts directory must be a scalar.\n";
                    }
                    if (outputConfig[Project::OUTPUT_BIN][Project::FONT_SECTION][Project::INCLUDE_VAL].IsDefined() &&
                        !outputConfig[Project::OUTPUT_BIN][Project::FONT_SECTION][Project::INCLUDE_VAL].IsSequence()
                    ) {
                        isValid = false;
                        errorString << "fonts includes must be a sequence.\n";
                    }
                }
            }
            if (outputConfig[Project::INCLUDE_VAL].IsDefined() && !outputConfig[Project::INCLUDE_VAL].IsSequence()) {
                isValid = false;
                errorString << "includes section for output must be a sequence.\n";
            }
        }
        if (!configYML[Project::FILES_SECTION][Project::DIR_ROM].IsDefined() || !configYML[Project::FILES_SECTION][Project::DIR_ROM].IsScalar()) {
            isValid = false;
            errorString << "romDir for project is missing from files config.\n";
        }
    }
    if (!configYML[Project::CONFIG_SECTION].IsDefined() || !configYML[Project::CONFIG_SECTION].IsMap()) {
        isValid = false;
        errorString << "config section is missing or not a map.\n";
    } else {
        if (!configYML[Project::CONFIG_SECTION][Project::DIR_VAL].IsDefined() || !configYML[Project::CONFIG_SECTION][Project::DIR_VAL].IsScalar()) {
            isValid = false;
            errorString << "directory for config section is missing or is not a scalar.\n";
        }
        if (!configYML[Project::CONFIG_SECTION][Project::IN_MAP].IsDefined() ) {
            isValid = false;
            errorString << "inMapping for config section is missing.\n";
        } else if (configYML[Project::CONFIG_SECTION][Project::IN_MAP].IsMap()) {
            isValid = false;
            errorString << "inMapping for config section must be a filename or sequence of filenames.\n";
        }
        if (configYML[Project::CONFIG_SECTION][Project::OUT_SIZE].IsDefined()) {
            if (!configYML[Project::CONFIG_SECTION][Project::OUT_SIZE].IsScalar()) {
                errorString << Project::CONFIG_SECTION + std::string(" > ") + Project::OUT_SIZE +
                               " must be a string with a valid file size(3mb, 4m, etc).";
                isValid = false;
            } else if (util::calculateFileSize(configYML[Project::CONFIG_SECTION][Project::OUT_SIZE].as<std::string>()) == 0) {
                errorString << "\"" + configYML[Project::CONFIG_SECTION][Project::OUT_SIZE].as<std::string>() +
                               "\" is not a supported rom size.";
                isValid = false;
            }
        }
        if (auto exportAddressesOption = configYML[Project::CONFIG_SECTION][Project::EXPORT_ALL_ADDRESSES];
                exportAddressesOption.IsDefined() && !exportAddressesOption.IsScalar()) {
            errorString << Project::CONFIG_SECTION + std::string(" > ") + Project::EXPORT_ALL_ADDRESSES +
                           " must be a string with a valid value(on/off or true/false).\n";
            isValid = false;
        }
    }
    if (!configYML[Project::ROMS].IsDefined()) {
        isValid = false;
        errorString << "roms section is missing.\n";
    } else {
        if (!configYML[Project::ROMS].IsSequence()) {
            isValid = false;
            errorString << "roms section in config must be a sequence.\n";
        } else {
            int romindex = 0;
            for(auto node: configYML[Project::ROMS]) {
                std::string romName;
                if (!node["name"].IsDefined() || !node["name"].IsScalar()) {
                    errorString << "rom at index " << romindex << " is missing a name value.\n";
                    char temp[50];
                    sprintf(temp, "at index %d", romindex);
                    romName = temp;
                    isValid = false;
                } else {
                    romName = node["name"].Scalar();
                }
                if (!node["file"].IsDefined() || !node["file"].IsScalar()) {
                    errorString << "rom " << romName << " is missing a file value.\n";
                    isValid = false;
                }
                if (node["header"].IsDefined() && (!node["header"].IsScalar() || (
                node["header"].Scalar() != "auto" && node["header"].Scalar() != "true" && node["header"].Scalar() != "false"))) {
                    errorString << "rom " << romName << " does not have a valid header option - must be \"true\", \"false\", \"auto\", or not defined.\n";
                    isValid = false;
                }
                ++romindex;
            }
        }
    }
    if (!isValid) {
        throw ConfigError(errorString.str());
    }
    return isValid;
}
namespace sable {

Project ProjectSerializer::read(
    const YAML::Node &config,
    const std::string &projectDir
)
{
    using std::string;
    using std::vector;
    Project pr{util::Mapper(util::MapperType::INVALID, false, true)};

    if (!validateConfig(config)) {
        throw std::logic_error("Config was not valid, but no error was reported.");
    }

    YAML::Node outputConfig = config[Project::FILES_SECTION][Project::Project::OUTPUT_SECTION];
    pr.m_MainDir = config[Project::FILES_SECTION][Project::DIR_MAIN].as<std::string>();
    pr.m_InputDir = config[Project::FILES_SECTION][Project::INPUT_SECTION][Project::DIR_VAL].as<string>();
    pr.m_OutputDir = outputConfig[Project::DIR_VAL].as<string>();
    pr.m_BinsDir = outputConfig[Project::OUTPUT_BIN][Project::DIR_MAIN].as<string>();
    pr.m_TextOutDir = outputConfig[Project::OUTPUT_BIN][Project::DIR_TEXT].as<string>();
    pr.m_FontDir = outputConfig[Project::OUTPUT_BIN][Project::FONT_SECTION][Project::DIR_FONT].as<string>();
    pr.m_RomsDir = config[Project::FILES_SECTION][Project::DIR_ROM].as<string>();
    pr.m_OutputSize = config[Project::CONFIG_SECTION][Project::OUT_SIZE].IsDefined() ?
                util::calculateFileSize(config[Project::CONFIG_SECTION][Project::OUT_SIZE].as<std::string>()) :
                0;
    if (config[Project::CONFIG_SECTION][Project::LOCALE].IsDefined()) {
        pr.m_LocaleString = config[Project::CONFIG_SECTION][Project::LOCALE].as<std::string>();
        if (!isLocaleValid(pr.m_LocaleString.c_str())) {
            throw ConfigError("The provided locale is not valid.");
        }
    } else {
        pr.m_LocaleString = "en_US.UTF8";
    }
    if (config[Project::CONFIG_SECTION][Project::MAP_TYPE].IsDefined()) {
        try {
            bool useMirrors = false;
            if (config[Project::CONFIG_SECTION][Project::USE_MIRRORED_BANKS].IsDefined()) {
                useMirrors = config[Project::CONFIG_SECTION][Project::USE_MIRRORED_BANKS].as<std::string>() == "true";
            }
            auto mapperType = config[Project::CONFIG_SECTION][Project::MAP_TYPE].as<util::MapperType>();
            pr.m_BaseType = mapperType;
            if (pr.m_OutputSize > util::NORMAL_ROM_MAX_SIZE) {
                mapperType = util::getExpandedType(mapperType);
            }
            pr.m_Mapper = util::Mapper(mapperType, false, !useMirrors, pr.m_OutputSize);
        } catch (YAML::TypedBadConversion<util::MapperType> &e) {
            throw ConfigError("The provided mapper must be one of: lorom, hirom, exlorom, exhirom.");
        } catch (YAML::TypedBadConversion<std::string> &e) {
            throw ConfigError("The useMirrorBanks option should be true or false.");
        }
    } else {
        pr.m_BaseType = util::MapperType::LOROM;
        if (pr.m_OutputSize > util::NORMAL_ROM_MAX_SIZE) {
            pr.m_Mapper = util::Mapper(util::MapperType::EXLOROM, false, true, pr.m_OutputSize);
        } else {
            pr.m_Mapper = util::Mapper(util::MapperType::LOROM, false, true, pr.m_OutputSize);
        }
    }

    fs::path mainDir(projectDir);
    if (fs::path(pr.m_MainDir).is_relative()) {
        pr.m_MainDir = (mainDir / pr.m_MainDir).string();
    }
    if (fs::path(pr.m_RomsDir).is_relative()) {
        pr.m_RomsDir = (mainDir / pr.m_RomsDir).string();
    }
    if (outputConfig[Project::OUTPUT_BIN][Project::FONT_SECTION][Project::INCLUDE_VAL].IsSequence()) {
        for (auto& include : outputConfig[Project::OUTPUT_BIN][Project::FONT_SECTION][Project::INCLUDE_VAL].as<vector<string>>()) {
            pr.m_FontIncludes.push_back(include + ".asm");
        }
    }
    if (outputConfig[Project::OUTPUT_BIN][Project::EXTRAS].IsSequence()) {
        for (auto& include : outputConfig[Project::OUTPUT_BIN][Project::EXTRAS].as<vector<string>>()) {
            pr.m_Extras.push_back((fs::path(include) / (include + ".asm")).string());
        }
    }
    if (outputConfig[Project::INCLUDE_VAL].IsSequence()){
        pr.m_Includes = outputConfig[Project::INCLUDE_VAL].as<vector<string>>();
    }
    for (auto romData: config[Project::ROMS]) {
        pr.m_Roms.push_back(romData.as<Rom>());
    }

    fs::path fontLocation = mainDir
            / config[Project::CONFIG_SECTION][Project::DIR_VAL].as<string>();

    pr.m_DefaultMode = config[Project::CONFIG_SECTION][Project::DEFAULT_MODE].IsDefined()
            ? config[Project::CONFIG_SECTION][Project::DEFAULT_MODE].as<string>() : "normal";
    if (config[Project::CONFIG_SECTION][Project::IN_MAP].IsScalar()) {
        fontLocation = fontLocation / config[Project::CONFIG_SECTION][Project::IN_MAP].as<std::string>();
        if (!fs::exists(fontLocation)) {
            throw ConfigError(fontLocation.string() + " not found.");
        }
        pr.m_MappingPaths.push_back(fontLocation.string());
    } else if (config[Project::CONFIG_SECTION][Project::IN_MAP].IsSequence()) {
        for (auto&& path: config[Project::CONFIG_SECTION][Project::IN_MAP]) {
            pr.m_MappingPaths.push_back((fontLocation / path.as<std::string>()).string());
        }
    }

    auto isExplicitylDisabled = [] (std::string&& value) -> bool
    {
        std::string lower = value;
        std::transform(value.begin(), value.end(), lower.begin(), [] (char c) {
            return std::tolower(c);
        });
        return lower == "false" || lower == "off";
    };

    if (auto exportAddressesOption = config[Project::CONFIG_SECTION][Project::EXPORT_ALL_ADDRESSES];
        exportAddressesOption.IsDefined() && exportAddressesOption.IsScalar() &&
        isExplicitylDisabled(exportAddressesOption.as<std::string>())) {
        pr.exportAllAddresses = options::ExportAddress::Off;
    } else {
        pr.exportAllAddresses = options::ExportAddress::On;
    }
    return pr;
}

void ProjectSerializer::write(YAML::Node &config, const Project& pr)
{
    config[Project::Project::CONFIG_SECTION][Project::Project::OUT_SIZE] = util::getFileSizeString(pr.m_OutputSize);
}

}

namespace YAML {

bool convert<sable::ProjectSerializer::Rom>::decode(
        const Node &node,
        sable::ProjectSerializer::Rom &rhs
) {
    rhs.name = node["name"].as<std::string>();
    rhs.file = node["file"].as<std::string>();
    rhs.hasHeader = 0;
    if (node["header"].IsDefined()) {
        std::string tmp = node["header"].as<std::string>();
        if (tmp == "true") {
            rhs.hasHeader = 1;
        } else if (tmp == "false") {
            rhs.hasHeader = -1;
        } else if (tmp == "auto") {
            rhs.hasHeader = 0;
        } else {
            return false;
        }
    }
    if (node["includes"].IsDefined() && node["includes"].IsSequence()) {
        rhs.includes = node["includes"].as<std::vector<std::string>>();
    }
    return true;
}

}


