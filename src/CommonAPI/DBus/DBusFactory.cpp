/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DBusProxy.h"
#include "DBusConnection.h"
#include "DBusFactory.h"
#include "DBusAddressTranslator.h"
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


DBusFactory::DBusFactory(std::shared_ptr<Runtime> runtime, const MiddlewareInfo* middlewareInfo, std::shared_ptr<MainLoopContext> mainLoopContext) :
                CommonAPI::Factory(runtime, middlewareInfo),
                dbusConnection_(CommonAPI::DBus::DBusConnection::getSessionBus()),
                acquiredConnectionName_(""),
                mainLoopContext_(mainLoopContext) {
    bool startDispatchThread = !mainLoopContext_;
    dbusConnection_->connect(startDispatchThread);
    if(mainLoopContext_) {
        dbusConnection_->attachMainLoopContext(mainLoopContext);
    }
}


DBusFactory::~DBusFactory() {
}


std::vector<std::string> DBusFactory::getAvailableServiceInstances(const std::string& serviceName,
                                                                   const std::string& domainName) {
    return dbusConnection_->getDBusServiceRegistry()->getAvailableServiceInstances(serviceName, domainName);
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
            return (it->second)(commonApiAddress, interfaceName, connectionName, objectPath, dbusConnection_);
        }
    }

    return NULL;
}

bool DBusFactory::registerAdapter(std::shared_ptr<StubBase> stubBase,
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

    if(acquiredConnectionName_ == "") {
        bool isServiceNameAcquired = dbusConnection_->requestServiceNameAndBlock(connectionName);
        if(!isServiceNameAcquired) {
            return false;
        }
        acquiredConnectionName_ = connectionName;
    } else if (acquiredConnectionName_ != connectionName) {
        return false;
    }

    if(!registeredAdapterFactoryFunctions_) {
        registeredAdapterFactoryFunctions_ = new std::unordered_map<std::string, DBusAdapterFactoryFunction> {};
    }

    auto foundFunction = registeredAdapterFactoryFunctions_->find(interfaceId);
    if(foundFunction != registeredAdapterFactoryFunctions_->end()) {
        std::shared_ptr<DBusStubAdapter> dbusStubAdapter =  (foundFunction->second)(commonApiAddress, interfaceName, connectionName, objectPath, dbusConnection_, stubBase);
        if(!dbusStubAdapter) {
            return false;
        }
        if(registeredServices_.insert( {std::move(commonApiAddress), dbusStubAdapter} ).second) {
            dbusStubAdapter->init();
            return true;
        }
    }

	return false;
}

bool DBusFactory::unregisterService(const std::string& participantId, const std::string& serviceName, const std::string& domain) {
    std::string commonApiAddress = domain + ":" + serviceName + ":" + participantId;
	auto foundStubAdapter = registeredServices_.find(commonApiAddress);
	if(foundStubAdapter != registeredServices_.end()) {
		std::shared_ptr<DBusStubAdapter> stubAdapter = foundStubAdapter->second;
		stubAdapter->deinit();
		return registeredServices_.erase(commonApiAddress);
	}
	return false;
}


} // namespace DBus
} // namespace CommonAPI
