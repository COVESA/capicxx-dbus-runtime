/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <fstream>

#include <CommonAPI/utils.h>

#include "DBusConfiguration.h"


namespace CommonAPI {
namespace DBus {


enum class TypeEnum {
    DBUS_CONNECTION, DBUS_OBJECT, DBUS_INTERFACE, DBUS_PREDEFINED
};

enum class TypeEnumFactory {
    DBUS_BUSTYPE
};

enum class FileParsingState {
    UNDEFINED,
    PARSING_ADDRESS,
    PARSING_FACTORY
};


static const std::unordered_map<std::string, TypeEnum> allowedValueTypes = {
                {"dbus_connection", TypeEnum::DBUS_CONNECTION},
                {"dbus_object", TypeEnum::DBUS_OBJECT},
                {"dbus_interface", TypeEnum::DBUS_INTERFACE},
                {"dbus_predefined", TypeEnum::DBUS_PREDEFINED}
};

static const std::unordered_map<std::string, TypeEnumFactory> allowedValueTypesFactory = {
                {"dbus_bustype", TypeEnumFactory::DBUS_BUSTYPE}
};


const DBusConfiguration& DBusConfiguration::getInstance() {
    static DBusConfiguration* instance = NULL;
    if (!instance) {
        instance = new DBusConfiguration();
        instance->retrieveCommonApiDBusConfiguration();
    }
    return *instance;
}

void DBusConfiguration::retrieveCommonApiDBusConfiguration() {
    std::string fqnOfConfigFile = getCurrentBinaryFileFQN();
    std::ifstream addressConfigFile;

    fqnOfConfigFile += DBUS_CONFIG_SUFFIX;

    addressConfigFile.open(fqnOfConfigFile.c_str());

    if (addressConfigFile.is_open()) {
        readConfigFile(addressConfigFile);
        addressConfigFile.close();
    }

    addressConfigFile.clear();
    std::vector<std::string> splittedConfigFQN = split(fqnOfConfigFile, '/');
    std::string globalConfigFQN = DBUS_GLOBAL_CONFIG_ROOT + splittedConfigFQN.at(splittedConfigFQN.size() - 1);
    addressConfigFile.open(globalConfigFQN);
    if (addressConfigFile.is_open()) {
        readConfigFile(addressConfigFile);
        addressConfigFile.close();
    }
    addressConfigFile.clear();

    addressConfigFile.open(DBUS_GLOBAL_CONFIG_FQN);
    if(addressConfigFile.is_open()) {
        readConfigFile(addressConfigFile);
        addressConfigFile.close();
    }
}

const std::unordered_map<std::string, CommonApiServiceDetails>& DBusConfiguration::getCommonApiAddressDetails() const {
    return std::move(commonApiAddressDetails_);
}

const DBusFactoryConfig* DBusConfiguration::getFactoryConfiguration(const std::string& factoryName) const {
    const auto foundConfig = factoryConfigurations_.find(factoryName);

    if (foundConfig != factoryConfigurations_.end()) {
        return &foundConfig->second;
    }
    return NULL;
}

inline void reset(DBusServiceAddress& dbusServiceAddress) {
    std::get<0>(dbusServiceAddress) = "";
    std::get<1>(dbusServiceAddress) = "";
    std::get<2>(dbusServiceAddress) = "";
}

inline void reset(CommonApiServiceDetails& serviceDetails) {
    reset(std::get<0>(serviceDetails));
    std::get<1>(serviceDetails) = false;
}


inline void reset(DBusFactoryConfig& dbusFactoryconfiguration) {
    dbusFactoryconfiguration.factoryName_ = "";
    dbusFactoryconfiguration.busType_ = BusType::SESSION;
}

void DBusConfiguration::readValue(std::string& readLine, DBusFactoryConfig& factoryConfig) {
    std::stringstream readStream(readLine);
    std::string paramName;
    std::string paramValue;

    getline(readStream, paramName, '=');

    auto typeEntry = allowedValueTypesFactory.find(paramName);
    if (typeEntry != allowedValueTypesFactory.end()) {
        getline(readStream, paramValue);
        switch (typeEntry->second) {
            case TypeEnumFactory::DBUS_BUSTYPE:
                if (paramValue == "system") {
                    factoryConfig.busType_ = DBus::BusType::SYSTEM;
                }
                break;
        }
    }
}

void DBusConfiguration::readValue(std::string& readLine, CommonApiServiceDetails& serviceDetails) {
    std::stringstream readStream(readLine);

    std::string paramName;
    std::string paramValue;

    getline(readStream, paramName, '=');

    auto typeEntry = allowedValueTypes.find(paramName);
    if (typeEntry != allowedValueTypes.end()) {
        getline(readStream, paramValue);
        switch (typeEntry->second) {
            case TypeEnum::DBUS_CONNECTION:
                std::get<0>(std::get<0>(serviceDetails)) = paramValue;
                break;
            case TypeEnum::DBUS_OBJECT:
                std::get<1>(std::get<0>(serviceDetails)) = paramValue;
                break;
            case TypeEnum::DBUS_INTERFACE:
                std::get<2>(std::get<0>(serviceDetails)) = paramValue;
                break;
            case TypeEnum::DBUS_PREDEFINED:
                std::get<1>(serviceDetails) = paramValue == "true" ? true : false;
                break;
        }
    }
}


const FileParsingState isValidSection(const std::string& sectionIdentifier, std::string& newSectionName) {
    const size_t sectionIdentifierLength = sectionIdentifier.length();

    if (sectionIdentifier[0] == '[' && sectionIdentifier[sectionIdentifier.length() - 1] == ']') {
        std::string sectionName = sectionIdentifier.substr(1, sectionIdentifierLength - 2);
        std::vector<std::string> addressParts = split(sectionName, '$');

        if (addressParts.size() == 2 && addressParts[0] == "factory") {
            newSectionName = addressParts[1];
            return FileParsingState::PARSING_FACTORY;
        } else if (isValidCommonApiAddress(sectionName)) {
            newSectionName = sectionName;
            return FileParsingState::PARSING_ADDRESS;
        }
    }
    return FileParsingState::UNDEFINED;
}


void DBusConfiguration::readConfigFile(std::ifstream& addressConfigFile) {
    std::string currentlyParsedCommonApiAddress;
    DBusFactoryConfig currentlyParsedFactoryConfig;
    CommonApiServiceDetails serviceDetails;
    reset(serviceDetails);

    bool newAddressFound = false;
    bool newFactoryFound = false;

    FileParsingState currentParsingState = FileParsingState::UNDEFINED;

    while (addressConfigFile.good()) {
        std::string readLine;
        std::string sectionName;
        getline(addressConfigFile, readLine);

        FileParsingState newState = isValidSection(readLine, sectionName);

        //Finish currently read sections if necessary
        if (newState != FileParsingState::UNDEFINED) {
            switch (currentParsingState) {
                case FileParsingState::PARSING_ADDRESS:
                    if (newAddressFound) {
                        commonApiAddressDetails_.insert( {currentlyParsedCommonApiAddress, serviceDetails});
                    }
                    newAddressFound = false;
                    break;
                case FileParsingState::PARSING_FACTORY:
                    if (newFactoryFound) {
                        factoryConfigurations_.insert( {currentlyParsedFactoryConfig.factoryName_, currentlyParsedFactoryConfig} );
                    }
                    newFactoryFound = false;
                    break;
            }
        }

        //See what comes next
        switch (newState) {
            case FileParsingState::PARSING_ADDRESS:
                reset(serviceDetails);
                currentlyParsedCommonApiAddress = sectionName;
                newAddressFound = commonApiAddressDetails_.find(currentlyParsedCommonApiAddress) == commonApiAddressDetails_.end();
                currentParsingState = FileParsingState::PARSING_ADDRESS;
                break;

            case FileParsingState::PARSING_FACTORY:
                reset(currentlyParsedFactoryConfig);
                currentlyParsedFactoryConfig.factoryName_ = sectionName;
                newFactoryFound = factoryConfigurations_.find(sectionName) == factoryConfigurations_.end();
                currentParsingState = FileParsingState::PARSING_FACTORY;
                break;

            //nothing new, so this lines should contain additional information for the current state
            case FileParsingState::UNDEFINED:
                switch (currentParsingState) {
                    case FileParsingState::PARSING_FACTORY:
                        readValue(readLine, currentlyParsedFactoryConfig);
                        break;
                    case FileParsingState::PARSING_ADDRESS:
                        readValue(readLine, serviceDetails);
                        break;
                }
                break;
        }
    }

    //End of file, finish last section
    if (newAddressFound) {
        commonApiAddressDetails_.insert( {currentlyParsedCommonApiAddress, serviceDetails});
    }
    if (newFactoryFound) {
        factoryConfigurations_.insert( {currentlyParsedFactoryConfig.factoryName_, currentlyParsedFactoryConfig});
    }
}


}// namespace DBus
} // namespace CommonAPI
