/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_ADDRESS_TRANSLATOR_H_
#define COMMONAPI_DBUS_DBUS_ADDRESS_TRANSLATOR_H_

#include <CommonAPI/types.h>

#include "DBusConnectionBusType.h"
#include "DBusFunctionalHash.h"
#include "DBusConfiguration.h"

#include <algorithm>
#include <unordered_map>


namespace CommonAPI {
namespace DBus {


class DBusAddressTranslator {
public:
    ~DBusAddressTranslator();

    static DBusAddressTranslator& getInstance();

    void searchForDBusAddress(const std::string& domain,
                              const std::string& interf,
                              const std::string& instance,
                              std::string& interfaceName,
                              std::string& connectionName,
                              std::string& objectPath);

    void searchForDBusAddress(const std::string& commonApiAddress,
                              std::string& interfaceName,
                              std::string& connectionName,
                              std::string& objectPath);

    void searchForCommonAddress(const std::string& interfaceName,
                                const std::string& connectionName,
                                const std::string& objectPath,
                                std::string& commonApiAddress);

    void getPredefinedInstances(const std::string& connectionName,
                                std::vector<DBusServiceAddress>& instances);

private:
    DBusAddressTranslator();
    DBusAddressTranslator(const DBusAddressTranslator&) = delete;
    DBusAddressTranslator& operator=(const DBusAddressTranslator&) = delete;

    void init();

    void findFallbackDBusAddress(const std::string& instanceId,
                    std::string& interfaceName,
                    std::string& connectionName,
                    std::string& objectPath) const;

    void findFallbackCommonAddress(std::string& instanceId,
                    const std::string& interfaceName,
                    const std::string& connectionName,
                    const std::string& objectPath) const;

    void fillUndefinedValues(CommonApiServiceDetails& serviceDetails, const std::string& commonApiAddress) const;

    std::string transfromObjectPathToInstance(const std::string& path) const;

    std::unordered_map<std::string, CommonApiServiceDetails> commonApiAddressDetails_;
    std::unordered_map<DBusServiceAddress, std::string> dbusToCommonApiAddress_;
};


}// namespace DBus
}// namespace CommonAPI

#endif /* COMMONAPI_DBUS_DBUS_ADDRESS_TRANSLATOR_H_ */
