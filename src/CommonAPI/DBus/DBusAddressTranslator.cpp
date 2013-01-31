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


namespace CommonAPI {
namespace DBus {





inline bool readSection(std::ifstream& addressConfigFile, std::string& section) {
    std::string readLine;
    getline(addressConfigFile, readLine);

    const size_t sectionLength = readLine.length();

    if(readLine[0] != '[' || readLine[sectionLength - 1] != ']') {
        return false;
    }

    section = readLine.substr(1, sectionLength - 2);

    return true;

//    [domain:service:instance]
//    dbus_connection=connection.name
//    dbus_object=/path/to/object
//    dbus_interface=service.name
}


std::unordered_set<std::string> allowedValueTypes = {"dbus_connection", "dbus_object", "dbus_interface"};


inline bool readValue(std::ifstream& addressConfigFile, std::string& paramName, std::string& paramValue) {
    getline(addressConfigFile, paramName, '=');
    if(allowedValueTypes.find(paramName) == allowedValueTypes.end()) {
        return false;
    }
    getline(addressConfigFile, paramValue);

    //TODO: Check whether its a valid bus name

    return true;
}






DBusAddressTranslator::DBusAddressTranslator()
{
    std::string fqnOfConfigFile = getBinaryFileName();
    fqnOfConfigFile += "_dbus.ini";

    std::ifstream addressConfigFile;

    addressConfigFile.open(fqnOfConfigFile.c_str());

    while (addressConfigFile.good()) {
        std::string section;

        readSection(addressConfigFile, section);
        std::cout << "---" << section<< "---" << std::endl;

        std::string paramName;
        std::string paramValue;

        readValue(addressConfigFile, paramName, paramValue);
        std::cout << paramName << "::" << paramValue << std::endl;
        readValue(addressConfigFile, paramName, paramValue);
        std::cout << paramName << "::" << paramValue << std::endl;
        readValue(addressConfigFile, paramName, paramValue);
        std::cout << paramName << "::" << paramValue << std::endl;
    }

    addressConfigFile.close();
}


DBusAddressTranslator& DBusAddressTranslator::getInstance() {
    static DBusAddressTranslator* dbusNameService_;
    if(!dbusNameService_) {
        dbusNameService_ = new DBusAddressTranslator();
    }
    return *dbusNameService_;
}


void DBusAddressTranslator::searchForDBusInstanceId(const std::string& instanceId,
                                              std::string& connectionName,
                                              std::string& objectPath) const {
    if(!true) {
        findFallbackDBusInstanceId(instanceId, connectionName, objectPath);
    }
}

void DBusAddressTranslator::searchForCommonInstanceId(std::string& instanceId,
                                                const std::string& connectionName,
                                                const std::string& objectPath) const {
    if(!true) {
        findFallbackCommonInstanceId(instanceId, connectionName, objectPath);
    }
}


std::string DBusAddressTranslator::findCommonAPIAddressForDBusAddress(const std::string& conName,
                                               const std::string& objName,
                                               const std::string& intName) const {
    return "local:" + intName + ":" + conName;
}

void DBusAddressTranslator::findFallbackDBusInstanceId(const std::string& instanceId,
                                                 std::string& connectionName,
                                                 std::string& objectPath) const {
    connectionName = instanceId;
    objectPath = '/' + instanceId;
    std::replace(objectPath.begin(), objectPath.end(), '.', '/');
}

void DBusAddressTranslator::findFallbackCommonInstanceId(std::string& instanceId,
                                                   const std::string& connectionName,
                                                   const std::string& objectPath) const {
    instanceId = connectionName;
}


}// namespace DBus
}// namespace CommonAPI
