/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "DBusAddressTranslator.h"

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string.h>


namespace CommonAPI {
namespace DBus {


DBusAddressTranslator::DBusAddressTranslator() {
    char fqnOfBinary[FILENAME_MAX];
    char pathToProcessImage[FILENAME_MAX];

    sprintf(pathToProcessImage, "/proc/%ld/exe", getpid());
    unsigned int lengthOfFqn;
    lengthOfFqn = readlink(pathToProcessImage, fqnOfBinary, sizeof(fqnOfBinary) - 1);

    if (lengthOfFqn != -1) {
        fqnOfBinary[lengthOfFqn] = '\0';
        std::string fqnOfConfigFile(std::move(fqnOfBinary));
        fqnOfConfigFile += "_dbus.ini";

        std::ifstream addressConfigFile;

        addressConfigFile.open(fqnOfConfigFile.c_str());

        std::string readLine;
        while (addressConfigFile.good()) {
            std::getline(addressConfigFile, readLine);
            std::cout << readLine << std::endl;
        }

        addressConfigFile.close();
    }
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
