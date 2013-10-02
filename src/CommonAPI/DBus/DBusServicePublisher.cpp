/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DBusServicePublisher.h"
#include "DBusFactory.h"

#include <cassert>


namespace CommonAPI {
namespace DBus {


std::shared_ptr<DBusServicePublisher> DBusServicePublisher::getInstance() {
    static std::shared_ptr<DBusServicePublisher> instance;
    if(!instance) {
        instance = std::make_shared<DBusServicePublisher>();
    }
    return instance;
}

bool DBusServicePublisher::registerService(const std::shared_ptr<DBusStubAdapter>& dbusStubAdapter) {
    auto serviceAddress = dbusStubAdapter->getAddress();
    const auto& dbusConnection = dbusStubAdapter->getDBusConnection();
    std::shared_ptr<DBusObjectManagerStub> rootDBusObjectManagerStub = dbusConnection->getDBusObjectManager()->getRootDBusObjectManagerStub();
    const bool isRegistrationAtManagerSuccessfull = registerManagedService(dbusStubAdapter);

    if (!isRegistrationAtManagerSuccessfull) {
        return false;
    }

    const bool isServiceExportSuccessful = rootDBusObjectManagerStub->exportManagedDBusStubAdapter(dbusStubAdapter);
    if (!isServiceExportSuccessful) {
        const bool isManagedDeregistrationSuccessfull = unregisterManagedService(serviceAddress);
        assert(isManagedDeregistrationSuccessfull);
    }

    return isServiceExportSuccessful;
}

std::shared_ptr<DBusStubAdapter> DBusServicePublisher::getRegisteredService(const std::string& serviceAddress)  {
    auto registeredServiceIterator = registeredServices_.find(serviceAddress);
    if (registeredServiceIterator != registeredServices_.end()) {
        return registeredServiceIterator->second;
    }
    return nullptr;
}

bool DBusServicePublisher::registerService(const std::shared_ptr<StubBase>& stubBase,
                                           const char* interfaceId,
                                           const std::string& participantId,
                                           const std::string& serviceName,
                                           const std::string& domain,
                                           const std::shared_ptr<Factory>& factory) {
    auto dbusFactory = std::dynamic_pointer_cast<DBusFactory>(factory);
    if (!dbusFactory) {
        return false;
    }

    auto dbusStubAdapter = dbusFactory->createDBusStubAdapter(stubBase, interfaceId, participantId, serviceName, domain);
    if (!dbusStubAdapter) {
        return false;
    }

    const bool isRegistrationSuccessful = registerService(dbusStubAdapter);
    return isRegistrationSuccessful;
}


bool DBusServicePublisher::unregisterService(const std::string& serviceAddress) {
    auto registeredServiceIterator = registeredServices_.find(serviceAddress);
    const bool isServiceAddressRegistered = (registeredServiceIterator != registeredServices_.end());

    if (!isServiceAddressRegistered) {
        return false;
    }

    const auto& registeredDBusStubAdapter = registeredServiceIterator->second;
    const auto& dbusConnection = registeredDBusStubAdapter->getDBusConnection();
    std::shared_ptr<DBusObjectManagerStub> rootDBusObjectManagerStub = dbusConnection->getDBusObjectManager()->getRootDBusObjectManagerStub();
    const bool isRootService = rootDBusObjectManagerStub->isDBusStubAdapterExported(registeredDBusStubAdapter);
    registeredDBusStubAdapter->deactivateManagedInstances();
    if (isRootService) {
        const bool isUnexportSuccessfull = rootDBusObjectManagerStub->unexportManagedDBusStubAdapter(registeredDBusStubAdapter);
        assert(isUnexportSuccessfull);

        unregisterManagedService(registeredServiceIterator);
    }

    return isRootService;
}


bool DBusServicePublisher::registerManagedService(const std::shared_ptr<DBusStubAdapter>& managedDBusStubAdapter) {
    auto serviceAddress = managedDBusStubAdapter->getAddress();
    const auto& insertResult = registeredServices_.insert( {serviceAddress, managedDBusStubAdapter} );
    const auto& insertIter = insertResult.first;
    const bool& isInsertSuccessful = insertResult.second;

    if (!isInsertSuccessful) {
        return false;
    }

    const auto& dbusConnection = managedDBusStubAdapter->getDBusConnection();
    const auto dbusObjectManager = dbusConnection->getDBusObjectManager();
    const bool isDBusObjectRegistrationSuccessful = dbusObjectManager->registerDBusStubAdapter(managedDBusStubAdapter);
    if (!isDBusObjectRegistrationSuccessful) {
        registeredServices_.erase(insertIter);
        return false;
    }

    const auto& dbusServiceName = managedDBusStubAdapter->getDBusName();
    const bool isServiceNameAcquired = dbusConnection->requestServiceNameAndBlock(dbusServiceName);
    if (!isServiceNameAcquired) {
        const bool isDBusObjectDeregistrationSuccessful = dbusObjectManager->unregisterDBusStubAdapter(managedDBusStubAdapter);
        assert(isDBusObjectDeregistrationSuccessful);

        registeredServices_.erase(insertIter);
    }

    return isServiceNameAcquired;
}


bool DBusServicePublisher::unregisterManagedService(const std::string& serviceAddress) {
    auto registeredServiceIterator = registeredServices_.find(serviceAddress);
    const bool isServiceAddressRegistered = (registeredServiceIterator != registeredServices_.end());

    if (isServiceAddressRegistered) {
        unregisterManagedService(registeredServiceIterator);
    }

    return isServiceAddressRegistered;
}

void DBusServicePublisher::unregisterManagedService(DBusServicesMap::iterator& managedServiceIterator) {
    const auto& registeredDbusStubAdapter = managedServiceIterator->second;
    const auto& dbusConnection = registeredDbusStubAdapter->getDBusConnection();
    const auto dbusObjectManager = dbusConnection->getDBusObjectManager();
    const auto& dbusServiceName = registeredDbusStubAdapter->getDBusName();

    const bool isDBusStubAdapterUnregistered = dbusObjectManager->unregisterDBusStubAdapter(registeredDbusStubAdapter);
    assert(isDBusStubAdapterUnregistered);

    dbusConnection->releaseServiceName(dbusServiceName);

    registeredServices_.erase(managedServiceIterator);
}

} // namespace DBus
} // namespace CommonAPI
