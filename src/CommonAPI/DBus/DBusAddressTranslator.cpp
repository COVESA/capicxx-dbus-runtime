/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include <CommonAPI/utils.h>

#include "DBusAddressTranslator.h"

#include "DBusConnection.h"
#include "DBusFactory.h"

#include "DBusConnection.h"
#include "DBusFactory.h"

#include <unordered_set>
#include <string.h>
#include <iostream>
#include <fstream>
#include <cassert>
#include <sstream>


namespace CommonAPI {
namespace DBus {


DBusAddressTranslator::DBusAddressTranslator() {}

void DBusAddressTranslator::init() {
    commonApiAddressDetails_ = DBusConfiguration::getInstance().getCommonApiAddressDetails();
    for (auto& addressDetail: commonApiAddressDetails_) {
        fillUndefinedValues(addressDetail.second, addressDetail.first);
        DBusServiceAddress dbusServiceDefinition = std::get<0>(addressDetail.second);
        dbusToCommonApiAddress_.insert( {dbusServiceDefinition, addressDetail.first});
    }
}

DBusAddressTranslator& DBusAddressTranslator::getInstance() {
    static DBusAddressTranslator* dbusAddressTranslator;
    if(!dbusAddressTranslator) {
        dbusAddressTranslator = new DBusAddressTranslator();
        dbusAddressTranslator->init();
    }
    return *dbusAddressTranslator;
}


void DBusAddressTranslator::searchForDBusAddress(const std::string& domain,
                              const std::string& interf,
                              const std::string& instance,
                              std::string& interfaceName,
                              std::string& connectionName,
                              std::string& objectPath) {
    std::stringstream ss;
    ss << domain << ":" << interf << ":" << instance;
    searchForDBusAddress(ss.str(), interfaceName, connectionName, objectPath);
}

void DBusAddressTranslator::searchForDBusAddress(const std::string& commonApiAddress,
                                                 std::string& interfaceName,
                                                 std::string& connectionName,
                                                 std::string& objectPath) {

    const auto& foundAddressMapping = commonApiAddressDetails_.find(commonApiAddress);

    if (foundAddressMapping != commonApiAddressDetails_.end()) {
        connectionName = std::get<0>(std::get<0>(foundAddressMapping->second));
        objectPath = std::get<1>(std::get<0>(foundAddressMapping->second));
        interfaceName = std::get<2>(std::get<0>(foundAddressMapping->second));
    } else {
        findFallbackDBusAddress(commonApiAddress, interfaceName, connectionName, objectPath);
        commonApiAddressDetails_.insert( {commonApiAddress, std::make_tuple(std::make_tuple(connectionName, objectPath, interfaceName), false) } );
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


void DBusAddressTranslator::searchForCommonAddress(const std::string& interfaceName,
                                                   const std::string& connectionName,
                                                   const std::string& objectPath,
                                                   std::string& commonApiAddress) {

    DBusServiceAddress dbusAddress(connectionName, objectPath, interfaceName);

    const auto& foundAddressMapping = dbusToCommonApiAddress_.find(dbusAddress);
    if (foundAddressMapping != dbusToCommonApiAddress_.end()) {
        commonApiAddress = foundAddressMapping->second;
    } else {
        findFallbackCommonAddress(commonApiAddress, interfaceName, connectionName, objectPath);
        dbusToCommonApiAddress_.insert( {std::move(dbusAddress), commonApiAddress} );
    }
}

void DBusAddressTranslator::getPredefinedInstances(const std::string& connectionName,
                                                   std::vector<DBusServiceAddress>& instances) {
    instances.clear();
    auto dbusAddress = commonApiAddressDetails_.begin();
    while (dbusAddress != commonApiAddressDetails_.end()) {
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
    commonApiAddress = "local:" + interfaceName + ":" + transfromObjectPathToInstance(objectPath);
}

std::string DBusAddressTranslator::transfromObjectPathToInstance(const std::string& path) const {
    std::string out = path.substr(1, std::string::npos);
    std::replace(out.begin(), out.end(), '/', '.');
    return out;
}

}// namespace DBus
} // namespace CommonAPI
