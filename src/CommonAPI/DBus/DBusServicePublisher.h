/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_SERVICE_PUBLISHER_H_
#define COMMONAPI_DBUS_DBUS_SERVICE_PUBLISHER_H_


#include <CommonAPI/ServicePublisher.h>
#include "DBusStubAdapter.h"


namespace CommonAPI {
namespace DBus {

class DBusFactory;
class DBusObjectManagerStub;

class DBusServicePublisher: public ServicePublisher {
 public:
    DBusServicePublisher() {}

    static std::shared_ptr<DBusServicePublisher> getInstance();

    bool registerService(const std::shared_ptr<DBusStubAdapter>& dbusStubAdapter);
    virtual bool unregisterService(const std::string& serviceAddress);

    bool registerManagedService(const std::shared_ptr<DBusStubAdapter>& managedDBusStubAdapter);
    bool unregisterManagedService(const std::string& serviceAddress);

    std::shared_ptr<DBusStubAdapter> getRegisteredService(const std::string&);

 protected:
    virtual bool registerService(const std::shared_ptr<StubBase>& stubBase,
                                 const char* interfaceId,
                                 const std::string& participantId,
                                 const std::string& serviceName,
                                 const std::string& domain,
                                 const std::shared_ptr<Factory>& factory);

 private:
    typedef std::unordered_map<std::string, std::shared_ptr<DBusStubAdapter>> DBusServicesMap;

    void unregisterManagedService(DBusServicesMap::iterator& managedServiceIterator);

    DBusServicesMap registeredServices_;

    friend class DBusFactory;
};

} // namespace DBus
} // namespace CommonAPI

#endif /* COMMONAPI_DBUS_DBUS_SERVICE_PUBLISHER_H_ */
