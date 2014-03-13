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
                commonApiDomain_(split(commonApiAddress, ':')[0]),
                commonApiServiceId_(split(commonApiAddress, ':')[1]),
                commonApiParticipantId_(split(commonApiAddress, ':')[2]),
                dbusBusName_(dbusBusName),
                dbusObjectPath_(dbusObjectPath),
                dbusInterfaceName_(dbusInterfaceName),
                dbusConnection_(dbusConnection),
                factory_(factory),
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

void DBusStubAdapter::init(std::shared_ptr<DBusStubAdapter> instance) {
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

const bool DBusStubAdapter::hasFreedesktopProperties() {
    return false;
}

const bool DBusStubAdapter::isManagingInterface() {
    return isManagingInterface_;
}

const std::string& DBusStubAdapter::getDBusName() const {
    return dbusBusName_;
}

const std::string& DBusStubAdapter::getObjectPath() const {
    return dbusObjectPath_;
}

const std::string& DBusStubAdapter::getInterfaceName() const {
    return dbusInterfaceName_;
}

const std::shared_ptr<DBusProxyConnection>& DBusStubAdapter::getDBusConnection() const {
    return dbusConnection_;
}

} // namespace dbus
} // namespace CommonAPI
