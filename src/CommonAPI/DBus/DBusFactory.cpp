/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusProxy.h"
#include "DBusConnection.h"
#include "DBusFactory.h"
#include "DBusNameService.h"
#include "DBusServiceRegistry.h"
#include "DBusUtils.h"

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



DBusFactory::DBusFactory(std::shared_ptr<Runtime> runtime, const MiddlewareInfo* middlewareInfo) :
                CommonAPI::Factory(runtime, middlewareInfo),
                dbusConnection_(CommonAPI::DBus::DBusConnection::getSessionBus()),
                acquiredConnectionName_("") {
    dbusConnection_->connect();
}

DBusFactory::~DBusFactory() {
}


std::vector<std::string> DBusFactory::getAvailableServiceInstances(const std::string& serviceInterfaceName,
                                                                   const std::string& serviceDomainName) {
    return dbusConnection_->getDBusServiceRegistry()->getAvailableServiceInstances(serviceInterfaceName, serviceDomainName);
}


bool DBusFactory::isServiceInstanceAlive(const std::string& serviceInstanceId,
                                         const std::string& serviceInterfaceName,
                                         const std::string& serviceDomainName) {

    return dbusConnection_->getDBusServiceRegistry()->isServiceInstanceAlive(serviceInstanceId, serviceInterfaceName, serviceDomainName);
}

std::shared_ptr<Proxy> DBusFactory::createProxy(const char* interfaceName, const std::string& participantId, const std::string& domain) {
    std::string connectionName;
    std::string objectPath;

    DBusNameService::getInstance().searchForDBusInstanceId(participantId, connectionName, objectPath);

    if(!registeredProxyFactoryFunctions_) {
        registeredProxyFactoryFunctions_ = new std::unordered_map<std::string, DBusProxyFactoryFunction> {};
    }

    for (auto it = registeredProxyFactoryFunctions_->begin(); it != registeredProxyFactoryFunctions_->end(); ++it) {
        if(it->first == interfaceName) {
            return (it->second)(connectionName.c_str(), objectPath.c_str(), dbusConnection_);
        }
    }

    return NULL;
}

std::shared_ptr<StubAdapter> DBusFactory::createAdapter(std::shared_ptr<StubBase> stubBase, const char* interfaceName, const std::string& participantId, const std::string& domain) {
    assert(dbusConnection_->isConnected());

    std::string connectionName;
    std::string objectPath;

    DBusNameService::getInstance().searchForDBusInstanceId(participantId, connectionName, objectPath);

    if(acquiredConnectionName_ == "") {
        dbusConnection_->requestServiceNameAndBlock(connectionName);
        acquiredConnectionName_ = connectionName;
    } else if (acquiredConnectionName_ != connectionName) {
        return NULL;
    }

    if(!registeredAdapterFactoryFunctions_) {
        registeredAdapterFactoryFunctions_ = new std::unordered_map<std::string, DBusAdapterFactoryFunction> {};
    }

    for (auto it = registeredAdapterFactoryFunctions_->begin(); it != registeredAdapterFactoryFunctions_->end(); ++it) {
        if(it->first == interfaceName) {
            std::shared_ptr<DBusStubAdapter> dbusStubAdapter =  (it->second)(connectionName.c_str(), objectPath.c_str(), dbusConnection_, stubBase);
            dbusStubAdapter->init();
            return dbusStubAdapter;
        }
    }

    return NULL;
}


} // namespace DBus
} // namespace CommonAPI
