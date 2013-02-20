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
    DBUS_CONNECTION, DBUS_OBJECT, DBUS_INTERFACE
};

static const std::unordered_map<std::string, TypeEnum> allowedValueTypes = {
                {"dbus_connection", TypeEnum::DBUS_CONNECTION},
                {"dbus_object", TypeEnum::DBUS_OBJECT},
                {"dbus_interface", TypeEnum::DBUS_INTERFACE}
};


inline void readValue(std::string& readLine, DBusServiceAddress& dbusServiceAddress) {
    std::stringstream readStream(readLine);

    std::string paramName;
    std::string paramValue;

    getline(readStream, paramName, '=');

    auto typeEntry = allowedValueTypes.find(paramName);
    if(typeEntry != allowedValueTypes.end()) {
        getline(readStream, paramValue);
        switch(typeEntry->second) {
            case TypeEnum::DBUS_CONNECTION:
                std::get<0>(dbusServiceAddress) = paramValue;
                break;
            case TypeEnum::DBUS_OBJECT:
                std::get<1>(dbusServiceAddress) = paramValue;
                break;
            case TypeEnum::DBUS_INTERFACE:
                std::get<2>(dbusServiceAddress) = paramValue;
                break;
        }
    }
}


inline void reset(DBusServiceAddress& dbusServiceAddress) {
    std::get<0>(dbusServiceAddress) = "";
    std::get<1>(dbusServiceAddress) = "";
    std::get<2>(dbusServiceAddress) = "";
}


void DBusAddressTranslator::fillUndefinedValues(DBusServiceAddress& dbusServiceAddress, const std::string& commonApiAddress) const {
    std::string connectionName;
    std::string objectPath;
    std::string interfaceName;

    findFallbackDBusAddress(commonApiAddress, interfaceName, connectionName, objectPath);

    std::get<0>(dbusServiceAddress) = std::get<0>(dbusServiceAddress) == "" ? connectionName : std::get<0>(dbusServiceAddress);
    std::get<1>(dbusServiceAddress) = std::get<1>(dbusServiceAddress) == "" ? objectPath : std::get<1>(dbusServiceAddress);
    std::get<2>(dbusServiceAddress) = std::get<2>(dbusServiceAddress) == "" ? interfaceName : std::get<2>(dbusServiceAddress);
}


DBusAddressTranslator::DBusAddressTranslator() {}


void DBusAddressTranslator::init() {
    std::string fqnOfConfigFile = getCurrentBinaryFileFQN();
    fqnOfConfigFile += DBUS_CONFIG_SUFFIX;

    std::ifstream addressConfigFile;
    addressConfigFile.open(fqnOfConfigFile.c_str());

    if(addressConfigFile.is_open()) {
        readConfigFile(addressConfigFile);
    } else {
        addressConfigFile.clear();
        addressConfigFile.open(DBUS_GLOBAL_CONFIG_FQN);
        if(addressConfigFile.is_open()) {
            readConfigFile(addressConfigFile);
        }
    }
}


void DBusAddressTranslator::readConfigFile(std::ifstream& addressConfigFile) {
    std::string currentlyParsedCommonApiAddress;
    DBusServiceAddress dbusServiceAddress;
    reset(dbusServiceAddress);

    bool currentAddressNotYetContained = false;
    bool atLeastOneAddressFound = false;

    while (addressConfigFile.good()) {
        std::string readLine;
        getline(addressConfigFile, readLine);
        const size_t readLineLength = readLine.length();

        if (readLine[0] == '[' && readLine[readLineLength - 1] == ']') {
            if (atLeastOneAddressFound) {
                fillUndefinedValues(dbusServiceAddress, currentlyParsedCommonApiAddress);
                knownDBusAddresses.insert( {currentlyParsedCommonApiAddress, dbusServiceAddress});
                knownCommonAddresses.insert( {dbusServiceAddress, currentlyParsedCommonApiAddress});
            }
            reset(dbusServiceAddress);
            currentlyParsedCommonApiAddress = readLine.substr(1, readLineLength - 2);
            currentAddressNotYetContained =
                            knownDBusAddresses.find(currentlyParsedCommonApiAddress) == knownDBusAddresses.end() &&
                            knownCommonAddresses.find(dbusServiceAddress) == knownCommonAddresses.end();
            atLeastOneAddressFound = true;

        } else if (currentAddressNotYetContained) {
            readValue(readLine, dbusServiceAddress);
        }
    }
    if(atLeastOneAddressFound) {
        fillUndefinedValues(dbusServiceAddress, currentlyParsedCommonApiAddress);
        knownDBusAddresses.insert( {currentlyParsedCommonApiAddress, dbusServiceAddress});
        knownCommonAddresses.insert( {dbusServiceAddress, currentlyParsedCommonApiAddress});
    }

    addressConfigFile.close();
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

    const auto& foundAddressMapping = knownDBusAddresses.find(commonApiAddress);

    if(foundAddressMapping != knownDBusAddresses.end()) {
        connectionName = std::get<0>(foundAddressMapping->second);
        objectPath = std::get<1>(foundAddressMapping->second);
        interfaceName = std::get<2>(foundAddressMapping->second);
    } else {
        findFallbackDBusAddress(commonApiAddress, interfaceName, connectionName, objectPath);
        knownDBusAddresses.insert( {commonApiAddress, std::make_tuple(connectionName, objectPath, interfaceName) } );
    }
}

void DBusAddressTranslator::searchForCommonAddress(const std::string& interfaceName,
                                                   const std::string& connectionName,
                                                   const std::string& objectPath,
                                                   std::string& commonApiAddress) {

    DBusServiceAddress dbusAddress(connectionName, objectPath, interfaceName);

    const auto& foundAddressMapping = knownCommonAddresses.find(dbusAddress);
    if (foundAddressMapping != knownCommonAddresses.end()) {
        commonApiAddress = foundAddressMapping->second;
    } else {
        findFallbackCommonAddress(commonApiAddress, interfaceName, connectionName, objectPath);
        knownCommonAddresses.insert( {std::move(dbusAddress), commonApiAddress} );
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
