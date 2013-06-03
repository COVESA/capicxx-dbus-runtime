/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DBusServicePublisher.h"


namespace CommonAPI {
namespace DBus {


std::shared_ptr<DBusServicePublisher> DBusServicePublisher::getInstance() {
    static std::shared_ptr<DBusServicePublisher> instance;
    if(!instance) {
        instance = std::make_shared<DBusServicePublisher>();
    }
    return instance;
}

bool DBusServicePublisher::registerService(const std::string& serviceAddress, std::shared_ptr<DBusStubAdapter> adapter) {
    return registeredServices_.insert( {serviceAddress, adapter} ).second;
}

bool DBusServicePublisher::unregisterService(const std::string& serviceAddress) {
    auto foundStubAdapter = registeredServices_.find(serviceAddress);
    if(foundStubAdapter != registeredServices_.end()) {
        std::shared_ptr<DBusStubAdapter> stubAdapter = foundStubAdapter->second;
        stubAdapter->deinit();
        return registeredServices_.erase(serviceAddress);
    }
    return false;
}


} // namespace DBus
} // namespace CommonAPI
