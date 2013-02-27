/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusFunctionalHash.h"
#include "DBusServiceRegistry.h"
#include "DBusDaemonProxy.h"
#include "DBusProxyAsyncCallbackHandler.h"

namespace CommonAPI {
namespace DBus {


DBusServiceRegistry::DBusServiceRegistry() :
                dbusServicesStatus_(AvailabilityStatus::UNKNOWN),
                serviceStatusEvent_(std::shared_ptr<DBusServiceRegistry>(this))
{
}

DBusServiceRegistry::DBusServiceRegistry(std::shared_ptr<DBusProxyConnection> dbusConnection) :
                dbusConnection_(dbusConnection),
                dbusServicesStatus_(AvailabilityStatus::UNKNOWN),
                serviceStatusEvent_(std::shared_ptr<DBusServiceRegistry>(this))
{
    dbusDaemonProxyStatusEventSubscription_ =
                    dbusConnection->getDBusDaemonProxy()->getProxyStatusEvent().subscribeCancellableListener(
                                    std::bind(&DBusServiceRegistry::onDBusDaemonProxyStatusEvent, this, std::placeholders::_1));

    dbusDaemonProxyNameOwnerChangedEventSubscription_ =
                    dbusConnection->getDBusDaemonProxy()->getNameOwnerChangedEvent().subscribeCancellableListener(
                    std::bind(&DBusServiceRegistry::onDBusDaemonProxyNameOwnerChangedEvent,
                              this,
                              std::placeholders::_1,
                              std::placeholders::_2,
                              std::placeholders::_3));
}

DBusServiceRegistry::~DBusServiceRegistry() {
    dbusConnection_->getDBusDaemonProxy()->getNameOwnerChangedEvent().unsubscribe(dbusDaemonProxyNameOwnerChangedEventSubscription_);
    dbusConnection_->getDBusDaemonProxy()->getProxyStatusEvent().unsubscribe(dbusDaemonProxyStatusEventSubscription_);
}

bool DBusServiceRegistry::waitDBusServicesAvailable(std::unique_lock<std::mutex>& lock, std::chrono::milliseconds& timeout) {
    bool dbusServicesStatusIsKnown = (dbusServicesStatus_ != AvailabilityStatus::UNKNOWN);

    while (!dbusServicesStatusIsKnown && timeout.count() > 0) {
        typedef std::chrono::high_resolution_clock clock;
        clock::time_point startTimePoint = clock::now();

        dbusServicesStatusIsKnown = dbusServiceChanged_.wait_for(
                                        lock,
                                        timeout,
                                        [&]{ return dbusServicesStatus_ != AvailabilityStatus::UNKNOWN; });

        std::chrono::milliseconds elapsedWaitTime =
                                    std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - startTimePoint);

        if (elapsedWaitTime > timeout) {
            timeout = std::chrono::milliseconds::zero();
            break;
        }

        timeout -= elapsedWaitTime;
    }

    return (dbusServicesStatus_ == AvailabilityStatus::AVAILABLE);
}

bool DBusServiceRegistry::isServiceInstanceAlive(const std::string& dbusInterfaceName, const std::string& dbusServiceName, const std::string& dbusObjectPath) {
    if (!dbusConnection_->isConnected()) {
        return false;
    }

    std::chrono::milliseconds timeout(2000);
    std::unique_lock<std::mutex> dbusServicesLock(dbusServicesMutex_);

    if (!waitDBusServicesAvailable(dbusServicesLock, timeout)) {
        return false;
    }

    auto dbusServiceIterator = dbusServices_.find(dbusServiceName);
    if (dbusServiceIterator == dbusServices_.end()) {
        return false;
    }

    DBusServiceState& dbusServiceState = dbusServiceIterator->second.first;

    if (dbusServiceState == DBusServiceState::AVAILABLE) {
        resolveDBusServiceInstances(dbusServiceIterator);
    }

    if (dbusServiceState == DBusServiceState::RESOLVING) {
        dbusServiceChanged_.wait_for(
                        dbusServicesLock,
                        timeout,
                        [&] { return dbusServiceState != DBusServiceState::RESOLVING; });
    }

    if (dbusServiceState == DBusServiceState::RESOLVED) {
        const DBusInstanceList& dbusInstanceList = dbusServiceIterator->second.second;
        auto dbusInstanceIterator = dbusInstanceList.find({ dbusObjectPath, dbusInterfaceName });

        if (dbusInstanceIterator != dbusInstanceList.end()) {
            const AvailabilityStatus& dbusInstanceAvailabilityStatus = dbusInstanceIterator->second.first;

            return (dbusInstanceAvailabilityStatus == AvailabilityStatus::AVAILABLE);
        }
    }

    return false;
}

// Go through the list of available services and check their interface lists
// If a list is still unknown, then send request to the remote object manager and count it as invalid
// If a list is in acquiring state, then just count it as invalid and skip over it
// Add all matching valid services to the available service list
// If the invalid service count is set, then wait upto waitTimeLimit (2 seconds) for the object manager requests to complete
// If the timeout expires, then go through the list for last time and add everything matching
// If the timeout didn't expire, then go through the list again and send requests for new UNKNOWN services, then wait again for them to complete
// Known limitations:
//   - if the method is called before the first "listNames()" call completes, this request will be blocked
//   - if libdbus is broken and doesn't report errors to timed out requests, then this request will always block for the default 2 seconds (waitTimeLimit)
//   - the method has to be called many times, if you actually want to wait for all services, otherwise you'll always get a partial response. I.e. the more you call this method, the hotter the internal cache gets.
std::vector<std::string> DBusServiceRegistry::getAvailableServiceInstances(const std::string& serviceName,
                                                                           const std::string& domainName) {
    std::vector<std::string> availableServiceInstances;

    if (!dbusConnection_->isConnected()) {
        return availableServiceInstances;
    }

    std::chrono::milliseconds timeout(2000);
    std::unique_lock<std::mutex> dbusServicesLock(dbusServicesMutex_);

    if (!waitDBusServicesAvailable(dbusServicesLock, timeout)) {
        return availableServiceInstances;
    }

    while (timeout.count() > 0) {
        size_t dbusServiceResolvingCount = getAvailableServiceInstances(serviceName, availableServiceInstances);

        if (!dbusServiceResolvingCount) {
            break;
        }

        // wait for unknown and acquiring services, then restart from the beginning
        typedef std::chrono::high_resolution_clock clock;
        clock::time_point startTimePoint = clock::now();

        size_t wakeupCount = 0;
        dbusServiceChanged_.wait_for(
                        dbusServicesLock,
                        timeout,
                        [&] {
                            wakeupCount++;
                            return wakeupCount > dbusServiceResolvingCount;
                        });

        if (wakeupCount > 1) {
            getAvailableServiceInstances(serviceName, availableServiceInstances);
            break;
        }

        std::chrono::milliseconds elapsedWaitTime =
                        std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - startTimePoint);
        timeout -= elapsedWaitTime;
    }

    // maybe partial list but it contains everything we know for now
    return availableServiceInstances;
}

size_t DBusServiceRegistry::getAvailableServiceInstances(const std::string& dbusInterfaceName, std::vector<std::string>& availableServiceInstances) {
    size_t dbusServicesResolvedCount = 0;

    availableServiceInstances.clear();

    // caller must hold lock
    auto dbusServiceIterator = dbusServices_.begin();
    while (dbusServiceIterator != dbusServices_.end()) {
        const std::string& dbusServiceName = dbusServiceIterator->first;
        DBusServiceState& dbusServiceState = dbusServiceIterator->second.first;
        const DBusInstanceList& dbusInstanceList = dbusServiceIterator->second.second;

        // count the resolving services and start aclquiring the objects for unknown ones
        switch (dbusServiceState) {
            case DBusServiceState::AVAILABLE:
                resolveDBusServiceInstances(dbusServiceIterator);
                dbusServicesResolvedCount++;
                break;

            case DBusServiceState::RESOLVING:
                dbusServicesResolvedCount++;
                break;

            case DBusServiceState::RESOLVED:
                for (auto& dbusInstanceIterator : dbusInstanceList) {
                    const AvailabilityStatus& dbusInstanceAvailabilityStatus = dbusInstanceIterator.second.first;
                    const std::string& dbusInstanceObjectPath = dbusInstanceIterator.first.first;
                    const std::string& dbusInstanceInterfaceName = dbusInstanceIterator.first.second;

                    if (dbusInstanceAvailabilityStatus == AvailabilityStatus::AVAILABLE
                                    && dbusInstanceInterfaceName == dbusInterfaceName) {
                        std::string commonApiAddress;

                        DBusAddressTranslator::getInstance().searchForCommonAddress(
                                        dbusInterfaceName,
                                        dbusServiceName,
                                        dbusInstanceObjectPath,
                                        commonApiAddress);

                        availableServiceInstances.emplace_back(std::move(commonApiAddress));
                    }
                }
                break;
        }

        dbusServiceIterator++;
    }

    return dbusServicesResolvedCount;
}

DBusServiceStatusEvent& DBusServiceRegistry::getServiceStatusEvent() {
    return serviceStatusEvent_;
}

DBusServiceRegistry::Subscription DBusServiceRegistry::subscribeAvailabilityListener(const std::string& commonApiAddress,
                                                                                     DBusServiceListener serviceListener) {
    std::string dbusInterfaceName;
    std::string dbusServiceName;
    std::string dbusObjectPath;

    DBusAddressTranslator::getInstance().searchForDBusAddress(commonApiAddress, dbusInterfaceName, dbusServiceName, dbusObjectPath);

    std::lock_guard<std::mutex> dbusServicesLock(dbusServicesMutex_);

    auto dbusServiceIterator = dbusServices_.find(dbusServiceName);

    // add service for the first time
    if (dbusServiceIterator == dbusServices_.end()) {
        DBusServiceState dbusServiceState = DBusServiceState::UNKNOWN;

        if (dbusServicesStatus_ == AvailabilityStatus::AVAILABLE) {
            dbusServiceState = DBusServiceState::NOT_AVAILABLE;
        }

        auto insertIterator = dbusServices_.insert({ dbusServiceName, { dbusServiceState, DBusInstanceList() } });
        assert(insertIterator.second);
        dbusServiceIterator = insertIterator.first;
    }

    DBusServiceState& dbusServiceState = dbusServiceIterator->second.first;
    DBusInstanceList& dbusInstanceList = dbusServiceIterator->second.second;

    auto dbusInstanceIterator = addDBusServiceInstance(
                    dbusInstanceList,
                    dbusObjectPath,
                    dbusInterfaceName,
                    AvailabilityStatus::UNKNOWN);
    AvailabilityStatus& dbusInstanceAvailabilityStatus = dbusInstanceIterator->second.first;
    DBusServiceListenerList& dbusServiceListenerList = dbusInstanceIterator->second.second;

    if (dbusServiceState == DBusServiceState::RESOLVED) {
        dbusInstanceAvailabilityStatus = AvailabilityStatus::NOT_AVAILABLE;
    }

    Subscription listenerSubscription = dbusServiceListenerList.insert(
                    dbusServiceListenerList.end(), serviceListener);

    switch (dbusServiceState) {
        case DBusServiceState::AVAILABLE:
            resolveDBusServiceInstances(dbusServiceIterator);
            break;

        case DBusServiceState::RESOLVED:
        case DBusServiceState::NOT_AVAILABLE:
            serviceListener(dbusInstanceAvailabilityStatus);
            break;
    }

    return listenerSubscription;
}

void DBusServiceRegistry::unsubscribeAvailabilityListener(const std::string& commonApiAddress,
                                                          Subscription& listenerSubscription) {
    std::string dbusInterfaceName;
    std::string dbusServiceName;
    std::string dbusObjectPath;

    DBusAddressTranslator::getInstance().searchForDBusAddress(commonApiAddress, dbusInterfaceName, dbusServiceName, dbusObjectPath);

    std::lock_guard<std::mutex> dbusServicesLock(dbusServicesMutex_);
    auto dbusServiceIterator = dbusServices_.find(dbusServiceName);

    if (dbusServiceIterator == dbusServices_.end()) {
        return;
    }

    DBusServiceState& dbusServiceState = dbusServiceIterator->second.first;
    DBusInstanceList& dbusInstanceList = dbusServiceIterator->second.second;

    auto dbusInstanceIterator = dbusInstanceList.find({ dbusObjectPath, dbusInterfaceName });
    if (dbusInstanceIterator == dbusInstanceList.end()) {
        return;
    }

    const AvailabilityStatus& dbusServiceAvailabilityStatus = dbusInstanceIterator->second.first;
    DBusServiceListenerList& dbusServiceListenerList = dbusInstanceIterator->second.second;

    dbusServiceListenerList.erase(listenerSubscription);

    if (dbusServiceListenerList.empty() && dbusServiceAvailabilityStatus != AvailabilityStatus::AVAILABLE) {
        dbusInstanceList.erase(dbusInstanceIterator);

        if (dbusInstanceList.empty() && dbusServiceState == DBusServiceState::UNKNOWN) {
            dbusServices_.erase(dbusServiceIterator);
        }
    }
}

SubscriptionStatus DBusServiceRegistry::onDBusDaemonProxyStatusEvent(const AvailabilityStatus& availabilityStatus) {
    std::unique_lock<std::mutex> dbusServicesLock(dbusServicesMutex_);

    switch (availabilityStatus) {
        case AvailabilityStatus::AVAILABLE:
            dbusServicesStatus_ = AvailabilityStatus::UNKNOWN;
            dbusConnection_->getDBusDaemonProxy()->listNamesAsync(std::bind(
                            &DBusServiceRegistry::onListNamesCallback,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2));
            break;

        case AvailabilityStatus::NOT_AVAILABLE:
            auto dbusServiceIterator = dbusServices_.begin();

            while (dbusServiceIterator != dbusServices_.end()) {
                dbusServiceIterator = onDBusServiceOffline(dbusServiceIterator, DBusServiceState::NOT_AVAILABLE);
            }

            dbusServicesStatus_ = AvailabilityStatus::NOT_AVAILABLE;
            break;
    }

    return SubscriptionStatus::RETAIN;
}

SubscriptionStatus DBusServiceRegistry::onDBusDaemonProxyNameOwnerChangedEvent(const std::string& affectedName,
                                                                               const std::string& oldOwner,
                                                                               const std::string& newOwner) {
    if (isDBusServiceName(affectedName)) {
        AvailabilityStatus dbusServiceAvailabilityStatus = AvailabilityStatus::AVAILABLE;

        if (newOwner.empty()) {
            dbusServiceAvailabilityStatus = AvailabilityStatus::NOT_AVAILABLE;
        }

        std::unique_lock<std::mutex> dbusServicesLock(dbusServicesMutex_);

        onDBusServiceAvailabilityStatus(affectedName, dbusServiceAvailabilityStatus);
    }

    return SubscriptionStatus::RETAIN;
}

void DBusServiceRegistry::onListNamesCallback(const CommonAPI::CallStatus& callStatus, std::vector<std::string> dbusNames) {
    std::unique_lock<std::mutex> dbusServicesLock(dbusServicesMutex_);

    if (callStatus == CallStatus::SUCCESS) {
        for (const std::string& dbusName : dbusNames) {
            if (isDBusServiceName(dbusName)) {
                onDBusServiceAvailabilityStatus(dbusName, AvailabilityStatus::AVAILABLE);
            }
        }
    }

    dbusServicesStatus_ = AvailabilityStatus::AVAILABLE;

    auto dbusServiceIterator = dbusServices_.begin();
    while (dbusServiceIterator != dbusServices_.end()) {
        const DBusServiceState& dbusServiceState = dbusServiceIterator->second.first;

        if (dbusServiceState == DBusServiceState::UNKNOWN) {
            dbusServiceIterator = onDBusServiceOffline(dbusServiceIterator, DBusServiceState::NOT_AVAILABLE);
        } else {
            dbusServiceIterator++;
        }
    }
}

void DBusServiceRegistry::onDBusServiceAvailabilityStatus(const std::string& dbusServiceName, const AvailabilityStatus& availabilityStatus) {
    auto dbusServiceIterator = dbusServices_.find(dbusServiceName);

    if (dbusServiceIterator != dbusServices_.end()) {
        onDBusServiceAvailabilityStatus(dbusServiceIterator, availabilityStatus);

    } else if (availabilityStatus == AvailabilityStatus::AVAILABLE) {
        dbusServices_.insert({ dbusServiceName, { DBusServiceState::AVAILABLE, DBusInstanceList() } });
        dbusServiceChanged_.notify_all();
    }
}

DBusServiceRegistry::DBusServiceList::iterator DBusServiceRegistry::onDBusServiceAvailabilityStatus(DBusServiceList::iterator& dbusServiceIterator,
                                                                                                    const AvailabilityStatus& availabilityStatus) {
    DBusServiceState& dbusServiceState = dbusServiceIterator->second.first;
    DBusInstanceList& dbusInstanceList = dbusServiceIterator->second.second;

    if (availabilityStatus == AvailabilityStatus::AVAILABLE) {
        const std::string& dbusServiceName = dbusServiceIterator->first;
        const bool dbusServiceHasListeners = !dbusInstanceList.empty();

        if (dbusServiceState != DBusServiceState::RESOLVING) {
            resolveDBusServiceInstances(dbusServiceIterator);
        }

        dbusServiceChanged_.notify_all();

        return dbusServiceIterator;
    }

    dbusServiceState = (availabilityStatus == AvailabilityStatus::UNKNOWN) ?
                    DBusServiceState::UNKNOWN :
                    DBusServiceState::NOT_AVAILABLE;

    return onDBusServiceOffline(dbusServiceIterator, dbusServiceState);
}

DBusServiceRegistry::DBusServiceList::iterator DBusServiceRegistry::onDBusServiceOffline(DBusServiceList::iterator& dbusServiceIterator,
                                                                                         const DBusServiceState& newDBusServiceState) {
    DBusServiceState& dbusServiceState = dbusServiceIterator->second.first;
    DBusInstanceList& dbusInstanceList = dbusServiceIterator->second.second;
    auto dbusInstanceIterator = dbusInstanceList.begin();

    assert(newDBusServiceState == DBusServiceState::UNKNOWN || newDBusServiceState == DBusServiceState::NOT_AVAILABLE);

    dbusServiceState = newDBusServiceState;

    while (dbusInstanceIterator != dbusInstanceList.end()) {
        AvailabilityStatus& dbusInstanceAvailabilityStatus = dbusInstanceIterator->second.first;
        DBusServiceListenerList& dbusServiceListenerList = dbusInstanceIterator->second.second;

        // notify listeners
        if (!dbusServiceListenerList.empty()) {
            // the internal state is unknown until the next time we ask the object manager
            notifyDBusServiceListeners(dbusServiceListenerList, AvailabilityStatus::NOT_AVAILABLE);
            dbusInstanceAvailabilityStatus = AvailabilityStatus::UNKNOWN;
            dbusInstanceIterator++;
        } else {
            dbusInstanceIterator = dbusInstanceList.erase(dbusInstanceIterator);
        }
    }

    dbusServiceChanged_.notify_all();

    if (dbusInstanceList.empty()) {
        return dbusServices_.erase(dbusServiceIterator);
    }

    dbusServiceIterator++;

    return dbusServiceIterator;
}

void DBusServiceRegistry::resolveDBusServiceInstances(DBusServiceList::iterator& dbusServiceIterator) {
    const std::string& dbusServiceName = dbusServiceIterator->first;
    DBusServiceState& dbusServiceState = dbusServiceIterator->second.first;
    DBusInstanceList& dbusInstanceList = dbusServiceIterator->second.second;
    std::vector<DBusServiceAddress> predefinedDBusServiceInstances;

    dbusServiceState = DBusServiceState::RESOLVING;

    // add predefined instances
    DBusAddressTranslator::getInstance().getPredefinedInstances(
                    dbusServiceName,
                    predefinedDBusServiceInstances);
    for (auto& dbusServiceAddress : predefinedDBusServiceInstances) {
        const std::string& dbusObjectPath = std::get<1>(dbusServiceAddress);
        const std::string& dbusInterfaceName = std::get<2>(dbusServiceAddress);

        auto dbusInstanceIterator = addDBusServiceInstance(
                        dbusInstanceList,
                        dbusObjectPath,
                        dbusInterfaceName,
                        AvailabilityStatus::AVAILABLE);
        DBusServiceListenerList& dbusServiceListenerList = dbusInstanceIterator->second.second;

        notifyDBusServiceListeners(dbusServiceListenerList, AvailabilityStatus::AVAILABLE);
    }

    // resolve remote objects
    auto dbusMethodCallMessage = DBusMessage::createMethodCall(
                    dbusServiceName.c_str(),
                    "/",
                    "org.freedesktop.DBus.ObjectManager",
                    "GetManagedObjects",
                    "");

    const int timeoutMilliseconds = 100;

    dbusConnection_->sendDBusMessageWithReplyAsync(
                    dbusMethodCallMessage,
                    DBusProxyAsyncCallbackHandler<DBusObjectToInterfaceDict>::create(
                                    std::bind(&DBusServiceRegistry::onGetManagedObjectsCallback,
                                              this,
                                              std::placeholders::_1,
                                              std::placeholders::_2,
                                              dbusServiceName)),
                                    timeoutMilliseconds);
}

void DBusServiceRegistry::onGetManagedObjectsCallback(const CallStatus& callStatus,
                                                      DBusObjectToInterfaceDict managedObjects,
                                                      const std::string& dbusServiceName) {
    std::unique_lock<std::mutex> dbusServicesLock(dbusServicesMutex_);

    auto dbusServiceIterator = dbusServices_.find(dbusServiceName);
    if (dbusServiceIterator == dbusServices_.end()) {
        return; // nothing we can do
    }

    DBusServiceState& dbusServiceState = dbusServiceIterator->second.first;
    DBusInstanceList& dbusInstanceList = dbusServiceIterator->second.second;

    dbusServiceState = (dbusServicesStatus_ == AvailabilityStatus::AVAILABLE) ? DBusServiceState::NOT_AVAILABLE : DBusServiceState::UNKNOWN;

    if (callStatus == CallStatus::SUCCESS) {
        dbusServiceState = DBusServiceState::RESOLVED;

        for (auto& dbusObjectPathIterator : managedObjects) {
            const std::string& dbusObjectPath = dbusObjectPathIterator.first;

            for (auto& dbusInterfaceNameIterator : dbusObjectPathIterator.second) {
                const std::string& dbusInterfaceName = dbusInterfaceNameIterator.first;

                auto dbusInstanceIterator = addDBusServiceInstance(
                                dbusInstanceList,
                                dbusObjectPath,
                                dbusInterfaceName,
                                AvailabilityStatus::AVAILABLE);
                DBusServiceListenerList& dbusServiceListenerList = dbusInstanceIterator->second.second;

                notifyDBusServiceListeners(dbusServiceListenerList, AvailabilityStatus::AVAILABLE);
            }
        }
    }

    dbusServiceChanged_.notify_all();

    // notify only UNKNOWN. The predefined and resolved have already been handled
    for (auto& dbusInstanceIterator : dbusInstanceList) {
        AvailabilityStatus& dbusInstanceAvailabilityStatus = dbusInstanceIterator.second.first;
        DBusServiceListenerList& dbusServiceListenerList = dbusInstanceIterator.second.second;

        if (dbusInstanceAvailabilityStatus == AvailabilityStatus::UNKNOWN) {
            dbusInstanceAvailabilityStatus = AvailabilityStatus::NOT_AVAILABLE;
            notifyDBusServiceListeners(dbusServiceListenerList, dbusInstanceAvailabilityStatus);
        }
    }
}

DBusServiceRegistry::DBusInstanceList::iterator DBusServiceRegistry::addDBusServiceInstance(DBusInstanceList& dbusInstanceList,
                                                                                            const std::string& dbusObjectPath,
                                                                                            const std::string& dbusInterfaceName,
                                                                                            const AvailabilityStatus& newDBusInstanceAvailabilityStatus) {
    auto dbusInstanceIterator = dbusInstanceList.find({ dbusObjectPath, dbusInterfaceName });

    // add instance for the first time
    if (dbusInstanceIterator == dbusInstanceList.end()) {
       auto insertIterator = dbusInstanceList.insert(
                        { { dbusObjectPath, dbusInterfaceName }, { newDBusInstanceAvailabilityStatus, DBusServiceListenerList() } });

       assert(insertIterator.second);
       dbusInstanceIterator = insertIterator.first;
    } else {
        AvailabilityStatus& dbusInstanceAvailabilityStatus = dbusInstanceIterator->second.first;
        dbusInstanceAvailabilityStatus = newDBusInstanceAvailabilityStatus;
    }

    return dbusInstanceIterator;
}

void DBusServiceRegistry::notifyDBusServiceListeners(DBusServiceListenerList& dbusServiceListenerList,
                                                     const AvailabilityStatus& availabilityStatus) {
    for (auto& dbusServiceListener : dbusServiceListenerList) {
        dbusServiceListener(availabilityStatus);
    }
}

bool DBusServiceRegistry::isDBusServiceName(const std::string& name) {
    return name[0] != ':';
}

}// namespace DBus
}// namespace CommonAPI
