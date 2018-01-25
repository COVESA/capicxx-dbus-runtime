// Copyright (C) 2013-2017 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.hpp> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUSSERVICEREGISTRY_HPP_
#define COMMONAPI_DBUS_DBUSSERVICEREGISTRY_HPP_

#include <algorithm>
#include <condition_variable>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <list>

#include <pugixml/pugixml.hpp>

#include <CommonAPI/Attribute.hpp>
#include <CommonAPI/Proxy.hpp>
#include <CommonAPI/Types.hpp>
#include <CommonAPI/DBus/DBusProxyConnection.hpp>
#include <CommonAPI/DBus/DBusFactory.hpp>
#include <CommonAPI/DBus/DBusObjectManagerStub.hpp>

namespace CommonAPI {
namespace DBus {

typedef Event<std::string, std::string, std::string> NameOwnerChangedEvent;
typedef Event<std::string, std::string, std::string>::Subscription NameOwnerChangedEventSubscription;

// Connection name, Object path
typedef std::pair<std::string, std::string> DBusInstanceId;

class DBusAddress;
class DBusAddressTranslator;
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
    typedef std::function<void(std::shared_ptr<DBusProxy>, const AvailabilityStatus& availabilityStatus)> DBusServiceListener;
    typedef long int DBusServiceSubscription;
    struct DBusServiceListenerInfo {
        DBusServiceListener listener;
        std::weak_ptr<DBusProxy> proxy;
    };
    typedef std::map<DBusServiceSubscription, std::shared_ptr<DBusServiceListenerInfo>> DBusServiceListenerList;

    typedef std::function<void(const std::vector<std::string>& interfaces,
                               const AvailabilityStatus& availabilityStatus)> DBusManagedInterfaceListener;
    typedef std::list<DBusManagedInterfaceListener> DBusManagedInterfaceListenerList;
    typedef DBusManagedInterfaceListenerList::iterator DBusManagedInterfaceSubscription;

    typedef std::function<void(const DBusObjectManagerStub::DBusObjectPathAndInterfacesDict)> GetAvailableServiceInstancesCallback;

    static std::shared_ptr<DBusServiceRegistry> get(std::shared_ptr<DBusProxyConnection> _connection, bool _insert=true);
    static void remove(std::shared_ptr<DBusProxyConnection> _connection);

    DBusServiceRegistry(std::shared_ptr<DBusProxyConnection> dbusProxyConnection);

    DBusServiceRegistry(const DBusServiceRegistry&) = delete;
    DBusServiceRegistry& operator=(const DBusServiceRegistry&) = delete;

    virtual ~DBusServiceRegistry();

    void init();

    DBusServiceSubscription subscribeAvailabilityListener(const std::string &_address,
                                                          DBusServiceListener _listener,
                                                          std::weak_ptr<DBusProxy> _proxy);

    void unsubscribeAvailabilityListener(const std::string &_address,
                                         DBusServiceSubscription &_listener);


    bool isServiceInstanceAlive(const std::string &_dbusInterfaceName,
                                const std::string &_dbusConnectionName,
                                const std::string &_dbusObjectPath);


    virtual void getAvailableServiceInstances(const std::string& dbusServiceName,
            const std::string& dbusObjectPath,
            DBusObjectManagerStub::DBusObjectPathAndInterfacesDict& availableServiceInstances);

    virtual void getAvailableServiceInstancesAsync(GetAvailableServiceInstancesCallback callback,
                                                   const std::string& dbusServiceName,
                                                   const std::string& dbusObjectPath);

    virtual void onSignalDBusMessage(const DBusMessage&);

    void setDBusServicePredefined(const std::string& _serviceName);

 private:
    struct DBusInterfaceNameListenersRecord {
        DBusInterfaceNameListenersRecord()
            : state(DBusRecordState::UNKNOWN),
              nextSubscriptionKey(0) {
        }

        DBusRecordState state;
        DBusServiceListenerList listenerList;
        std::list<DBusServiceSubscription> listenersToRemove;
        DBusServiceSubscription nextSubscriptionKey;
    };

    typedef std::unordered_map<std::string, DBusInterfaceNameListenersRecord> DBusInterfaceNameListenersMap;

    struct DBusServiceListenersRecord {
        DBusServiceListenersRecord()
            : uniqueBusNameState(DBusRecordState::UNKNOWN),
              promiseOnResolve(std::make_shared<std::promise<DBusRecordState>>()),
              mutexOnResolve() {
        }

        ~DBusServiceListenersRecord() { 
            if(uniqueBusNameState == DBusRecordState::RESOLVING && futureOnResolve.valid())
                promiseOnResolve->set_value(DBusRecordState::NOT_AVAILABLE);
        };

        DBusRecordState uniqueBusNameState;
        std::string uniqueBusName;

        std::shared_ptr<std::promise<DBusRecordState>> promiseOnResolve;
        std::shared_future<DBusRecordState> futureOnResolve;
        std::unique_lock<std::mutex>* mutexOnResolve;

        std::unordered_map<std::string, DBusInterfaceNameListenersMap> dbusObjectPathListenersMap;
    };

    std::unordered_map<std::string, DBusServiceListenersRecord> dbusServiceListenersMap;


    struct DBusObjectPathCache {
        DBusObjectPathCache()
            : referenceCount(0),
              state(DBusRecordState::UNKNOWN),
              promiseOnResolve(std::make_shared<std::promise<DBusRecordState>>()),
              pendingObjectManagerCalls(0) {
        }

        ~DBusObjectPathCache() {
            if(state == DBusRecordState::RESOLVING && futureOnResolve.valid())
                promiseOnResolve->set_value(DBusRecordState::NOT_AVAILABLE);
        }

        size_t referenceCount;
        DBusRecordState state;
        std::shared_ptr<std::promise<DBusRecordState>> promiseOnResolve;
        std::shared_future<DBusRecordState> futureOnResolve;
        std::string serviceName;

        std::unordered_set<std::string> dbusInterfaceNamesCache;
        uint8_t pendingObjectManagerCalls;
    };

    struct DBusUniqueNameRecord {
        DBusUniqueNameRecord()
            : objectPathsState(DBusRecordState::UNKNOWN) {
        }

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
    std::recursive_mutex dbusServicesMutex_;

    void resolveDBusServiceName(const std::string& dbusServiceName,
                                DBusServiceListenersRecord& dbusServiceListenersRecord);

    void onGetNameOwnerCallback(const CallStatus& status,
                                std::string dbusServiceUniqueName,
                                const std::string& dbusServiceName);


    DBusRecordState resolveDBusInterfaceNameState(const DBusAddress &_address,
                                                  DBusServiceListenersRecord &_record);


    DBusObjectPathCache& getDBusObjectPathCacheReference(const std::string& dbusObjectPath,
                                                         const std::string& dbusServiceName,
                                                         const std::string& dbusServiceUniqueName,
                                                         DBusUniqueNameRecord& dbusUniqueNameRecord);

    bool resolveObjectPathWithObjectManager(DBusObjectPathCache& dbusObjectPathRecord,
                                            const std::string& dbusServiceUniqueName,
                                            const std::string& dbusObjectPath);

    typedef std::function<void(const CallStatus&,
                               const DBusObjectManagerStub::DBusObjectPathAndInterfacesDict,
                               const std::string&,
                               const std::string&)> GetManagedObjectsCallback;

    bool getManagedObjects(const std::string& dbusServiceName,
                           const std::string& dbusObjectPath,
                           DBusObjectManagerStub::DBusObjectPathAndInterfacesDict& availableServiceInstances);

    bool getManagedObjectsAsync(const std::string& dbusServiceName,
                                const std::string& dbusObjectPath,
                                GetManagedObjectsCallback callback);

    void onGetManagedObjectsCallbackResolve(const CallStatus& callStatus,
                                     const DBusObjectManagerStub::DBusObjectPathAndInterfacesDict availableServiceInstances,
                                     const std::string& dbusServiceUniqueName,
                                     const std::string& dbusObjectPath);

    void onDBusDaemonProxyNameOwnerChangedEvent(const std::string& name,
                                                const std::string& oldOwner,
                                                const std::string& newOwner);

    std::shared_ptr<DBusDaemonProxy> dbusDaemonProxy_;
    bool initialized_;

    ProxyStatusEvent::Subscription dbusDaemonProxyStatusEventSubscription_;
    NameOwnerChangedEvent::Subscription dbusDaemonProxyNameOwnerChangedEventSubscription_;


    void checkDBusServiceWasAvailable(const std::string& dbusServiceName, const std::string& dbusServiceUniqueName);

    void onDBusServiceAvailable(const std::string& dbusServiceName, const std::string& dbusServiceUniqueName);

    void onDBusServiceNotAvailable(DBusServiceListenersRecord& dbusServiceListenersRecord, const std::string &_serviceName = "");

    void notifyDBusServiceListenersLocked(const DBusUniqueNameRecord _dbusUniqueNameRecord,
                                    const std::string _dbusObjectPath,
                                    const std::unordered_set<std::string> _dbusInterfaceNames,
                                    const DBusRecordState _dbusInterfaceNamesState);


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


    void removeUniqueName(const DBusUniqueNamesMapIterator &_dbusUniqueName, const std::string &_serviceName);
    DBusUniqueNameRecord* insertServiceNameMapping(const std::string& dbusUniqueName, const std::string& dbusServiceName);
    bool findCachedDbusService(const std::string& dbusServiceName, DBusUniqueNameRecord** uniqueNameRecord);
    bool findCachedObjectPath(const std::string& dbusObjectPathName, const DBusUniqueNameRecord* uniqueNameRecord, DBusObjectPathCache* objectPathCache);

    void fetchAllServiceNames();

    inline bool isDBusServiceName(const std::string &_name) {
        return (_name.length() > 0 && _name[0] != ':');
    };


    inline bool isOrgFreedesktopDBusInterface(const std::string& dbusInterfaceName) {
        return dbusInterfaceName.find("org.freedesktop.DBus.") == 0;
    }

    std::thread::id notificationThread_;

    std::unordered_set<std::string> dbusPredefinedServices_;

private:
    typedef std::map<DBusProxyConnection*, std::shared_ptr<DBusServiceRegistry>> RegistryMap_t;
    static std::shared_ptr<RegistryMap_t> getRegistryMap() {
        static std::shared_ptr<RegistryMap_t> registries(new RegistryMap_t);
        return registries;
    }
    static std::mutex registriesMutex_;
    std::shared_ptr<RegistryMap_t> registries_;
    std::shared_ptr<DBusAddressTranslator> translator_;
    std::weak_ptr<DBusServiceRegistry> selfReference_;
};


} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUSSERVICEREGISTRY_HPP_
