/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusProxyBase.h"
#include "DBusMessage.h"

namespace CommonAPI {
namespace DBus {

const std::string DBusProxyBase::commonApiDomain_ = "local";

DBusProxyBase::DBusProxyBase(const std::shared_ptr<DBusProxyConnection>& dbusConnection) :
                dbusConnection_(dbusConnection) {
}

DBusMessage DBusProxyBase::createMethodCall(const char* methodName,
                                        const char* methodSignature) const {
    return DBusMessage::createMethodCall(
                    getDBusBusName().c_str(),
                    getDBusObjectPath().c_str(),
                    getInterfaceName().c_str(),
                    methodName,
                    methodSignature);
}

} // namespace DBus
} // namespace CommonAPI
