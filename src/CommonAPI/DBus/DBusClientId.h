/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef DBUSCLIENTID_H_
#define DBUSCLIENTID_H_

#include <CommonAPI/types.h>
#include <string>

namespace CommonAPI {
namespace DBus {

class DBusMessage;

/**
 * \brief Implementation of CommonAPI::ClientId for DBus
 *
 * This class represents the DBus specific implementation of CommonAPI::ClientId.
 * It internally uses a string to identify clients. This string is the unique sender id used by dbus.
 */
class DBusClientId: public CommonAPI::ClientId {
    friend class std::hash<DBusClientId>;

public:
    DBusClientId(std::string dbusId);

    bool operator==(CommonAPI::ClientId& clientIdToCompare);
    bool operator==(DBusClientId& clientIdToCompare);
    size_t hashCode();

    const char * getDBusId();

    DBusMessage createMessage(const std::string objectPath, const std::string interfaceName, const std::string signalName) const;
protected:
    std::string dbusId_;
};

} /* namespace DBus */
} /* namespace CommonAPI */
#endif /* DBUSCLIENTID_H_ */
