/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_SERVICE_REGISTRY_H_
#define COMMONAPI_DBUS_DBUS_SERVICE_REGISTRY_H_

#include <CommonAPI/types.h>
#include <CommonAPI/Attribute.h>
#include <CommonAPI/Proxy.h>
#include <CommonAPI/Factory.h>

#include "DBusProxyConnection.h"
#include "DBusAddressTranslator.h"
#include "DBusDaemonProxy.h"

#include "pugixml/pugixml.hpp"

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <map>
#include <unordered_set>
#include <string>
#include <vector>
#include <memory>
#include <list>
#include <algorithm>
#include <set>

#include <condition_variable>
#include <mutex>
#include <future>

namespace CommonAPI {
namespace DBus {

typedef Event<std::string, std::string, std::string> NameOwnerChangedEvent;
typedef Event<std::string, std::string, std::string>::Subscription NameOwnerChangedEventSubscription;

//connectionName, objectPath
typedef std::pair<std::string, std::string> DBusInstanceId;

class DBusProxyConnection;
class DBusDaemonProxy;

class DBusServiceRegistry: public std::enable_shared_from_this<DBusServiceRegistry>,
                           public DBusProxyConnection::DBusSignalHandler {
 public:
    enum class DBusRecordState {
        UNKNOWN,
        AVAILABLE,
        RESOLVING,
        RESOLVED,
        NOT_AVAILABLE
    };

    // template class DBusServiceListener<> { typedef functor; typedef list; typedef subscription }
    typedef std::function<SubscriptionStatus(const AvailabilityStatus& availabilityStatus)> DBusServiceListener;
    typedef std::list<DBusServiceListener> DBusServiceListenerList;
    typedef DBusServiceListenerList::iterator DBusServiceSubscription;


    typedef std::function<SubscriptionStatus(const std::vector<std::string>& interfaces,
                                             const AvailabilityStatus& availabilityStatus)> DBusManagedInterfaceListener;
    typedef std::list<DBusManagedInterfaceListener> DBusManagedInterfaceListenerList;
    typedef DBusManagedInterfaceListenerList::iterator DBusManagedInterfaceSubscription;


    DBusServiceRegistry(std::shared_ptr<DBusProxyConnection> dbusProxyConnection);

    DBusServiceRegistry(const DBusServiceRegistry&) = delete;
    DBusServiceRegistry& operator=(const DBusServiceRegistry&) = delete;

    virtual ~DBusServiceRegistry();

    void init();


    DBusServiceSubscription subscribeAvailabilityListener(const std::string& commonApiAddress,
                                                          DBusServiceListener serviceListener);

    void unsubscribeAvailabilityListener(const std::string& commonApiAddress,
                                         DBusServiceSubscription& listenerSubscription);


    bool isServiceInstanceAlive(const std::string& dbusInterfaceName,
                                const std::string& dbusConnectionName,
                                const std::string& dbusObjectPath);


    virtual std::vector<std::string> getAvailableServiceInstances(const std::string& interfaceName,
                                                                  const std::string& domainName = "local");

    virtual void getAvailableServiceInstancesAsync(Factory::GetAvailableServiceInstancesCallback callback,
                                                   const std::string& interfaceName,
                                                   const std::string& domainName = "local");


    virtual SubscriptionStatus onSignalDBusMessage(const DBusMessage&);

 private:
    struct DBusInterfaceNameListenersRecord {
        DBusInterfaceNameListenersRecord(): state(DBusRecordState::UNKNOWN) {
        }

        DBusInterfaceNameListenersRecord(DBusInterfaceNameListenersRecord&& other):
            state(other.state),
            listenerList(std::move(other.listenerList))
        {
        }

        DBusRecordState state;
        DBusServiceListenerList listenerList;
    };

    typedef std::unordered_map<std::string, DBusInterfaceNameListenersRecord> DBusInterfaceNameListenersMap;

    struct DBusServiceListenersRecord {
        DBusServiceListenersRecord(): uniqueBusNameState(DBusRecordState::UNKNOWN),
                        mutexOnResolve() {
        }

        DBusServiceListenersRecord(DBusServiceListenersRecord&& other):
            uniqueBusNameState(other.uniqueBusNameState),
            uniqueBusName(std::move(other.uniqueBusName)),
            promiseOnResolve(std::move(other.promiseOnResolve)),
            futureOnResolve(std::move(other.futureOnResolve)),
            mutexOnResolve(std::move(other.mutexOnResolve)),
            dbusObjectPathListenersMap(std::move(other.dbusObjectPathListenersMap))
        {}

        ~DBusServiceListenersRecord() {};

        DBusRecordState uniqueBusNameState;
        std::string uniqueBusName;

        std::promise<DBusRecordState> promiseOnResolve;
        std::shared_future<DBusRecordState> futureOnResolve;
        std::unique_lock<std::mutex>* mutexOnResolve;

        std::unordered_map<std::string, DBusInterfaceNameListenersMap> dbusObjectPathListenersMap;
    };

    std::unordered_map<std::string, DBusServiceListenersRecord> dbusServiceListenersMap;


    struct DBusObjectPathCache {
        DBusObjectPathCache(): referenceCount(0), state(DBusRecordState::UNKNOWN) {
        }

        DBusObjectPathCache(DBusObjectPathCache&& other):
            referenceCount(other.referenceCount),
            state(other.state),
            promiseOnResolve(std::move(other.promiseOnResolve)),
            dbusInterfaceNamesCache(std::move(other.dbusInterfaceNamesCache))
        {
            /*other.promiseOnResolve = NULL;
            other.dbusInterfaceNamesCache = NULL;*/
        }

        ~DBusObjectPathCache() {}

        size_t referenceCount;
        DBusRecordState state;
        std::promise<DBusRecordState> promiseOnResolve;

        std::unordered_set<std::string> dbusInterfaceNamesCache;
    };

    struct DBusUniqueNameRecord {
        DBusUniqueNameRecord(): objectPathsState(DBusRecordState::UNKNOWN) {
        }

        DBusUniqueNameRecord(DBusUniqueNameRecord&& other) :
            uniqueName(std::move(other.uniqueName)),
            objectPathsState(other.objectPathsState),
            ownedBusNames(std::move(other.ownedBusNames)),
            dbusObjectPathsCache(std::move(other.dbusObjectPathsCache))
        {}

        std::string uniqueName;
        DBusRecordState objectPathsState;
        std::unordered_set<std::string> ownedBusNames;
        std::unordered_map<std::string, DBusObjectPathCache> dbusObjectPathsCache;
    };

    std::unordered_map<std::string, DBusUniqueNameRecord> dbusUniqueNamesMap_;
    typedef std::unordered_map<std::string, DBusUniqueNameRecord>::iterator DBusUniqueNamesMapIterator;

    // mapping service names (well-known names) to service instances
    std::unordered_map<std::string, DBusUniqueNameRecord*> dbusServiceNameMap_;


    // protects the dbus service maps
    std::mutex dbusServicesMutex_;


    void resolveDBusServiceName(const std::string& dbusServiceName,
                                DBusServiceListenersRecord& dbusServiceListenersRecord);

    void onGetNameOwnerCallback(const CallStatus& status, std::string dbusServiceUniqueName, const std::string& dbusServiceName);


    DBusRecordState resolveDBusInterfaceNameState(const std::string& dbusInterfaceName,
                                                  const std::string& dbusObjectPath,
                                                  const std::string& dbusServiceName,
                                                  DBusServiceListenersRecord& dbusServiceListenersRecord);


    DBusObjectPathCache& getDBusObjectPathCacheReference(const std::string& dbusObjectPath,
                                                         const std::string& dbusServiceUniqueName,
                                                         DBusUniqueNameRecord& dbusUniqueNameRecord);

    void releaseDBusObjectPathCacheReference(const std::string& dbusObjectPath,
                                             const DBusServiceListenersRecord& dbusServiceListenersRecord);


    bool introspectDBusObjectPath(const std::string& dbusServiceUniqueName, const std::string& dbusObjectPath);

    void onIntrospectCallback(const CallStatus& status,
                              std::string xmlData,
                              const std::string& dbusServiceName,
                              const std::string& dbusObjectPath);

    void parseIntrospectionData(const std::string& xmlData,
                                const std::string& rootObjectPath,
                                const std::string& dbusServiceUniqueName);

    void parseIntrospectionNode(const pugi::xml_node& node,
                                const std::string& rootObjectPath,
                                const std::string& fullObjectPath,
                                const std::string& dbusServiceUniqueName);

    void processIntrospectionObjectPath(const pugi::xml_node& node,
                                        const std::string& rootObjectPath,
                                        const std::string& dbusServiceUniqueName);

    void processIntrospectionInterface(const pugi::xml_node& node,
                                       const std::string& rootObjectPath,
                                       const std::string& fullObjectPath,
                                       const std::string& dbusServiceUniqueName);

    SubscriptionStatus onDBusDaemonProxyStatusEvent(const AvailabilityStatus& availabilityStatus);

    SubscriptionStatus onDBusDaemonProxyNameOwnerChangedEvent(const std::string& name,
                                                              const std::string& oldOwner,
                                                              const std::string& newOwner);

    std::shared_ptr<DBusDaemonProxy> dbusDaemonProxy_;
    bool initialized_;

    ProxyStatusEvent::Subscription dbusDaemonProxyStatusEventSubscription_;
    NameOwnerChangedEvent::Subscription dbusDaemonProxyNameOwnerChangedEventSubscription_;


    void checkDBusServiceWasAvailable(const std::string& dbusServiceName, const std::string& dbusServiceUniqueName);

    void onDBusServiceAvailable(const std::string& dbusServiceName, const std::string& dbusServiceUniqueName);

    void onDBusServiceNotAvailable(DBusServiceListenersRecord& dbusServiceListenersRecord);


    void notifyDBusServiceListeners(const DBusUniqueNameRecord& dbusUniqueNameRecord,
                                    const std::string& dbusObjectPath,
                                    const std::unordered_set<std::string>& dbusInterfaceNames,
                                    const DBusRecordState& dbusInterfaceNamesState);

    void notifyDBusObjectPathResolved(DBusInterfaceNameListenersMap& dbusInterfaceNameListenersMap,
                                      const std::unordered_set<std::string>& dbusInterfaceNames);

    void notifyDBusObjectPathChanged(DBusInterfaceNameListenersMap& dbusInterfaceNameListenersMap,
                                     const std::unordered_set<std::string>& dbusInterfaceNames,
                                     const DBusRecordState& dbusInterfaceNamesState);

    void notifyDBusInterfaceNameListeners(DBusInterfaceNameListenersRecord& dbusInterfaceNameListenersRecord,
                                          const bool& isDBusInterfaceNameAvailable);


    void removeUniqueName(const DBusUniqueNamesMapIterator& dbusUniqueName);
    DBusUniqueNameRecord* insertServiceNameMapping(const std::string& dbusUniqueName, const std::string& dbusServiceName);
    bool findCachedDbusService(const std::string& dbusServiceName, DBusUniqueNameRecord** uniqueNameRecord);
    bool findCachedObjectPath(const std::string& dbusObjectPathName, const DBusUniqueNameRecord* uniqueNameRecord, DBusObjectPathCache* objectPathCache);

    std::condition_variable monitorResolveAllServices_;
    std::mutex mutexServiceResolveCount;
    int servicesToResolve;

    std::condition_variable monitorResolveAllObjectPaths_;
    std::mutex mutexObjectPathsResolveCount;
    int objectPathsToResolve;


    void fetchAllServiceNames();

    inline const bool isDBusServiceName(const std::string& serviceName) {
        return (serviceName.length() > 0 && serviceName[0] != ':');
    };


    inline const bool isOrgFreedesktopDBusInterface(const std::string& dbusInterfaceName) {
        return dbusInterfaceName.find("org.freedesktop.DBus.") == 0;
    }

    std::thread::id notificationThread_;
};


} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_SERVICE_REGISTRY_H_
