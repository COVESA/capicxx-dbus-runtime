/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_CONFIGURATION_H_
#define COMMONAPI_DBUS_DBUS_CONFIGURATION_H_


#include <unordered_map>

#include "DBusConnectionBusType.h"
#include "DBusFunctionalHash.h"


namespace CommonAPI {
namespace DBus {

static const char DBUS_CONFIG_SUFFIX[] = "_dbus.conf";
static const char DBUS_GLOBAL_CONFIG_FQN[] = "/etc/CommonApiDBus/dbusAddresses.conf";
static const char DBUS_GLOBAL_CONFIG_ROOT[] = "/etc/CommonApiDBus/";

//connectionName, objectPath, interfaceName
typedef std::tuple<std::string, std::string, std::string> DBusServiceAddress;

//Details for a common api address key: DBusAddress, predefined service
typedef std::tuple<DBusServiceAddress, bool> CommonApiServiceDetails;


struct DBusFactoryConfig {
    DBusFactoryConfig(): factoryName_(""), busType_(BusType::SESSION) {}

    std::string factoryName_;
    BusType busType_;
};


class DBusConfiguration {
 public:
    static const DBusConfiguration& getInstance();

    DBusConfiguration(const DBusConfiguration&) = delete;
    DBusConfiguration& operator=(const DBusConfiguration&) = delete;
    DBusConfiguration(DBusConfiguration&&) = delete;
    DBusConfiguration& operator=(DBusConfiguration&&) = delete;

    const std::unordered_map<std::string, CommonApiServiceDetails>& getCommonApiAddressDetails() const;

    const DBusFactoryConfig* getFactoryConfiguration(const std::string& factoryName) const;

 private:
    DBusConfiguration() = default;

    void readConfigFile(std::ifstream& addressConfigFile);
    void retrieveCommonApiDBusConfiguration();

    void readValue(std::string& readLine, DBusFactoryConfig& factoryConfig);
    void readValue(std::string& readLine, CommonApiServiceDetails& serviceDetails);

    std::unordered_map<std::string, CommonApiServiceDetails> commonApiAddressDetails_;
    std::unordered_map<std::string, DBusFactoryConfig> factoryConfigurations_;
};


}// namespace DBus
}// namespace CommonAPI

#endif /* COMMONAPI_DBUS_DBUS_CONFIGURATION_H_ */
