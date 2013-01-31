/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef DBUSNAMESERVICE_H_
#define DBUSNAMESERVICE_H_


#include <algorithm>


namespace CommonAPI {
namespace DBus {


//connectionName, objectPath
typedef std::pair<std::string, std::string> DBusServiceInstanceId;


class DBusAddressTranslator {
public:
    ~DBusAddressTranslator();

    std::string findCommonAPIAddressForDBusAddress(const std::string& conName, const std::string& objName, const std::string& intName) const;

    void searchForDBusInstanceId(const std::string& instanceId, std::string& connectionName, std::string& objectPath) const;
    void searchForCommonInstanceId(std::string& instanceId, const std::string& connectionName, const std::string& objectPath) const;

    static DBusAddressTranslator& getInstance();

private:
    DBusAddressTranslator();
    DBusAddressTranslator(const DBusAddressTranslator&) = delete;
    DBusAddressTranslator& operator=(const DBusAddressTranslator&) = delete;

    void findFallbackDBusInstanceId(const std::string& instanceId, std::string& connectionName, std::string& objectPath) const;
    void findFallbackCommonInstanceId(std::string& instanceId, const std::string& connectionName, const std::string& objectPath) const;
};


}// namespace DBus
}// namespace CommonAPI

#endif /* DBUSNAMESERVICE_H_ */
