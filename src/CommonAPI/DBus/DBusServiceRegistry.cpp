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

DBusServiceRegistry::DBusServiceRegistry(std::shared_ptr<DBusProxyConnection> dbusProxyConnection):
                dbusDaemonProxy_(std::make_shared<CommonAPI::DBus::DBusDaemonProxy>(dbusProxyConnection)),
                dbusNameListStatus_(AvailabilityStatus::UNKNOWN),
                initialized_(false) {
}

DBusServiceRegistry::~DBusServiceRegistry() {
	if(initialized_) {
		dbusDaemonProxy_->getNameOwnerChangedEvent().unsubscribe(dbusDaemonProxyNameOwnerChangedEventSubscription_);
		dbusDaemonProxy_->getProxyStatusEvent().unsubscribe(dbusDaemonProxyStatusEventSubscription_);
	}
}

void DBusServiceRegistry::init() {
	dbusDaemonProxyStatusEventSubscription_ =
					dbusDaemonProxy_->getProxyStatusEvent().subscribeCancellableListener(
									std::bind(&DBusServiceRegistry::onDBusDaemonProxyStatusEvent, this, std::placeholders::_1));

	dbusDaemonProxyNameOwnerChangedEventSubscription_ =
					dbusDaemonProxy_->getNameOwnerChangedEvent().subscribeCancellableListener(
					std::bind(&DBusServiceRegistry::onDBusDaemonProxyNameOwnerChangedEvent,
							  this,
							  std::placeholders::_1,
							  std::placeholders::_2,
							  std::placeholders::_3));
	initialized_ = true;
}

bool DBusServiceRegistry::waitDBusServicesAvailable(std::unique_lock<std::mutex>& lock, std::chrono::milliseconds& timeout) {
    bool dbusServicesStatusIsKnown = (dbusNameListStatus_ == AvailabilityStatus::AVAILABLE);

    if(!dbusServicesStatusIsKnown) {
        typedef std::chrono::high_resolution_clock clock;
        clock::time_point startTimePoint = clock::now();

        while (!dbusServicesStatusIsKnown && timeout.count() > 0) {
            dbusServicesStatusIsKnown = dbusServiceChanged_.wait_for(
                                            lock,
                                            timeout / 10,
                                            [&]{ return dbusNameListStatus_ == AvailabilityStatus::AVAILABLE; });

            std::chrono::milliseconds elapsedWaitTime =
                                        std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - startTimePoint);

            if (elapsedWaitTime > timeout) {
                break;
            }
        }
    }

    return (dbusNameListStatus_ == AvailabilityStatus::AVAILABLE);
}

bool DBusServiceRegistry::isServiceInstanceAlive(const std::string& dbusInterfaceName, const std::string& dbusServiceName, const std::string& dbusObjectPath) {
    if (!dbusDaemonProxy_->isAvailable()) {
        return false;
    }

    std::chrono::milliseconds timeout(1000);
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

    const DBusInstanceList& dbusInstanceList = dbusServiceIterator->second.second;
    auto dbusInstanceIterator = dbusInstanceList.find({ dbusObjectPath, dbusInterfaceName });

    if (dbusInstanceIterator != dbusInstanceList.end()) {
        const AvailabilityStatus& dbusInstanceAvailabilityStatus = dbusInstanceIterator->second.first;

        return (dbusInstanceAvailabilityStatus == AvailabilityStatus::AVAILABLE);
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

    if (!dbusDaemonProxy_->isAvailable()) {
        return availableServiceInstances;
    }

    std::chrono::milliseconds timeout(1000);
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
    size_t dbusServicesResolvingCount = 0;

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
                dbusServicesResolvingCount++;
                break;

            case DBusServiceState::RESOLVING:
            case DBusServiceState::RESOLVED:
                if (dbusServiceState == DBusServiceState::RESOLVING) {
                    dbusServicesResolvingCount++;
                }

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

    return dbusServicesResolvingCount;
}


DBusServiceRegistry::Subscription DBusServiceRegistry::subscribeAvailabilityListener(const std::string& commonApiAddress,
                                                                                     DBusServiceListener serviceListener) {
    std::string dbusInterfaceName;
    std::string dbusServiceName;
    std::string dbusObjectPath;

    DBusAddressTranslator::getInstance().searchForDBusAddress(commonApiAddress, dbusInterfaceName, dbusServiceName, dbusObjectPath);

    std::lock_guard<std::mutex> dbusServicesLock(dbusServicesMutex_);

    DBusServiceList::iterator dbusServiceIterator = dbusServices_.find(dbusServiceName);

    // Service not known, so just add it to the list of unkown or definitely not available services
    if (dbusServiceIterator == dbusServices_.end()) {
        DBusServiceState dbusConnectionNameState = DBusServiceState::UNKNOWN;

        // Service is definitely not available if the complete list of available services is known and it is not in there
        if (dbusNameListStatus_ == AvailabilityStatus::AVAILABLE) {
            dbusConnectionNameState = DBusServiceState::RESOLVED;
        }

        std::pair<DBusServiceList::iterator, bool> insertResult = dbusServices_.insert({ dbusServiceName, { dbusConnectionNameState, DBusInstanceList() } });
        assert(insertResult.second);
        dbusServiceIterator = insertResult.first;
    }

    DBusServiceState& dbusConnectionNameState = dbusServiceIterator->second.first;
    DBusInstanceList& dbusInstanceList = dbusServiceIterator->second.second;

    auto dbusInstanceIterator = addDBusServiceInstance(
                    dbusInstanceList,
                    dbusObjectPath,
                    dbusInterfaceName);
    AvailabilityStatus& dbusInstanceAvailabilityStatus = dbusInstanceIterator->second.first;
    DBusServiceListenerList& dbusServiceListenerList = dbusInstanceIterator->second.second;

    if (dbusConnectionNameState == DBusServiceState::RESOLVED
                    && dbusInstanceAvailabilityStatus == AvailabilityStatus::UNKNOWN) {
        dbusInstanceAvailabilityStatus = AvailabilityStatus::NOT_AVAILABLE;
    }

    Subscription listenerSubscription = dbusServiceListenerList.insert(
                    dbusServiceListenerList.end(), serviceListener);

    switch (dbusConnectionNameState) {
        case DBusServiceState::AVAILABLE:
            resolveDBusServiceInstances(dbusServiceIterator);
            break;

        case DBusServiceState::RESOLVING:
            if (dbusInstanceAvailabilityStatus == AvailabilityStatus::AVAILABLE) {
                serviceListener(dbusInstanceAvailabilityStatus);
            }
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

    std::lock_guard<std::mutex> dbusServicesLock(dbusServicesMutex_);
    DBusAddressTranslator::getInstance().searchForDBusAddress(commonApiAddress, dbusInterfaceName, dbusServiceName, dbusObjectPath);

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
    std::lock_guard<std::mutex> dbusServicesLock(dbusServicesMutex_);

    switch (availabilityStatus) {
        case AvailabilityStatus::AVAILABLE:
            dbusNameListStatus_ = AvailabilityStatus::UNKNOWN;
            dbusDaemonProxy_->listNamesAsync(std::bind(
                            &DBusServiceRegistry::onListNamesCallback,
                            this->shared_from_this(),
                            std::placeholders::_1,
                            std::placeholders::_2));
            break;

        case AvailabilityStatus::NOT_AVAILABLE:
            auto dbusServiceIterator = dbusServices_.begin();

            while (dbusServiceIterator != dbusServices_.end()) {
                dbusServiceIterator = onDBusServiceOffline(dbusServiceIterator, DBusServiceState::NOT_AVAILABLE);
            }

            dbusNameListStatus_ = AvailabilityStatus::NOT_AVAILABLE;
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

        std::lock_guard<std::mutex> dbusServicesLock(dbusServicesMutex_);

        onDBusServiceAvailabilityStatus(affectedName, dbusServiceAvailabilityStatus);
    }

    return SubscriptionStatus::RETAIN;
}

void DBusServiceRegistry::onListNamesCallback(const CommonAPI::CallStatus& callStatus, std::vector<std::string> dbusNames) {
    std::lock_guard<std::mutex> dbusServicesLock(dbusServicesMutex_);

    if (callStatus == CallStatus::SUCCESS) {
        for (const std::string& dbusName : dbusNames) {
            if (isDBusServiceName(dbusName)) {
                onDBusServiceAvailabilityStatus(dbusName, AvailabilityStatus::AVAILABLE);
            }
        }
    }

    dbusNameListStatus_ = AvailabilityStatus::AVAILABLE;

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

        if (dbusServiceState != DBusServiceState::RESOLVING) {
            resolveDBusServiceInstances(dbusServiceIterator);
        }

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
    DBusAddressTranslator::getInstance().getPredefinedInstances(dbusServiceName, predefinedDBusServiceInstances);

    for (auto& dbusServiceAddress : predefinedDBusServiceInstances) {
        const std::string& dbusObjectPath = std::get<1>(dbusServiceAddress);
        const std::string& dbusInterfaceName = std::get<2>(dbusServiceAddress);

        onDBusServiceInstanceAvailable(dbusInstanceList, dbusObjectPath, dbusInterfaceName);
    }

    dbusServiceChanged_.notify_all();

    // search for remote instances
    DBusDaemonProxy::GetManagedObjectsAsyncCallback callback = std::bind(&DBusServiceRegistry::onGetManagedObjectsCallback,
    																     this->shared_from_this(),
                                                                         std::placeholders::_1,
                                                                         std::placeholders::_2,
                                                                         dbusServiceName);
    dbusDaemonProxy_->getManagedObjectsAsync(dbusServiceName, callback);
}

void DBusServiceRegistry::onGetManagedObjectsCallback(const CallStatus& callStatus,
                                                      DBusDaemonProxy::DBusObjectToInterfaceDict managedObjects,
                                                      const std::string& dbusServiceName) {
    std::lock_guard<std::mutex> dbusServicesLock(dbusServicesMutex_);

    // already offline
    if (dbusNameListStatus_ == AvailabilityStatus::NOT_AVAILABLE) {
        return;
    }

    auto dbusServiceIterator = dbusServices_.find(dbusServiceName);
    if (dbusServiceIterator == dbusServices_.end()) {
        return; // nothing we can do
    }

    DBusServiceState& dbusServiceState = dbusServiceIterator->second.first;
    DBusInstanceList& dbusInstanceList = dbusServiceIterator->second.second;

    dbusServiceState = DBusServiceState::RESOLVED;

    if (callStatus == CallStatus::SUCCESS) {
        for (auto& dbusObjectPathIterator : managedObjects) {
            const std::string& dbusObjectPath = dbusObjectPathIterator.first;

            for (auto& dbusInterfaceNameIterator : dbusObjectPathIterator.second) {
                const std::string& dbusInterfaceName = dbusInterfaceNameIterator.first;

                onDBusServiceInstanceAvailable(dbusInstanceList, dbusObjectPath, dbusInterfaceName);
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

void DBusServiceRegistry::onDBusServiceInstanceAvailable(DBusInstanceList& dbusInstanceList,
                                                         const std::string& dbusObjectPath,
                                                         const std::string& dbusInterfaceName) {
    auto dbusInstanceIterator = addDBusServiceInstance(dbusInstanceList, dbusObjectPath, dbusInterfaceName);
    AvailabilityStatus& dbusInstanceAvailabilityStatus = dbusInstanceIterator->second.first;
    DBusServiceListenerList& dbusServiceListenerList = dbusInstanceIterator->second.second;

    dbusInstanceAvailabilityStatus = AvailabilityStatus::AVAILABLE;

    notifyDBusServiceListeners(dbusServiceListenerList, dbusInstanceAvailabilityStatus);
}

DBusServiceRegistry::DBusInstanceList::iterator DBusServiceRegistry::addDBusServiceInstance(DBusInstanceList& dbusInstanceList,
                                                                                            const std::string& dbusObjectPath,
                                                                                            const std::string& dbusInterfaceName) {
    auto dbusInstanceIterator = dbusInstanceList.find({ dbusObjectPath, dbusInterfaceName });

    // add instance for the first time
    if (dbusInstanceIterator == dbusInstanceList.end()) {
       auto insertIterator = dbusInstanceList.insert(
                        { { dbusObjectPath, dbusInterfaceName }, { AvailabilityStatus::UNKNOWN, DBusServiceListenerList() } });
       const bool& insertSuccessfull = insertIterator.second;

       assert(insertSuccessfull);
       dbusInstanceIterator = insertIterator.first;
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
