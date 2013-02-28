/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "DBusAddressTranslator.h"
#include "DBusUtils.h"

#include <unordered_set>
#include <string.h>
#include <iostream>
#include <fstream>
#include <cassert>


namespace CommonAPI {
namespace DBus {


enum class TypeEnum {
    DBUS_CONNECTION, DBUS_OBJECT, DBUS_INTERFACE, DBUS_PREDEFINED
};

static const std::unordered_map<std::string, TypeEnum> allowedValueTypes = {
                {"dbus_connection", TypeEnum::DBUS_CONNECTION},
                {"dbus_object", TypeEnum::DBUS_OBJECT},
                {"dbus_interface", TypeEnum::DBUS_INTERFACE},
                {"dbus_predefined", TypeEnum::DBUS_PREDEFINED}
};


DBusAddressTranslator::DBusAddressTranslator() {}


void DBusAddressTranslator::init() {
    std::string fqnOfConfigFile = getCurrentBinaryFileFQN();
    fqnOfConfigFile += DBUS_CONFIG_SUFFIX;

    std::ifstream addressConfigFile;
    addressConfigFile.open(fqnOfConfigFile.c_str());

    if(addressConfigFile.is_open()) {
        readConfigFile(addressConfigFile);
        addressConfigFile.close();
    }

    addressConfigFile.clear();
    std::vector<std::string> splittedConfigFQN = split(fqnOfConfigFile, '/');
    std::string globalConfigFQN = DBUS_GLOBAL_CONFIG_ROOT + splittedConfigFQN.at(splittedConfigFQN.size() - 1);
    addressConfigFile.open(globalConfigFQN);
    if(addressConfigFile.is_open()) {
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


inline void readValue(std::string& readLine, CommonApiServiceDetails& serviceDetails) {
    std::stringstream readStream(readLine);

    std::string paramName;
    std::string paramValue;

    getline(readStream, paramName, '=');

    auto typeEntry = allowedValueTypes.find(paramName);
    if(typeEntry != allowedValueTypes.end()) {
        getline(readStream, paramValue);
        switch(typeEntry->second) {
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


inline void reset(DBusServiceAddress& dbusServiceAddress) {
    std::get<0>(dbusServiceAddress) = "";
    std::get<1>(dbusServiceAddress) = "";
    std::get<2>(dbusServiceAddress) = "";
}

inline void reset(CommonApiServiceDetails& serviceDetails) {
    reset(std::get<0>(serviceDetails));
    std::get<1>(serviceDetails) = false;
}


void DBusAddressTranslator::readConfigFile(std::ifstream& addressConfigFile) {
    std::string currentlyParsedCommonApiAddress;
    CommonApiServiceDetails serviceDetails;
    reset(serviceDetails);

    bool newAddressFound = false;

    while (addressConfigFile.good()) {
        std::string readLine;
        getline(addressConfigFile, readLine);
        const size_t readLineLength = readLine.length();

        if (readLine[0] == '[' && readLine[readLineLength - 1] == ']') {
            if (newAddressFound) {
                fillUndefinedValues(serviceDetails, currentlyParsedCommonApiAddress);
                commonApiAddressDetails.insert( {currentlyParsedCommonApiAddress, serviceDetails});
                dbusToCommonApiAddress.insert( {std::get<0>(serviceDetails), currentlyParsedCommonApiAddress});
            }
            reset(serviceDetails);
            currentlyParsedCommonApiAddress = readLine.substr(1, readLineLength - 2);
            newAddressFound = commonApiAddressDetails.find(currentlyParsedCommonApiAddress) == commonApiAddressDetails.end();

        } else if (newAddressFound) {
            readValue(readLine, serviceDetails);
        }
    }
    if(newAddressFound) {
        fillUndefinedValues(serviceDetails, currentlyParsedCommonApiAddress);
        commonApiAddressDetails.insert( {currentlyParsedCommonApiAddress, serviceDetails});
        dbusToCommonApiAddress.insert( {std::get<0>(serviceDetails), currentlyParsedCommonApiAddress});
    }
}


void DBusAddressTranslator::fillUndefinedValues(CommonApiServiceDetails& serviceDetails, const std::string& commonApiAddress) const {
    std::string connectionName;
    std::string objectPath;
    std::string interfaceName;

    findFallbackDBusAddress(commonApiAddress, interfaceName, connectionName, objectPath);

    std::get<0>(std::get<0>(serviceDetails)) = std::get<0>(std::get<0>(serviceDetails)) == "" ? connectionName : std::get<0>(std::get<0>(serviceDetails));
    std::get<1>(std::get<0>(serviceDetails)) = std::get<1>(std::get<0>(serviceDetails)) == "" ? objectPath : std::get<1>(std::get<0>(serviceDetails));
    std::get<2>(std::get<0>(serviceDetails)) = std::get<2>(std::get<0>(serviceDetails)) == "" ? interfaceName : std::get<2>(std::get<0>(serviceDetails));
}


DBusAddressTranslator& DBusAddressTranslator::getInstance() {
    static DBusAddressTranslator* dbusNameService_;
    if(!dbusNameService_) {
        dbusNameService_ = new DBusAddressTranslator();
        dbusNameService_->init();
    }
    return *dbusNameService_;
}


void DBusAddressTranslator::searchForDBusAddress(const std::string& commonApiAddress,
                                                 std::string& interfaceName,
                                                 std::string& connectionName,
                                                 std::string& objectPath) {

    const auto& foundAddressMapping = commonApiAddressDetails.find(commonApiAddress);

    if(foundAddressMapping != commonApiAddressDetails.end()) {
        connectionName = std::get<0>(std::get<0>(foundAddressMapping->second));
        objectPath = std::get<1>(std::get<0>(foundAddressMapping->second));
        interfaceName = std::get<2>(std::get<0>(foundAddressMapping->second));
    } else {
        findFallbackDBusAddress(commonApiAddress, interfaceName, connectionName, objectPath);
        commonApiAddressDetails.insert( {commonApiAddress, std::make_tuple(std::make_tuple(connectionName, objectPath, interfaceName), false) } );
    }
}

void DBusAddressTranslator::searchForCommonAddress(const std::string& interfaceName,
                                                   const std::string& connectionName,
                                                   const std::string& objectPath,
                                                   std::string& commonApiAddress) {

    DBusServiceAddress dbusAddress(connectionName, objectPath, interfaceName);

    const auto& foundAddressMapping = dbusToCommonApiAddress.find(dbusAddress);
    if (foundAddressMapping != dbusToCommonApiAddress.end()) {
        commonApiAddress = foundAddressMapping->second;
    } else {
        findFallbackCommonAddress(commonApiAddress, interfaceName, connectionName, objectPath);
        dbusToCommonApiAddress.insert( {std::move(dbusAddress), commonApiAddress} );
    }
}

void DBusAddressTranslator::getPredefinedInstances(const std::string& connectionName,
                                                   std::vector<DBusServiceAddress>& instances) {
    instances.clear();
    auto dbusAddress = commonApiAddressDetails.begin();
    while (dbusAddress != commonApiAddressDetails.end()) {
        CommonApiServiceDetails serviceDetails = dbusAddress->second;
        if (connectionName == std::get<0>(std::get<0>(serviceDetails))
                        && true == std::get<1>(serviceDetails)) {
            instances.push_back(std::get<0>(serviceDetails));
        }
        dbusAddress++;
    }
}

void DBusAddressTranslator::findFallbackDBusAddress(const std::string& commonApiAddress,
                                                    std::string& interfaceName,
                                                    std::string& connectionName,
                                                    std::string& objectPath) const {
    std::vector<std::string> parts = split(commonApiAddress, ':');
    interfaceName = parts[1];
    connectionName = parts[2];
    objectPath = '/' + parts[2];
    std::replace(objectPath.begin(), objectPath.end(), '.', '/');
}

void DBusAddressTranslator::findFallbackCommonAddress(std::string& commonApiAddress,
                                                      const std::string& interfaceName,
                                                      const std::string& connectionName,
                                                      const std::string& objectPath) const {
    commonApiAddress = "local:" + interfaceName + ":" + connectionName;
}


}// namespace DBus
}// namespace CommonAPI
