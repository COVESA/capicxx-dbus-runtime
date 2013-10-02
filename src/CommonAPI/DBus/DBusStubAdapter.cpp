/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusStubAdapter.h"
#include "DBusUtils.h"

#include <CommonAPI/utils.h>

#include <cassert>
#include <functional>
#include <sstream>

namespace CommonAPI {
namespace DBus {

const std::string DBusStubAdapter::domain_ = "local";

DBusStubAdapter::DBusStubAdapter(const std::shared_ptr<DBusFactory>& factory,
                                 const std::string& commonApiAddress,
                                 const std::string& dbusInterfaceName,
                                 const std::string& dbusBusName,
                                 const std::string& dbusObjectPath,
                                 const std::shared_ptr<DBusProxyConnection>& dbusConnection,
                                 const bool isManagingInterface) :
                factory_(factory),
                commonApiDomain_(split(commonApiAddress, ':')[0]),
                commonApiServiceId_(split(commonApiAddress, ':')[1]),
                commonApiParticipantId_(split(commonApiAddress, ':')[2]),
                dbusInterfaceName_(dbusInterfaceName),
                dbusBusName_(dbusBusName),
                dbusObjectPath_(dbusObjectPath),
                dbusConnection_(dbusConnection),
                isManagingInterface_(isManagingInterface) {

    assert(!dbusBusName_.empty());
    assert(!dbusInterfaceName_.empty());
    assert(!dbusObjectPath_.empty());
    assert(dbusObjectPath_[0] == '/');
    assert(!dbusInterfaceName_.empty());
    assert(dbusConnection_);
}

DBusStubAdapter::~DBusStubAdapter() {
    deinit();
}

void DBusStubAdapter::init() {
}

void DBusStubAdapter::deinit() {
}

const std::string DBusStubAdapter::getAddress() const {
    return commonApiDomain_ + ":" + commonApiServiceId_ + ":" + commonApiParticipantId_;
}

const std::string& DBusStubAdapter::getDomain() const {
    return commonApiDomain_;
}

const std::string& DBusStubAdapter::getServiceId() const {
    return commonApiServiceId_;
}

const std::string& DBusStubAdapter::getInstanceId() const {
    return commonApiParticipantId_;
}

} // namespace dbus
} // namespace CommonAPI
