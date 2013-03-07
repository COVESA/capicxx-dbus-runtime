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

DBusProxyBase::DBusProxyBase(const std::string& commonApiServiceId,
                             const std::string& commonApiParticipantId,
                             const std::string& dbusInterfaceName,
                             const std::string& dbusBusName,
                             const std::string& dbusObjectPath,
                             const std::shared_ptr<DBusProxyConnection>& dbusConnection) :
                commonApiServiceId_(commonApiServiceId),
                commonApiParticipantId_(commonApiParticipantId),
                dbusBusName_(dbusBusName),
                dbusObjectPath_(dbusObjectPath),
                dbusInterfaceName_(dbusInterfaceName),
                dbusConnection_(dbusConnection) {
}

DBusProxyBase::DBusProxyBase(const std::string& dbusInterfaceName,
                             const std::string& dbusBusName,
                             const std::string& dbusObjectPath,
                             const std::shared_ptr<DBusProxyConnection>& dbusConnection) :
                commonApiServiceId_(dbusInterfaceName),
                commonApiParticipantId_(dbusBusName + "-" + dbusObjectPath),
                dbusBusName_(dbusBusName),
                dbusObjectPath_(dbusObjectPath),
                dbusInterfaceName_(dbusInterfaceName),
                dbusConnection_(dbusConnection) {
}

std::string DBusProxyBase::getAddress() const {
    return commonApiDomain_ + ":" + commonApiServiceId_ + ":" + commonApiParticipantId_;
}

const std::string& DBusProxyBase::getDomain() const {
    return commonApiDomain_;
}

const std::string& DBusProxyBase::getServiceId() const {
    return commonApiServiceId_;
}

const std::string& DBusProxyBase::getInstanceId() const {
    return commonApiParticipantId_;
}

DBusMessage DBusProxyBase::createMethodCall(const char* methodName,
                                        const char* methodSignature) const {
    return DBusMessage::createMethodCall(
                    dbusBusName_.c_str(),
                    dbusObjectPath_.c_str(),
                    dbusInterfaceName_.c_str(),
                    methodName,
                    methodSignature);
}

} // namespace DBus
} // namespace CommonAPI
