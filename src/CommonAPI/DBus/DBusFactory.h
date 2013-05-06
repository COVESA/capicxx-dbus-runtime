/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_DBUS_DBUS_FACTORY_H_
#define COMMONAPI_DBUS_DBUS_FACTORY_H_

#include <thread>

#include <CommonAPI/Factory.h>

#include "CommonAPI/DBus/DBusStubAdapter.h"
#include "DBusConnection.h"
#include "DBusProxy.h"

namespace CommonAPI {
namespace DBus {

class DBusMainLoopContext;

typedef std::shared_ptr<DBusProxy> (*DBusProxyFactoryFunction) (const std::string& commonApiAddress,
                                                                const std::string& interfaceName,
                                                                const std::string& busName,
                                                                const std::string& objectPath,
                                                                const std::shared_ptr<DBusProxyConnection>& dbusProxyConnection);

typedef std::shared_ptr<DBusStubAdapter> (*DBusAdapterFactoryFunction) (const std::string& commonApiAddress,
                                                                        const std::string& interfaceName,
                                                                        const std::string& busName,
                                                                        const std::string& objectPath,
                                                                        const std::shared_ptr<DBusProxyConnection>& dbusProxyConnection,
                                                                        const std::shared_ptr<StubBase>& stubBase);

class DBusFactory: public Factory {
 public:
    DBusFactory(std::shared_ptr<Runtime> runtime, const MiddlewareInfo* middlewareInfo, std::shared_ptr<MainLoopContext> mainLoopContext = std::shared_ptr<MainLoopContext>(NULL));
    virtual ~DBusFactory();

    static void registerProxyFactoryMethod(std::string interfaceName, DBusProxyFactoryFunction proxyFactoryFunction);
    static void registerAdapterFactoryMethod(std::string interfaceName, DBusAdapterFactoryFunction adapterFactoryMethod);

    virtual std::vector<std::string> getAvailableServiceInstances(const std::string& serviceInterfaceName, const std::string& serviceDomainName = "local");

    virtual bool isServiceInstanceAlive(const std::string& serviceAddress);
    virtual bool isServiceInstanceAlive(const std::string& participantId, const std::string& serviceName, const std::string& domain = "local");

    virtual bool unregisterService(const std::string& participantId, const std::string& serviceName, const std::string& domain = "local");

 protected:
    virtual std::shared_ptr<Proxy> createProxy(const char* interfaceId, const std::string& participantId, const std::string& serviceName, const std::string& domain);
    virtual bool registerAdapter(std::shared_ptr<StubBase> stubBase, const char* interfaceId, const std::string& participantId, const std::string& serviceName, const std::string& domain);

 private:
    std::shared_ptr<CommonAPI::DBus::DBusConnection> dbusConnection_;
    std::string acquiredConnectionName_;
    std::unordered_map<std::string, std::shared_ptr<DBusStubAdapter>> registeredServices_;
    std::shared_ptr<MainLoopContext> mainLoopContext_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_FACTORY_H_
