/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DBusProxy.h"
#include "DBusConnection.h"
#include "DBusFactory.h"
#include "DBusServiceRegistry.h"
#include "DBusUtils.h"
#include "DBusServicePublisher.h"

#include <algorithm>
#include <cassert>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace CommonAPI {
namespace DBus {


std::unordered_map<std::string, DBusProxyFactoryFunction>* registeredProxyFactoryFunctions_;
std::unordered_map<std::string, DBusAdapterFactoryFunction>* registeredAdapterFactoryFunctions_;


void DBusFactory::registerProxyFactoryMethod(std::string interfaceName, DBusProxyFactoryFunction proxyFactoryMethod) {
    if(!registeredProxyFactoryFunctions_) {
        registeredProxyFactoryFunctions_ = new std::unordered_map<std::string, DBusProxyFactoryFunction>();
    }
    registeredProxyFactoryFunctions_->insert({interfaceName, proxyFactoryMethod});
}

void DBusFactory::registerAdapterFactoryMethod(std::string interfaceName, DBusAdapterFactoryFunction adapterFactoryMethod) {
    if(!registeredAdapterFactoryFunctions_) {
        registeredAdapterFactoryFunctions_ = new std::unordered_map<std::string, DBusAdapterFactoryFunction>();
    }
    registeredAdapterFactoryFunctions_->insert({interfaceName, adapterFactoryMethod});
}


DBusFactory::DBusFactory(std::shared_ptr<Runtime> runtime,
                         const MiddlewareInfo* middlewareInfo,
                         std::shared_ptr<MainLoopContext> mainLoopContext,
                         const DBusFactoryConfig& DBusFactoryConfig) :
                CommonAPI::Factory(runtime, middlewareInfo),
                mainLoopContext_(mainLoopContext),
                DBusFactoryConfig_(DBusFactoryConfig){

    dbusConnection_ = CommonAPI::DBus::DBusConnection::getBus(DBusFactoryConfig_.busType_);
    bool startDispatchThread = !mainLoopContext_;
    dbusConnection_->connect(startDispatchThread);
    if (mainLoopContext_) {
        dbusConnection_->attachMainLoopContext(mainLoopContext_);
    }
}


DBusFactory::~DBusFactory() {
}


std::vector<std::string> DBusFactory::getAvailableServiceInstances(const std::string& serviceName,
                                                                   const std::string& domainName) {
    return dbusConnection_->getDBusServiceRegistry()->getAvailableServiceInstances(serviceName, domainName);
}


void DBusFactory::getAvailableServiceInstancesAsync(Factory::GetAvailableServiceInstancesCallback callback, const std::string& serviceName, const std::string& serviceDomainName) {
    dbusConnection_->getDBusServiceRegistry()->getAvailableServiceInstancesAsync(callback, serviceName, serviceDomainName);
}


bool DBusFactory::isServiceInstanceAlive(const std::string& serviceAddress) {
    std::vector<std::string> parts = split(serviceAddress, ':');
    assert(parts[0] == "local");

    std::string interfaceName;
    std::string connectionName;
    std::string objectPath;
    DBusAddressTranslator::getInstance().searchForDBusAddress(serviceAddress, interfaceName, connectionName, objectPath);

    return dbusConnection_->getDBusServiceRegistry()->isServiceInstanceAlive(interfaceName, connectionName, objectPath);
}


bool DBusFactory::isServiceInstanceAlive(const std::string& participantId,
                                         const std::string& serviceName,
                                         const std::string& domainName) {
    std::string serviceAddress = domainName + ":" + serviceName + ":" + participantId;
    return isServiceInstanceAlive(serviceAddress);
}


SubscriptionStatus DBusFactory::isServiceInstanceAliveCallbackThunk(Factory::IsServiceInstanceAliveCallback callback, const AvailabilityStatus& status, std::shared_ptr<DBusServiceRegistry> serviceRegistry) {
    callback(status == AvailabilityStatus::AVAILABLE);
    return SubscriptionStatus::CANCEL;
}

void DBusFactory::isServiceInstanceAliveAsync(Factory::IsServiceInstanceAliveCallback callback, const std::string& serviceAddress) {
    std::string interfaceName;
    std::string connectionName;
    std::string objectPath;

    DBusAddressTranslator::getInstance().searchForDBusAddress(serviceAddress, interfaceName, connectionName, objectPath);

    std::shared_ptr<DBusServiceRegistry> serviceRegistry = dbusConnection_->getDBusServiceRegistry();

    serviceRegistry->subscribeAvailabilityListener(
                    serviceAddress,
                    std::bind(&DBusFactory::isServiceInstanceAliveCallbackThunk,
                              this,
                              callback,
                              std::placeholders::_1,
                              serviceRegistry)
    );
}

std::shared_ptr<CommonAPI::DBus::DBusConnection> DBusFactory::getDbusConnection() {
    return dbusConnection_;
}

void DBusFactory::isServiceInstanceAliveAsync(Factory::IsServiceInstanceAliveCallback callback,
                                              const std::string& serviceInstanceID,
                                              const std::string& serviceName,
                                              const std::string& serviceDomainName) {
    std::string commonApiAddress = serviceDomainName + ":" + serviceName + ":" + serviceInstanceID;
    isServiceInstanceAliveAsync(callback, commonApiAddress);
}


std::shared_ptr<Proxy> DBusFactory::createProxy(const char* interfaceId,
                                                const std::string& participantId,
                                                const std::string& serviceName,
                                                const std::string& domain) {
    std::string commonApiAddress = domain + ":" + serviceName + ":" + participantId;

    std::string interfaceName;
    std::string connectionName;
    std::string objectPath;

    DBusAddressTranslator::getInstance().searchForDBusAddress(commonApiAddress, interfaceName, connectionName, objectPath);

    if(!registeredProxyFactoryFunctions_) {
        registeredProxyFactoryFunctions_ = new std::unordered_map<std::string, DBusProxyFactoryFunction> {};
    }

    for (auto it = registeredProxyFactoryFunctions_->begin(); it != registeredProxyFactoryFunctions_->end(); ++it) {
        if(it->first == interfaceId) {
            std::shared_ptr<DBusProxy> proxy = (it->second)(shared_from_this(), commonApiAddress, interfaceName, connectionName, objectPath, dbusConnection_);
            proxy->init();
            return proxy;
        }
    }

    return NULL;
}

std::shared_ptr<DBusStubAdapter> DBusFactory::createDBusStubAdapter(const std::shared_ptr<StubBase>& stubBase,
                                                                    const char* interfaceId,
                                                                    const std::string& participantId,
                                                                    const std::string& serviceName,
                                                                    const std::string& domain) {
    assert(dbusConnection_->isConnected());

    std::string commonApiAddress = domain + ":" + serviceName + ":" + participantId;

    std::string interfaceName;
    std::string connectionName;
    std::string objectPath;

    DBusAddressTranslator::getInstance().searchForDBusAddress(commonApiAddress, interfaceName, connectionName, objectPath);

    if (!registeredAdapterFactoryFunctions_) {
        registeredAdapterFactoryFunctions_ = new std::unordered_map<std::string, DBusAdapterFactoryFunction> {};
    }

    auto registeredAdapterFactoryFunctionsIter = registeredAdapterFactoryFunctions_->find(interfaceId);
    const bool hasRegisteredAdapterFactoryFunctions = (registeredAdapterFactoryFunctionsIter != registeredAdapterFactoryFunctions_->end());
    std::shared_ptr<DBusStubAdapter> dbusStubAdapter;

    if (hasRegisteredAdapterFactoryFunctions) {
        const auto& dbusAdapterFactoryFunction = registeredAdapterFactoryFunctionsIter->second;

        dbusStubAdapter = dbusAdapterFactoryFunction(shared_from_this(), commonApiAddress, interfaceName, connectionName, objectPath, dbusConnection_, stubBase);
        dbusStubAdapter->init();
    }

    return dbusStubAdapter;
}

bool DBusFactory::unregisterService(const std::string& participantId, const std::string& serviceName, const std::string& domain) {
    std::string serviceAddress(domain + ":" + serviceName + ":" + participantId);
    return DBusServicePublisher::getInstance()->unregisterService(serviceAddress);
}

COMMONAPI_DEPRECATED bool DBusFactory::registerAdapter(std::shared_ptr<StubBase> stubBase,
                                                       const char* interfaceId,
                                                       const std::string& participantId,
                                                       const std::string& serviceName,
                                                       const std::string& domain) {

    std::shared_ptr<DBusServicePublisher> pub = std::dynamic_pointer_cast<DBusServicePublisher>(runtime_->getServicePublisher());
    return pub->registerService(
                    stubBase,
                    interfaceId,
                    participantId,
                    serviceName,
                    domain,
                    shared_from_this());
}


} // namespace DBus
} // namespace CommonAPI
