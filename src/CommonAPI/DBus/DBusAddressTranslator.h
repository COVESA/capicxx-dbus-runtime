/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef DBUSNAMESERVICE_H_
#define DBUSNAMESERVICE_H_

#include "DBusFunctionalHash.h"

#include <CommonAPI/types.h>
#include "DBusConnectionBusType.h"

#include <algorithm>
#include <unordered_map>


namespace CommonAPI {
namespace DBus {


static const char* DBUS_CONFIG_SUFFIX = "_dbus.conf";
static const char* DBUS_GLOBAL_CONFIG_FQN = "/etc/CommonApiDBus/dbusAddresses.conf";
static const char* DBUS_GLOBAL_CONFIG_ROOT = "/etc/CommonApiDBus/";


//connectionName, objectPath, interfaceName
typedef std::tuple<std::string, std::string, std::string> DBusServiceAddress;

//Details for a common api address key: DBusAddress, predefined service
typedef std::tuple<DBusServiceAddress, bool> CommonApiServiceDetails;


class DBusAddressTranslator {
public:
    struct FactoryConfigDBus {
        std::string factoryName;
        BusType busType;
    };

    ~DBusAddressTranslator();

    static DBusAddressTranslator& getInstance();

    void searchForDBusAddress(const std::string& commonApiAddress,
                              std::string& interfaceName,
                              std::string& connectionName,
                              std::string& objectPath);

    void searchForCommonAddress(const std::string& interfaceName,
                                const std::string& connectionName,
                                const std::string& objectPath,
                                std::string& commonApiAddress);

    FactoryConfigDBus* searchForFactoryConfiguration(const std::string& factoryName);

    void getPredefinedInstances(const std::string& connectionName,
                                   std::vector<DBusServiceAddress>& instances);

private:
    DBusAddressTranslator();
    DBusAddressTranslator(const DBusAddressTranslator&) = delete;
    DBusAddressTranslator& operator=(const DBusAddressTranslator&) = delete;

    void init();
    void readConfigFile(std::ifstream& addressConfigFile);

    void findFallbackDBusAddress(const std::string& instanceId,
                                    std::string& interfaceName,
                                    std::string& connectionName,
                                    std::string& objectPath) const;

    void findFallbackCommonAddress(std::string& instanceId,
                                      const std::string& interfaceName,
                                      const std::string& connectionName,
                                      const std::string& objectPath) const;

    void fillUndefinedValues(CommonApiServiceDetails& dbusServiceAddress, const std::string& commonApiAddress) const;

    std::unordered_map<std::string, CommonApiServiceDetails> commonApiAddressDetails;
    std::unordered_map<DBusServiceAddress, std::string> dbusToCommonApiAddress;
    std::unordered_map<std::string, DBusAddressTranslator::FactoryConfigDBus> factoryConfigurations;

 };


}// namespace DBus
}// namespace CommonAPI

#endif /* DBUSNAMESERVICE_H_ */
