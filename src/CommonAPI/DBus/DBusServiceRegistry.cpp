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
#include "DBusUtils.h"

#include <iostream>
#include <iterator>

namespace CommonAPI {
namespace DBus {

DBusServiceRegistry::DBusServiceRegistry(std::shared_ptr<DBusProxyConnection> dbusProxyConnection) :
                dbusDaemonProxy_(std::make_shared<CommonAPI::DBus::DBusDaemonProxy>(dbusProxyConnection)),
                initialized_(false),
                notificationThread_() {
}

DBusServiceRegistry::~DBusServiceRegistry() {
    if (!initialized_) {
        return;
    }

    dbusDaemonProxy_->getNameOwnerChangedEvent().unsubscribe(dbusDaemonProxyNameOwnerChangedEventSubscription_);
    dbusDaemonProxy_->getProxyStatusEvent().unsubscribe(dbusDaemonProxyStatusEventSubscription_);

    // notify only listeners of resolved services (online > offline)
    for (auto& dbusServiceListenersIterator : dbusServiceListenersMap) {
        auto& dbusServiceListenersRecord = dbusServiceListenersIterator.second;

        // fulfill all open promises
        std::promise<DBusRecordState> promiseOnResolve = std::move(dbusServiceListenersRecord.promiseOnResolve);
        promiseOnResolve.set_value(DBusRecordState::NOT_AVAILABLE);

        if (dbusServiceListenersRecord.uniqueBusNameState == DBusRecordState::RESOLVED) {
            onDBusServiceNotAvailable(dbusServiceListenersRecord);
        }
    }

    // remove all object manager signal member handlers
    for (auto& dbusServiceUniqueNameIterator : dbusUniqueNamesMap_) {
        const auto& dbusServiceUniqueName = dbusServiceUniqueNameIterator.first;

        auto dbusProxyConnection = dbusDaemonProxy_->getDBusConnection();
        const bool isSubscriptionCancelled = dbusProxyConnection->removeObjectManagerSignalMemberHandler(
            dbusServiceUniqueName,
            this);
        assert(isSubscriptionCancelled);
    }
}

void DBusServiceRegistry::init() {
    dbusDaemonProxyStatusEventSubscription_ =
                    dbusDaemonProxy_->getProxyStatusEvent().subscribeCancellableListener(
                        std::bind(&DBusServiceRegistry::onDBusDaemonProxyStatusEvent, shared_from_this(), std::placeholders::_1));

    dbusDaemonProxyNameOwnerChangedEventSubscription_ =
                    dbusDaemonProxy_->getNameOwnerChangedEvent().subscribeCancellableListener(
                        std::bind(&DBusServiceRegistry::onDBusDaemonProxyNameOwnerChangedEvent,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2,
                            std::placeholders::_3));

    fetchAllServiceNames(); // initialize list of registered bus names

    initialized_ = true;
}

DBusServiceRegistry::DBusServiceSubscription DBusServiceRegistry::subscribeAvailabilityListener(const std::string& commonApiAddress,
                                                                                                DBusServiceListener serviceListener) {
    std::string dbusInterfaceName;
    std::string dbusServiceName;
    std::string dbusObjectPath;

    DBusAddressTranslator::getInstance().searchForDBusAddress(
        commonApiAddress,
        dbusInterfaceName,
        dbusServiceName,
        dbusObjectPath);

    if (notificationThread_ == std::this_thread::get_id()) {
        std::cerr << "ERROR: You must not build proxies in callbacks of ProxyStatusEvent."
                        << " Refer to the documentation for suggestions how to avoid this.\n";
        assert(false);
    }

    std::lock_guard<std::mutex> dbusServicesLock(dbusServicesMutex_);
    auto& dbusServiceListenersRecord = dbusServiceListenersMap[dbusServiceName];
    assert(dbusServiceListenersRecord.uniqueBusNameState != DBusRecordState::AVAILABLE);

    auto& dbusInterfaceNameListenersMap = dbusServiceListenersRecord.dbusObjectPathListenersMap[dbusObjectPath];
    auto& dbusInterfaceNameListenersRecord = dbusInterfaceNameListenersMap[dbusInterfaceName];

    AvailabilityStatus availabilityStatus = AvailabilityStatus::UNKNOWN;

    if (dbusServiceListenersRecord.uniqueBusNameState == DBusRecordState::UNKNOWN) {
        dbusInterfaceNameListenersRecord.state = DBusRecordState::UNKNOWN;
        if (dbusServiceListenersRecord.uniqueBusNameState == DBusRecordState::UNKNOWN) {
            resolveDBusServiceName(dbusServiceName, dbusServiceListenersRecord);
        }
    } else if (dbusServiceListenersRecord.uniqueBusNameState == DBusRecordState::NOT_AVAILABLE) {
        availabilityStatus = AvailabilityStatus::NOT_AVAILABLE;
    } else if (dbusServiceListenersRecord.uniqueBusNameState != DBusRecordState::RESOLVING && dbusInterfaceNameListenersRecord.state == DBusRecordState::UNKNOWN) {
        dbusInterfaceNameListenersRecord.state = resolveDBusInterfaceNameState(
            dbusInterfaceName,
            dbusObjectPath,
            dbusServiceName,
            dbusServiceListenersRecord);
    }

    if(availabilityStatus == AvailabilityStatus::UNKNOWN) {
        switch (dbusInterfaceNameListenersRecord.state) {
            case DBusRecordState::AVAILABLE:
                availabilityStatus = AvailabilityStatus::AVAILABLE;
                break;
            case DBusRecordState::NOT_AVAILABLE:
                availabilityStatus = AvailabilityStatus::NOT_AVAILABLE;
                break;
            default:
                availabilityStatus = AvailabilityStatus::UNKNOWN;
        }
    }


    if (availabilityStatus != AvailabilityStatus::UNKNOWN) {
        notificationThread_ = std::this_thread::get_id();
        SubscriptionStatus subscriptionStatus = serviceListener(availabilityStatus);
        notificationThread_ = std::thread::id();

        if (subscriptionStatus == SubscriptionStatus::CANCEL) {
            if (dbusInterfaceNameListenersRecord.listenerList.empty()) {
                dbusInterfaceNameListenersMap.erase(dbusInterfaceName);
                if (dbusInterfaceNameListenersMap.empty()) {
                    dbusServiceListenersRecord.dbusObjectPathListenersMap.erase(dbusObjectPath);
                }
            }

            return DBusServiceSubscription();
        }
    }

    dbusInterfaceNameListenersRecord.listenerList.push_front(std::move(serviceListener));

    return dbusInterfaceNameListenersRecord.listenerList.begin();
}

void DBusServiceRegistry::unsubscribeAvailabilityListener(const std::string& commonApiAddress,
                                                          DBusServiceSubscription& listenerSubscription) {
    std::string dbusInterfaceName;
    std::string dbusServiceName;
    std::string dbusObjectPath;

    DBusAddressTranslator::getInstance().searchForDBusAddress(
        commonApiAddress,
        dbusInterfaceName,
        dbusServiceName,
        dbusObjectPath);

    std::lock_guard<std::mutex> dbusServicesLock(dbusServicesMutex_);
    auto dbusServiceListenersIterator = dbusServiceListenersMap.find(dbusServiceName);
    const bool isDBusServiceListenersRecordFound = (dbusServiceListenersIterator != dbusServiceListenersMap.end());

    if (!isDBusServiceListenersRecordFound) {
        return; // already unsubscribed
    }

    auto& dbusServiceListenersRecord = dbusServiceListenersIterator->second;
    auto dbusObjectPathListenersIterator =
                    dbusServiceListenersRecord.dbusObjectPathListenersMap.find(dbusObjectPath);
    const bool isDBusObjectPathListenersRecordFound =
                    (dbusObjectPathListenersIterator != dbusServiceListenersRecord.dbusObjectPathListenersMap.end());

    if (!isDBusObjectPathListenersRecordFound) {
        return; // already unsubscribed
    }

    auto& dbusInterfaceNameListenersMap = dbusObjectPathListenersIterator->second;
    auto dbusInterfaceNameListenersIterator = dbusInterfaceNameListenersMap.find(dbusInterfaceName);
    const bool isDBusInterfaceNameListenersRecordFound =
                    (dbusInterfaceNameListenersIterator != dbusInterfaceNameListenersMap.end());

    if (!isDBusInterfaceNameListenersRecordFound) {
        return; // already unsubscribed
    }

    auto& dbusInterfaceNameListenersRecord = dbusInterfaceNameListenersIterator->second;

    dbusInterfaceNameListenersRecord.listenerList.erase(listenerSubscription);

    if (dbusInterfaceNameListenersRecord.listenerList.empty()) {
        dbusInterfaceNameListenersMap.erase(dbusInterfaceNameListenersIterator);

        if (dbusInterfaceNameListenersMap.empty()) {
            dbusServiceListenersRecord.dbusObjectPathListenersMap.erase(dbusObjectPathListenersIterator);
        }
    }
}

// d-feet mode until service is found
bool DBusServiceRegistry::isServiceInstanceAlive(const std::string& dbusInterfaceName,
                                                 const std::string& dbusServiceName,
                                                 const std::string& dbusObjectPath) {
    std::chrono::milliseconds timeout(1000);
    bool uniqueNameFound = false;

    DBusUniqueNameRecord* dbusUniqueNameRecord = NULL;
    std::string uniqueName;

    dbusServicesMutex_.lock();

    findCachedDbusService(dbusServiceName, &dbusUniqueNameRecord);

    if(dbusUniqueNameRecord != NULL) {
        uniqueName = dbusUniqueNameRecord->uniqueName;
        uniqueNameFound = true;
    }

    if(!uniqueNameFound) {
        DBusServiceListenersRecord dbusServiceListenersRecord;
        dbusServiceListenersRecord.uniqueBusNameState = DBusRecordState::RESOLVING;

        dbusServiceListenersRecord.futureOnResolve = dbusServiceListenersRecord.promiseOnResolve.get_future();
        std::unordered_map<std::string, DBusServiceListenersRecord>::value_type value(dbusServiceName, std::move(dbusServiceListenersRecord));
        auto insertedDbusServiceListenerRecord = dbusServiceListenersMap.insert(std::move(value));

        if(insertedDbusServiceListenerRecord.second) { // if dbusServiceListenerRecord was inserted, start resolving
            resolveDBusServiceName(dbusServiceName, dbusServiceListenersMap[dbusServiceName]);
        }

        dbusServicesMutex_.unlock();

        std::shared_future<DBusRecordState> futureNameResolved = insertedDbusServiceListenerRecord.first->second.futureOnResolve;
        futureNameResolved.wait_for(timeout);

        if(futureNameResolved.get() != DBusRecordState::RESOLVED) {
            return false;
        }

        dbusServicesMutex_.lock();
        auto dbusServiceListenersMapIterator = dbusServiceListenersMap.find(dbusServiceName);

        if(dbusServiceListenersMapIterator == dbusServiceListenersMap.end()) {
            dbusServicesMutex_.unlock();
            return false;
        }

        uniqueName = dbusServiceListenersMapIterator->second.uniqueBusName;

        if(uniqueName.empty() || dbusServiceListenersMapIterator->second.uniqueBusNameState != DBusRecordState::RESOLVED) {
            dbusServicesMutex_.unlock();
            return false;
        }

        auto dbusUniqueNameRecordIterator = dbusUniqueNamesMap_.find(uniqueName);

        if(dbusUniqueNameRecordIterator == dbusUniqueNamesMap_.end()) {
            dbusServicesMutex_.unlock();
            return false;
        }

        dbusUniqueNameRecord = &dbusUniqueNameRecordIterator->second;
    }

    dbusServicesMutex_.unlock();

    assert(dbusUniqueNameRecord != NULL);

    auto* dbusObjectPathsCache = &(dbusUniqueNameRecord->dbusObjectPathsCache);
    auto dbusObjectPathCacheIterator = dbusObjectPathsCache->find(dbusObjectPath);

    DBusObjectPathCache* dbusObjectPathCache = NULL;


    if(dbusObjectPathCacheIterator != dbusObjectPathsCache->end()) {
        dbusObjectPathCache = &(dbusObjectPathCacheIterator->second);
    }
    else {
        // try to resolve object paths
        DBusObjectPathCache newDbusObjectPathCache;
        newDbusObjectPathCache.state = DBusRecordState::RESOLVING;

        dbusServicesMutex_.lock();
        //std::unordered_map<std::string, DBusObjectPathCache>::value_type value(dbusObjectPath, std::move(newDbusObjectPathCache));
        //auto dbusObjectPathCacheInserted = dbusObjectPathsCache->insert(std::move({dbusObjectPath, std::move(newDbusObjectPathCache)}));
        auto dbusObjectPathCacheInserted =
                        dbusObjectPathsCache->insert(std::make_pair(dbusObjectPath, std::move(newDbusObjectPathCache)));

        dbusObjectPathCacheIterator = dbusObjectPathsCache->find(dbusObjectPath);

        dbusObjectPathCache = &(dbusObjectPathCacheIterator->second);

        std::future<DBusRecordState> futureObjectPathResolved = dbusObjectPathCache->promiseOnResolve.get_future();
        dbusServicesMutex_.unlock();

        introspectDBusObjectPath(uniqueName, dbusObjectPath);
        futureObjectPathResolved.wait_for(timeout);
    }

    assert(dbusObjectPathCache != NULL);

    dbusServicesMutex_.lock();
    if(dbusObjectPathCache->state != DBusRecordState::RESOLVED) {
        dbusServicesMutex_.unlock();
        return false;
    }

    auto dbusInterfaceNamesIterator = dbusObjectPathCache->dbusInterfaceNamesCache.find(dbusInterfaceName);
    bool result = dbusInterfaceNamesIterator != dbusObjectPathCache->dbusInterfaceNamesCache.end();
    dbusServicesMutex_.unlock();

    return(result);
}

void DBusServiceRegistry::fetchAllServiceNames() {
    if (!dbusDaemonProxy_->isAvailable()) {
        return;
    }

    CallStatus callStatus;
    std::vector<std::string> availableServiceNames;

    dbusDaemonProxy_->listNames(callStatus, availableServiceNames);

    if(callStatus == CallStatus::SUCCESS) {
        for(std::string serviceName : availableServiceNames) {
            if(isDBusServiceName(serviceName)) {
                dbusServiceNameMap_[serviceName];
            }
        }
    }
}

// d-feet mode
std::vector<std::string> DBusServiceRegistry::getAvailableServiceInstances(const std::string& interfaceName,
                                                                           const std::string& domainName) {
    std::vector<std::string> availableServiceInstances;

    // resolve all service names
    for (auto serviceNameIterator = dbusServiceNameMap_.begin();
                    serviceNameIterator != dbusServiceNameMap_.end();
                    serviceNameIterator++) {

        std::string serviceName = serviceNameIterator->first;
        DBusUniqueNameRecord* dbusUniqueNameRecord = serviceNameIterator->second;

        if(dbusUniqueNameRecord == NULL) {
            DBusServiceListenersRecord& serviceListenerRecord = dbusServiceListenersMap[serviceName];
            if(serviceListenerRecord.uniqueBusNameState != DBusRecordState::RESOLVING) {
                resolveDBusServiceName(serviceName, serviceListenerRecord);
            }
        }
    }

    std::mutex mutexResolveAllServices;
    std::unique_lock<std::mutex> lockResolveAllServices(mutexResolveAllServices);
    std::chrono::milliseconds timeout(5000);

    monitorResolveAllServices_.wait_for(lockResolveAllServices, timeout, [&] {
        mutexServiceResolveCount.lock();
        bool finished = servicesToResolve == 0;
        mutexServiceResolveCount.unlock();

        return finished;
    });

    for (auto serviceNameIterator = dbusServiceNameMap_.begin();
                    serviceNameIterator != dbusServiceNameMap_.end();
                    serviceNameIterator++) {

        std::string serviceName = serviceNameIterator->first;
        DBusUniqueNameRecord* dbusUniqueNameRecord = serviceNameIterator->second;

        if(dbusUniqueNameRecord != NULL) {
            if(dbusUniqueNameRecord->objectPathsState == DBusRecordState::UNKNOWN) {
                DBusObjectPathCache& rootObjectPathCache = dbusUniqueNameRecord->dbusObjectPathsCache["/"];
                if(rootObjectPathCache.state == DBusRecordState::UNKNOWN) {
                    rootObjectPathCache.state = DBusRecordState::RESOLVING;
                    introspectDBusObjectPath(dbusUniqueNameRecord->uniqueName, "/");
                }
            }
        }
    }

    std::mutex mutexResolveAllObjectPaths;
    std::unique_lock<std::mutex> lockResolveAllObjectPaths(mutexResolveAllObjectPaths);

    // TODO: should use the remaining timeout not "used" during wait before
    monitorResolveAllObjectPaths_.wait_for(lockResolveAllObjectPaths, timeout, [&] {
        mutexServiceResolveCount.lock();
        bool finished = objectPathsToResolve == 0;
        mutexServiceResolveCount.unlock();

        return finished;
    });

    DBusAddressTranslator& dbusAddressTranslator = DBusAddressTranslator::getInstance();

    for (auto serviceNameIterator = dbusServiceNameMap_.begin();
                    serviceNameIterator != dbusServiceNameMap_.end();
                    serviceNameIterator++) {

        std::string serviceName = serviceNameIterator->first;
        DBusUniqueNameRecord* dbusUniqueNameRecord = serviceNameIterator->second;

        if(dbusUniqueNameRecord != NULL) {
            if(dbusUniqueNameRecord->objectPathsState == DBusRecordState::RESOLVED) {
                for (auto dbusObjectPathCacheIterator = dbusUniqueNameRecord->dbusObjectPathsCache.begin();
                                dbusObjectPathCacheIterator != dbusUniqueNameRecord->dbusObjectPathsCache.end();
                                dbusObjectPathCacheIterator++) {
                    if (dbusObjectPathCacheIterator->second.state == DBusRecordState::RESOLVED) {
                        if (dbusObjectPathCacheIterator->second.dbusInterfaceNamesCache.find(interfaceName)
                                        != dbusObjectPathCacheIterator->second.dbusInterfaceNamesCache.end()) {
                            std::string commonApiAddress;
                            dbusAddressTranslator.searchForCommonAddress(interfaceName, serviceName, dbusObjectPathCacheIterator->first, commonApiAddress);
                            availableServiceInstances.push_back(commonApiAddress);
                        }
                    }
                }
            }
        }
    }

    // maybe partial list but it contains everything we know for now
    return availableServiceInstances;
}

//std::vector<std::string> DBusServiceRegistry::getManagedObjects(const std::string& connectionName, const std::string& objectpath) {
//    if (auto iter = dbusServiceNameMap_.find(connectionName) != dbusServiceNameMap_.end()) {
//        DBusUniqueNameRecord* rec = iter->second;
//        if (rec->uniqueName != DBusRecordState::RESOLVED) {
//            return std::vector<std::string>();
//        } else {
//            rec->dbusObjectPathsCache
//        }
//
//    } else {
//        return std::vector<std::string>();
//    }
//}

void DBusServiceRegistry::getAvailableServiceInstancesAsync(Factory::GetAvailableServiceInstancesCallback callback,
                                                            const std::string& interfaceName,
                                                            const std::string& domainName) {
    //Necessary as service discovery might need some time, but the async version of "getAvailableServiceInstances"
    //shall return without delay.
    std::thread(
            [this, callback, interfaceName, domainName](std::shared_ptr<DBusServiceRegistry> selfRef) {
                auto availableServiceInstances = getAvailableServiceInstances(interfaceName, domainName);
                callback(availableServiceInstances);
            }, this->shared_from_this()
    ).detach();
}

SubscriptionStatus DBusServiceRegistry::onSignalDBusMessage(const DBusMessage& dbusMessage) {
    const std::string& dbusServiceUniqueName = dbusMessage.getSenderName();

    assert(dbusMessage.isSignalType());
    assert(dbusMessage.hasInterfaceName("org.freedesktop.DBus.ObjectManager"));
    assert(dbusMessage.hasMemberName("InterfacesAdded") || dbusMessage.hasMemberName("InterfacesRemoved"));

    DBusInputStream dbusInputStream(dbusMessage);
    std::string dbusObjectPath;
    std::unordered_set<std::string> dbusInterfaceNames;
    DBusRecordState dbusInterfaceNameState;

    dbusInputStream >> dbusObjectPath;

    bool added = false;

    if (dbusMessage.hasMemberName("InterfacesAdded")) {
        added = true;
        dbusInterfaceNameState = DBusRecordState::AVAILABLE;

        typedef std::unordered_map<std::string, bool> DBusPropertiesChangedDict;
        typedef std::unordered_map<std::string, DBusPropertiesChangedDict> DBusInterfacesAndPropertiesDict;
        typedef std::unordered_map<std::string, DBusInterfacesAndPropertiesDict> DBusObjectPathAndInterfacesDict;
        DBusObjectPathAndInterfacesDict dbusObjectPathAndInterfacesDict;
        dbusInputStream >> dbusObjectPathAndInterfacesDict;

        for (auto& dbusInterfaceIterator : dbusObjectPathAndInterfacesDict) {
            const auto& dbusInterfaceName = dbusInterfaceIterator.first;
            dbusInterfaceNames.insert(dbusInterfaceName);
        }
    } else {
        std::vector<std::string> removedDBusInterfaceNames;

        dbusInterfaceNameState = DBusRecordState::NOT_AVAILABLE;
        dbusInputStream >> removedDBusInterfaceNames;
        std::move(
            removedDBusInterfaceNames.begin(),
            removedDBusInterfaceNames.end(),
            std::inserter(dbusInterfaceNames, dbusInterfaceNames.begin()));
    }

    if (dbusInputStream.hasError()) {
        return SubscriptionStatus::RETAIN;
    }

    if (dbusInterfaceNames.empty()) {
        return SubscriptionStatus::RETAIN;
    }

    std::lock_guard<std::mutex> dbusServicesLock(dbusServicesMutex_);

    auto dbusServiceUniqueNameIterator = dbusUniqueNamesMap_.find(dbusServiceUniqueName);
    const bool isDBusServiceUniqueNameFound = (dbusServiceUniqueNameIterator != dbusUniqueNamesMap_.end());

    if (!isDBusServiceUniqueNameFound) {
        return SubscriptionStatus::CANCEL;
    }

    auto& dbusUniqueNameRecord = dbusServiceUniqueNameIterator->second;

    //auto dbusObjectPathIterator = dbusUniqueNameRecord.dbusObjectPathsCache.find(dbusObjectPath);
    //const bool isDBusObjectPathFound = (dbusObjectPathIterator != dbusUniqueNameRecord.dbusObjectPathsCache.end());

    /*
    if (!isDBusObjectPathFound) {
        return SubscriptionStatus::RETAIN;
    }
    */

    DBusObjectPathCache& dbusObjectPathRecord = dbusUniqueNameRecord.dbusObjectPathsCache[dbusObjectPath];
/*
    if (isDBusObjectPathFound) {
        dbusObjectPathRecord = &dbusObjectPathIterator->second;
    }
    else
    {
        DBusObjectPathCache dbusObjectPathRecord;
        auto insertionResult = dbusUniqueNameRecord.dbusObjectPathsCache.insert(std::make_pair(dbusObjectPath, std::move(dbusObjectPath)));
        auto objectPathCacheIterator = insertionResult.first;
        dbusObjectPathRecord = &(objectPathCacheIterator->second);
    }
*/

    if (dbusObjectPathRecord.state != DBusRecordState::RESOLVED) {
        return SubscriptionStatus::RETAIN;
    }

    for (const auto& dbusInterfaceName : dbusInterfaceNames) {
        if (dbusInterfaceNameState == DBusRecordState::AVAILABLE) {
            dbusObjectPathRecord.dbusInterfaceNamesCache.insert(dbusInterfaceName);
        } else {
            dbusObjectPathRecord.dbusInterfaceNamesCache.erase(dbusInterfaceName);
        }
    }

    notifyDBusServiceListeners(dbusUniqueNameRecord, dbusObjectPath, dbusInterfaceNames, dbusInterfaceNameState);

    return SubscriptionStatus::RETAIN;
}


void DBusServiceRegistry::resolveDBusServiceName(const std::string& dbusServiceName,
                                                 DBusServiceListenersRecord& dbusServiceListenersRecord) {
    assert(dbusServiceListenersRecord.uniqueBusNameState != DBusRecordState::RESOLVED);
    assert(dbusServiceListenersRecord.uniqueBusName.empty());

    mutexServiceResolveCount.lock();
    servicesToResolve++;
    mutexServiceResolveCount.unlock();

    if (dbusDaemonProxy_->isAvailable()) {
        dbusDaemonProxy_->getNameOwnerAsync(
            dbusServiceName,
            std::bind(
                &DBusServiceRegistry::onGetNameOwnerCallback,
                this->shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2,
                dbusServiceName));

        dbusServiceListenersRecord.uniqueBusNameState = DBusRecordState::RESOLVING;
    }
}

void DBusServiceRegistry::onGetNameOwnerCallback(const CallStatus& status,
                                                 std::string dbusServiceUniqueName,
                                                 const std::string& dbusServiceName) {
    std::lock_guard<std::mutex> dbusServicesLock(dbusServicesMutex_);

    auto dbusServiceListenerIterator = dbusServiceListenersMap.find(dbusServiceName);
    const bool isDBusServiceListenerRecordFound = (dbusServiceListenerIterator != dbusServiceListenersMap.end());

    if (!isDBusServiceListenerRecordFound) {
        return;
    }

    DBusServiceListenersRecord& dbusServiceListenersRecord = dbusServiceListenerIterator->second;

    if (status == CallStatus::SUCCESS) {
        onDBusServiceAvailable(dbusServiceName, dbusServiceUniqueName);
        if(dbusServiceListenersRecord.futureOnResolve.valid()) {
            dbusServiceListenersRecord.promiseOnResolve.set_value(DBusRecordState(dbusServiceListenersRecord.uniqueBusNameState));
        }
    } else {
        // try to fulfill open promises
        if(dbusServiceListenersRecord.futureOnResolve.valid()) {
            dbusServiceListenersRecord.promiseOnResolve.set_value(DBusRecordState::NOT_AVAILABLE);
        }

        onDBusServiceNotAvailable(dbusServiceListenersRecord);
    }

    mutexServiceResolveCount.lock();
    servicesToResolve--;
    mutexServiceResolveCount.unlock();
    monitorResolveAllServices_.notify_all();
}

DBusServiceRegistry::DBusRecordState DBusServiceRegistry::resolveDBusInterfaceNameState(const std::string& dbusInterfaceName,
                                                                                        const std::string& dbusObjectPath,
                                                                                        const std::string& dbusServiceName,
                                                                                        DBusServiceListenersRecord& dbusServiceListenersRecord) {
    assert(dbusServiceListenersRecord.uniqueBusNameState == DBusRecordState::RESOLVED);
    assert(!dbusServiceListenersRecord.uniqueBusName.empty());

    auto& dbusServiceUniqueNameRecord = dbusUniqueNamesMap_[dbusServiceListenersRecord.uniqueBusName];
    assert(!dbusServiceUniqueNameRecord.ownedBusNames.empty());

    auto& dbusObjectPathRecord = getDBusObjectPathCacheReference(
        dbusObjectPath,
        dbusServiceListenersRecord.uniqueBusName,
        dbusServiceUniqueNameRecord);

    if (dbusObjectPathRecord.state != DBusRecordState::RESOLVED) {
        return dbusObjectPathRecord.state;
    }

    auto dbusInterfaceNameIterator = dbusObjectPathRecord.dbusInterfaceNamesCache.find(dbusInterfaceName);
    const bool isDBusInterfaceNameFound =
                    (dbusInterfaceNameIterator != dbusObjectPathRecord.dbusInterfaceNamesCache.end());

    return isDBusInterfaceNameFound ? DBusRecordState::AVAILABLE : DBusRecordState::NOT_AVAILABLE;
}


DBusServiceRegistry::DBusObjectPathCache& DBusServiceRegistry::getDBusObjectPathCacheReference(const std::string& dbusObjectPath,
                                                                                               const std::string& dbusServiceUniqueName,
                                                                                               DBusUniqueNameRecord& dbusUniqueNameRecord) {
    const bool isFirstDBusObjectPathCache = dbusUniqueNameRecord.dbusObjectPathsCache.empty();

    auto dbusObjectPathCacheIterator = dbusUniqueNameRecord.dbusObjectPathsCache.find(dbusObjectPath);
    if(dbusObjectPathCacheIterator == dbusUniqueNameRecord.dbusObjectPathsCache.end()) {
        std::unordered_map<std::string, DBusObjectPathCache>::value_type value (dbusObjectPath, DBusObjectPathCache());
        dbusUniqueNameRecord.dbusObjectPathsCache.insert(std::move(value));
        dbusObjectPathCacheIterator = dbusUniqueNameRecord.dbusObjectPathsCache.find(dbusObjectPath);
    }

    if (isFirstDBusObjectPathCache) {
        auto dbusProxyConnection = dbusDaemonProxy_->getDBusConnection();
        const bool isSubscriptionSuccessful = dbusProxyConnection->addObjectManagerSignalMemberHandler(
            dbusServiceUniqueName,
            this);
        assert(isSubscriptionSuccessful);
    }

    if (dbusObjectPathCacheIterator->second.state == DBusRecordState::UNKNOWN
                    && introspectDBusObjectPath(dbusServiceUniqueName, dbusObjectPath)) {
        dbusObjectPathCacheIterator->second.state = DBusRecordState::RESOLVING;
    }

    return dbusObjectPathCacheIterator->second;
}

void DBusServiceRegistry::releaseDBusObjectPathCacheReference(const std::string& dbusObjectPath,
                                                              const DBusServiceListenersRecord& dbusServiceListenersRecord) {
    if (!dbusDaemonProxy_->isAvailable()) {
        return;
    }

    if (dbusServiceListenersRecord.uniqueBusNameState != DBusRecordState::RESOLVED) {
        return;
    }

    assert(!dbusServiceListenersRecord.uniqueBusName.empty());

    auto& dbusUniqueNameRecord = dbusUniqueNamesMap_[dbusServiceListenersRecord.uniqueBusName];
    assert(!dbusUniqueNameRecord.ownedBusNames.empty());
    assert(!dbusUniqueNameRecord.dbusObjectPathsCache.empty());

    auto dbusObjectPathCacheIterator = dbusUniqueNameRecord.dbusObjectPathsCache.find(dbusObjectPath);
    const bool isDBusObjectPathCacheFound = (dbusObjectPathCacheIterator != dbusUniqueNameRecord.dbusObjectPathsCache.end());
    assert(isDBusObjectPathCacheFound);

    auto& dbusObjectPathCache = dbusObjectPathCacheIterator->second;
    assert(dbusObjectPathCache.referenceCount > 0);

    dbusObjectPathCache.referenceCount--;

    if (dbusObjectPathCache.referenceCount == 0) {
        dbusUniqueNameRecord.dbusObjectPathsCache.erase(dbusObjectPathCacheIterator);

        const bool isLastDBusObjectPathCache = dbusUniqueNameRecord.dbusObjectPathsCache.empty();
        if (isLastDBusObjectPathCache) {
            auto dbusProxyConnection = dbusDaemonProxy_->getDBusConnection();
            const bool isSubscriptionCancelled = dbusProxyConnection->removeObjectManagerSignalMemberHandler(
                dbusServiceListenersRecord.uniqueBusName,
                this);
            assert(isSubscriptionCancelled);
        }
    }
}


bool DBusServiceRegistry::introspectDBusObjectPath(const std::string& dbusServiceUniqueName,
                                                   const std::string& dbusObjectPath) {
    bool isResolvingInProgress = false;
    auto dbusConnection = dbusDaemonProxy_->getDBusConnection();

    assert(!dbusServiceUniqueName.empty());

    if (dbusConnection->isConnected()) {
        mutexObjectPathsResolveCount.lock();
        objectPathsToResolve++;
        mutexObjectPathsResolveCount.unlock();

        DBusMessage dbusMessageCall = DBusMessage::createMethodCall(
            dbusServiceUniqueName,
            dbusObjectPath,
            "org.freedesktop.DBus.Introspectable",
            "Introspect");
        auto instrospectAsyncCallback = std::bind(
            &DBusServiceRegistry::onIntrospectCallback,
            this->shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2,
            dbusServiceUniqueName,
            dbusObjectPath);

        dbusConnection->sendDBusMessageWithReplyAsync(
            dbusMessageCall,
            DBusProxyAsyncCallbackHandler<std::string>::create(instrospectAsyncCallback),
            2000);

        isResolvingInProgress = true;
    }

    return isResolvingInProgress;
}

/**
 * Callback for org.freedesktop.DBus.Introspectable.Introspect
 *
 * This is the other end of checking if a dbus object path is available.
 * On success it'll extract all interface names from the xml data response.
 * Special interfaces that start with org.freedesktop will be ignored.
 *
 * @param status
 * @param xmlData
 * @param dbusServiceUniqueName
 * @param dbusObjectPath
 */
void DBusServiceRegistry::onIntrospectCallback(const CallStatus& callStatus,
                                               std::string xmlData,
                                               const std::string& dbusServiceUniqueName,
                                               const std::string& dbusObjectPath) {
    if (callStatus == CallStatus::SUCCESS) {
        parseIntrospectionData(xmlData, dbusObjectPath, dbusServiceUniqueName);
    }

    std::lock_guard<std::mutex> dbusServicesLock(dbusServicesMutex_);

    // Error CallStatus will result in empty parsedDBusInterfaceNameSet (and not available notification)

    auto dbusServiceUniqueNameIterator = dbusUniqueNamesMap_.find(dbusServiceUniqueName);
    const bool isDBusServiceUniqueNameFound = (dbusServiceUniqueNameIterator != dbusUniqueNamesMap_.end());

    if (!isDBusServiceUniqueNameFound) {
        return;
    }

    auto& dbusUniqueNameRecord = dbusServiceUniqueNameIterator->second;
    auto dbusObjectPathIterator = dbusUniqueNameRecord.dbusObjectPathsCache.find(dbusObjectPath);
    const bool isDBusObjectPathFound = (dbusObjectPathIterator != dbusUniqueNameRecord.dbusObjectPathsCache.end());

    if (!isDBusObjectPathFound) {
        return;
    }

    auto& dbusObjectPathRecord = dbusObjectPathIterator->second;

    dbusObjectPathRecord.state = DBusRecordState::RESOLVED;
    dbusObjectPathRecord.promiseOnResolve.set_value(dbusObjectPathRecord.state);
    mutexObjectPathsResolveCount.lock();
    objectPathsToResolve++;
    mutexObjectPathsResolveCount.unlock();
    monitorResolveAllObjectPaths_.notify_all();

    dbusUniqueNameRecord.objectPathsState = DBusRecordState::RESOLVED;

    notifyDBusServiceListeners(
        dbusUniqueNameRecord,
        dbusObjectPath,
        dbusObjectPathRecord.dbusInterfaceNamesCache,
        DBusRecordState::RESOLVED);
}

void DBusServiceRegistry::parseIntrospectionNode(const pugi::xml_node& node, const std::string& rootObjectPath, const std::string& fullObjectPath, const std::string& dbusServiceUniqueName) {
    std::string nodeName;

    for(pugi::xml_node& subNode : node.children()) {
        nodeName = std::string(subNode.name());

        if(nodeName == "node") {
            processIntrospectionObjectPath(subNode, rootObjectPath, dbusServiceUniqueName);
        }

        if(nodeName == "interface") {
            processIntrospectionInterface(subNode, rootObjectPath, fullObjectPath, dbusServiceUniqueName);
        }
    }
}

void DBusServiceRegistry::processIntrospectionObjectPath(const pugi::xml_node& node, const std::string& rootObjectPath, const std::string& dbusServiceUniqueName) {
    std::string fullObjectPath = rootObjectPath;

    if(fullObjectPath.at(fullObjectPath.length()-1) != '/') {
        fullObjectPath += "/";
    }

    fullObjectPath += std::string(node.attribute("name").as_string());

    DBusUniqueNameRecord& dbusUniqueNameRecord = dbusUniqueNamesMap_[dbusServiceUniqueName];
    DBusObjectPathCache& dbusObjectPathCache = dbusUniqueNameRecord.dbusObjectPathsCache[fullObjectPath];

    if(dbusObjectPathCache.state == DBusRecordState::UNKNOWN) {
        dbusObjectPathCache.state = DBusRecordState::RESOLVING;
        introspectDBusObjectPath(dbusServiceUniqueName, fullObjectPath);
    }

    for(pugi::xml_node subNode : node.children()) {
        parseIntrospectionNode(subNode, fullObjectPath, fullObjectPath, dbusServiceUniqueName);
    }
}

void DBusServiceRegistry::processIntrospectionInterface(const pugi::xml_node& node, const std::string& rootObjectPath, const std::string& fullObjectPath, const std::string& dbusServiceUniqueName) {
    std::string interfaceName = node.attribute("name").as_string();

    DBusUniqueNameRecord& dbusUniqueNameRecord = dbusUniqueNamesMap_[dbusServiceUniqueName];
    DBusObjectPathCache& dbusObjectPathCache = dbusUniqueNameRecord.dbusObjectPathsCache[fullObjectPath];

    if(!isOrgFreedesktopDBusInterface(interfaceName)) {
        dbusObjectPathCache.dbusInterfaceNamesCache.insert(interfaceName);
    }

    for(pugi::xml_node subNode : node.children()) {
        parseIntrospectionNode(subNode, rootObjectPath, fullObjectPath, dbusServiceUniqueName);
    }
}

void DBusServiceRegistry::parseIntrospectionData(const std::string& xmlData,
                                                 const std::string& rootObjectPath,
                                                 const std::string& dbusServiceUniqueName) {
    pugi::xml_document xmlDocument;
    pugi::xml_parse_result parsedResult = xmlDocument.load_buffer(xmlData.c_str(), xmlData.length(), pugi::parse_minimal, pugi::encoding_utf8);

    if(parsedResult.status != pugi::xml_parse_status::status_ok) {
        return;
    }

    const pugi::xml_node rootNode = xmlDocument.child("node");

    dbusServicesMutex_.lock();

    parseIntrospectionNode(rootNode, rootObjectPath, rootObjectPath, dbusServiceUniqueName);

    DBusUniqueNameRecord& dbusUniqueNameRecord = dbusUniqueNamesMap_[dbusServiceUniqueName];
    dbusServicesMutex_.unlock();
}


SubscriptionStatus DBusServiceRegistry::onDBusDaemonProxyStatusEvent(const AvailabilityStatus& availabilityStatus) {
    assert(availabilityStatus != AvailabilityStatus::UNKNOWN);

    std::lock_guard<std::mutex> dbusServicesLock(dbusServicesMutex_);

    for (auto& dbusServiceListenersIterator : dbusServiceListenersMap) {
        const auto& dbusServiceName = dbusServiceListenersIterator.first;
        auto& dbusServiceListenersRecord = dbusServiceListenersIterator.second;

        if (availabilityStatus == AvailabilityStatus::AVAILABLE) {
            resolveDBusServiceName(dbusServiceName, dbusServiceListenersRecord);
        } else {
            onDBusServiceNotAvailable(dbusServiceListenersRecord);
        }
    }

    return SubscriptionStatus::RETAIN;
}

void DBusServiceRegistry::checkDBusServiceWasAvailable(const std::string& dbusServiceName,
                                                       const std::string& dbusServiceUniqueName) {

    auto dbusUniqueNameIterator = dbusUniqueNamesMap_.find(dbusServiceUniqueName);
    const bool isDBusUniqueNameFound = (dbusUniqueNameIterator != dbusUniqueNamesMap_.end());

    if (isDBusUniqueNameFound) {
        auto& dbusServiceListenersRecord = dbusServiceListenersMap[dbusServiceName];
        onDBusServiceNotAvailable(dbusServiceListenersRecord);
    }
}

SubscriptionStatus DBusServiceRegistry::onDBusDaemonProxyNameOwnerChangedEvent(const std::string& affectedName,
                                                                               const std::string& oldOwner,
                                                                               const std::string& newOwner) {
    if (!isDBusServiceName(affectedName)) {
        return SubscriptionStatus::RETAIN;
    }

    const bool isDBusServiceNameLost = newOwner.empty();
    const std::string& dbusServiceUniqueName = (isDBusServiceNameLost ? oldOwner : newOwner);

    std::lock_guard<std::mutex> dbusServicesLock(dbusServicesMutex_);

    if (isDBusServiceNameLost) {
        checkDBusServiceWasAvailable(affectedName, dbusServiceUniqueName);
    } else {
        onDBusServiceAvailable(affectedName, dbusServiceUniqueName);
    }

    return SubscriptionStatus::RETAIN;
}


void DBusServiceRegistry::onDBusServiceAvailable(const std::string& dbusServiceName,
                                                 const std::string& dbusServiceUniqueName) {
    DBusUniqueNameRecord* dbusUniqueNameRecord = insertServiceNameMapping(dbusServiceUniqueName, dbusServiceName);

    auto& dbusServiceListenersRecord = dbusServiceListenersMap[dbusServiceName];
    const bool isDBusServiceNameObserved = !dbusServiceListenersRecord.dbusObjectPathListenersMap.empty();

    if (dbusServiceListenersRecord.uniqueBusNameState == DBusRecordState::RESOLVED
                    && dbusServiceListenersRecord.uniqueBusName != dbusServiceUniqueName) {
        //A new unique connection name claims an already claimed name
        //-> release of old name and claim of new name arrive in reverted order.
        //Therefore: Release of old and proceed with claiming of new owner.
        checkDBusServiceWasAvailable(dbusServiceName, dbusServiceListenersRecord.uniqueBusName);
    }

    dbusServiceListenersRecord.uniqueBusNameState = DBusRecordState::RESOLVED;
    dbusServiceListenersRecord.uniqueBusName = dbusServiceUniqueName;

    if (!isDBusServiceNameObserved) {
        return;
    }

    // resolve object path and notify service listners
    for (auto dbusObjectPathListenersIterator = dbusServiceListenersRecord.dbusObjectPathListenersMap.begin();
                    dbusObjectPathListenersIterator != dbusServiceListenersRecord.dbusObjectPathListenersMap.end();) {
        const std::string& listenersDBusObjectPath = dbusObjectPathListenersIterator->first;
        auto& dbusInterfaceNameListenersMap = dbusObjectPathListenersIterator->second;
        auto& dbusObjectPathRecord = getDBusObjectPathCacheReference(
            listenersDBusObjectPath,
            dbusServiceUniqueName,
            *dbusUniqueNameRecord);

        if (dbusObjectPathRecord.state == DBusRecordState::RESOLVED) {
            notifyDBusObjectPathResolved(dbusInterfaceNameListenersMap, dbusObjectPathRecord.dbusInterfaceNamesCache);
        }

        if (dbusInterfaceNameListenersMap.empty()) {
            dbusObjectPathListenersIterator = dbusServiceListenersRecord.dbusObjectPathListenersMap.erase(
                dbusObjectPathListenersIterator);
        } else {
            dbusObjectPathListenersIterator++;
        }
    }
}

void DBusServiceRegistry::onDBusServiceNotAvailable(DBusServiceListenersRecord& dbusServiceListenersRecord) {
    const std::unordered_set<std::string> dbusInterfaceNamesCache;

    const DBusUniqueNamesMapIterator dbusUniqueNameRecordIterator = dbusUniqueNamesMap_.find(dbusServiceListenersRecord.uniqueBusName);

    // fulfill all open promises on object path resolution
    if(dbusUniqueNameRecordIterator != dbusUniqueNamesMap_.end()) {
        DBusUniqueNameRecord& dbusUniqueNameRecord = dbusUniqueNameRecordIterator->second;
        for (auto dbusObjectPathsCacheIterator = dbusUniqueNameRecord.dbusObjectPathsCache.begin();
                        dbusObjectPathsCacheIterator != dbusUniqueNameRecord.dbusObjectPathsCache.end();
                        dbusObjectPathsCacheIterator++) {

            auto& dbusObjectPathsCache = dbusObjectPathsCacheIterator->second;

            std::promise<DBusRecordState> promiseOnResolve = std::move(dbusObjectPathsCache.promiseOnResolve);

            try {
                std::future<DBusRecordState> futureOnResolve = promiseOnResolve.get_future();
                if(!futureOnResolve.valid()) {
                    promiseOnResolve.set_value(DBusRecordState::NOT_AVAILABLE);
                }
            } catch (std::future_error& e) { }

        }

        removeUniqueName(dbusUniqueNameRecordIterator);
    }

    dbusServiceListenersRecord.uniqueBusName.clear();
    dbusServiceListenersRecord.uniqueBusNameState = DBusRecordState::NOT_AVAILABLE;


    for (auto dbusObjectPathListenersIterator = dbusServiceListenersRecord.dbusObjectPathListenersMap.begin();
                    dbusObjectPathListenersIterator != dbusServiceListenersRecord.dbusObjectPathListenersMap.end(); ) {
        auto& dbusInterfaceNameListenersMap = dbusObjectPathListenersIterator->second;

        notifyDBusObjectPathResolved(dbusInterfaceNameListenersMap, dbusInterfaceNamesCache);

        if (dbusInterfaceNameListenersMap.empty()) {
            dbusObjectPathListenersIterator = dbusServiceListenersRecord.dbusObjectPathListenersMap.erase(
                dbusObjectPathListenersIterator);
        } else {
            dbusObjectPathListenersIterator++;
        }
    }
}

void DBusServiceRegistry::notifyDBusServiceListeners(const DBusUniqueNameRecord& dbusUniqueNameRecord,
                                                     const std::string& dbusObjectPath,
                                                     const std::unordered_set<std::string>& dbusInterfaceNames,
                                                     const DBusRecordState& dbusInterfaceNamesState) {
    notificationThread_ = std::this_thread::get_id();

    for (auto& dbusServiceName : dbusUniqueNameRecord.ownedBusNames) {
        auto dbusServiceListenersIterator = dbusServiceListenersMap.find(dbusServiceName);

        if(dbusServiceListenersIterator == dbusServiceListenersMap.end()) {
            continue;
        }

        auto& dbusServiceListenersRecord = dbusServiceListenersIterator->second;
        if(dbusServiceListenersRecord.uniqueBusNameState != DBusRecordState::RESOLVED) {
            continue;
        }

        auto dbusObjectPathListenersIterator = dbusServiceListenersRecord.dbusObjectPathListenersMap.find(dbusObjectPath);
        const bool isDBusObjectPathListenersRecordFound =
                        (dbusObjectPathListenersIterator != dbusServiceListenersRecord.dbusObjectPathListenersMap.end());

        if (!isDBusObjectPathListenersRecordFound) {
            continue; // skip
        }

        auto& dbusInterfaceNameListenersMap = dbusObjectPathListenersIterator->second;

        if (dbusInterfaceNamesState == DBusRecordState::RESOLVED) {
            notifyDBusObjectPathResolved(dbusInterfaceNameListenersMap, dbusInterfaceNames);
        } else {
            notifyDBusObjectPathChanged(dbusInterfaceNameListenersMap, dbusInterfaceNames, dbusInterfaceNamesState);
        }

        if (dbusInterfaceNameListenersMap.empty()) {
            dbusServiceListenersRecord.dbusObjectPathListenersMap.erase(dbusObjectPathListenersIterator);
        }
    }

    notificationThread_ = std::thread::id();
}

void DBusServiceRegistry::notifyDBusObjectPathResolved(DBusInterfaceNameListenersMap& dbusInterfaceNameListenersMap,
                                                       const std::unordered_set<std::string>& dbusInterfaceNames) {
    for (auto dbusObjectPathListenersIterator = dbusInterfaceNameListenersMap.begin();
                    dbusObjectPathListenersIterator != dbusInterfaceNameListenersMap.end();) {
        const auto& listenersDBusInterfaceName = dbusObjectPathListenersIterator->first;
        auto& dbusInterfaceNameListenersRecord = dbusObjectPathListenersIterator->second;

        const auto& dbusInterfaceNameIterator = dbusInterfaceNames.find(listenersDBusInterfaceName);
        const bool isDBusInterfaceNameAvailable = (dbusInterfaceNameIterator != dbusInterfaceNames.end());

        notifyDBusInterfaceNameListeners(dbusInterfaceNameListenersRecord, isDBusInterfaceNameAvailable);

        if (dbusInterfaceNameListenersRecord.listenerList.empty()) {
            dbusObjectPathListenersIterator = dbusInterfaceNameListenersMap.erase(dbusObjectPathListenersIterator);
        } else {
            dbusObjectPathListenersIterator++;
        }
    }
}

void DBusServiceRegistry::notifyDBusObjectPathChanged(DBusInterfaceNameListenersMap& dbusInterfaceNameListenersMap,
                                                      const std::unordered_set<std::string>& dbusInterfaceNames,
                                                      const DBusRecordState& dbusInterfaceNamesState) {
    const bool isDBusInterfaceNameAvailable = (dbusInterfaceNamesState == DBusRecordState::AVAILABLE);

    assert(
        dbusInterfaceNamesState == DBusRecordState::AVAILABLE
                        || dbusInterfaceNamesState == DBusRecordState::NOT_AVAILABLE);

    for (const auto& dbusInterfaceName : dbusInterfaceNames) {
        auto dbusInterfaceNameListenersIterator = dbusInterfaceNameListenersMap.find(dbusInterfaceName);
        const bool isDBusInterfaceNameObserved = (dbusInterfaceNameListenersIterator
                        != dbusInterfaceNameListenersMap.end());

        if (isDBusInterfaceNameObserved) {
            auto& dbusInterfaceNameListenersRecord = dbusInterfaceNameListenersIterator->second;

            notifyDBusInterfaceNameListeners(dbusInterfaceNameListenersRecord, isDBusInterfaceNameAvailable);
        }
    }
}

void DBusServiceRegistry::notifyDBusInterfaceNameListeners(DBusInterfaceNameListenersRecord& dbusInterfaceNameListenersRecord,
                                                           const bool& isDBusInterfaceNameAvailable) {

    const AvailabilityStatus availabilityStatus = (isDBusInterfaceNameAvailable ?
                    AvailabilityStatus::AVAILABLE : AvailabilityStatus::NOT_AVAILABLE);
    const DBusRecordState notifyState = (isDBusInterfaceNameAvailable ?
                    DBusRecordState::AVAILABLE : DBusRecordState::NOT_AVAILABLE);

    if (notifyState == dbusInterfaceNameListenersRecord.state) {
        return;
    }
    dbusInterfaceNameListenersRecord.state = notifyState;

    for (auto dbusServiceListenerIterator = dbusInterfaceNameListenersRecord.listenerList.begin();
                    dbusServiceListenerIterator != dbusInterfaceNameListenersRecord.listenerList.end();) {
        const auto& dbusServiceListener = *dbusServiceListenerIterator;

        if (dbusServiceListener(availabilityStatus) != SubscriptionStatus::RETAIN) {
            dbusServiceListenerIterator = dbusInterfaceNameListenersRecord.listenerList.erase(
                dbusServiceListenerIterator);
        } else {
            dbusServiceListenerIterator++;
        }
    }
}

void DBusServiceRegistry::removeUniqueName(const DBusUniqueNamesMapIterator& dbusUniqueNamesIterator) {
    for (auto dbusServiceNamesIterator = dbusUniqueNamesIterator->second.ownedBusNames.begin();
                    dbusServiceNamesIterator != dbusUniqueNamesIterator->second.ownedBusNames.end();
                    dbusServiceNamesIterator++) {
        dbusServiceNameMap_.erase(*dbusServiceNamesIterator);
    }

    dbusUniqueNamesMap_.erase(dbusUniqueNamesIterator);
}

DBusServiceRegistry::DBusUniqueNameRecord* DBusServiceRegistry::insertServiceNameMapping(const std::string& dbusUniqueName,
                                                                                         const std::string& dbusServiceName) {
    auto* dbusUniqueNameRecord = &(dbusUniqueNamesMap_[dbusUniqueName]);
    dbusUniqueNameRecord->uniqueName = dbusUniqueName;
    dbusUniqueNameRecord->ownedBusNames.insert(dbusServiceName);

    auto dbusServiceNameMapIterator = dbusServiceNameMap_.find(dbusServiceName);

    if(dbusServiceNameMapIterator == dbusServiceNameMap_.end()) {
        dbusServiceNameMap_.insert({ dbusServiceName, dbusUniqueNameRecord });
    }
    else {
        dbusServiceNameMapIterator->second = dbusUniqueNameRecord;
    }

    DBusServiceListenersRecord& dbusServiceListenersRecord = dbusServiceListenersMap[dbusServiceName];

    if(dbusServiceListenersRecord.uniqueBusNameState != DBusRecordState::RESOLVED) {
        dbusServiceListenersRecord.uniqueBusName = dbusUniqueName;
        dbusServiceListenersRecord.uniqueBusNameState = DBusRecordState::UNKNOWN;
    }

    return dbusUniqueNameRecord;
}

/**
 * finds a DBusUniquNameRecord associated with a given well-known name.
 * The returned DBusUniquNameRecord* may be a NULL pointer, if the well-known
 * name is known, but not associated with a unique name yet.
 *
 * @return true, if the given well-known name is found
 */
bool DBusServiceRegistry::findCachedDbusService(
                const std::string& dbusServiceName,
                DBusUniqueNameRecord** uniqueNameRecord) {
    auto dbusUniqueNameRecordIterator = dbusServiceNameMap_.find(dbusServiceName);

    if(dbusUniqueNameRecordIterator != dbusServiceNameMap_.end()) {
        *uniqueNameRecord = dbusUniqueNameRecordIterator->second;
        return true;
    }

    return false;
}

} // namespace DBus
} // namespace CommonAPI
