/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_DBUS_DBUS_SERVICE_REGISTRY_H_
#define COMMONAPI_DBUS_DBUS_SERVICE_REGISTRY_H_

#include <CommonAPI/types.h>
#include <CommonAPI/Attribute.h>
#include <CommonAPI/Proxy.h>

#include "DBusProxyConnection.h"
#include "DBusAddressTranslator.h"

#include <unordered_map>
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

namespace CommonAPI {
namespace DBus {

typedef Event<std::string, std::string, std::string> NameOwnerChangedEvent;
typedef Event<std::string, std::string, std::string>::Subscription NameOwnerChangedEventSubscription;

//connectionName, objectPath
typedef std::pair<std::string, std::string> DBusInstanceId;

typedef std::unordered_map<std::string, int> PropertyDictStub;
typedef std::unordered_map<std::string, PropertyDictStub> InterfaceToPropertyDict;
typedef std::unordered_map<std::string, InterfaceToPropertyDict> DBusObjectToInterfaceDict;

class DBusProxyConnection;
class DBusDaemonProxy;


class DBusServiceRegistry {
 public:
    enum class DBusServiceState {
        UNKNOWN,
        AVAILABLE,
        RESOLVING,
        RESOLVED,
        NOT_AVAILABLE
    };

    typedef std::function<void(const AvailabilityStatus& availabilityStatus)> DBusServiceListener;
    typedef std::list<DBusServiceListener> DBusServiceListenerList;
    typedef DBusServiceListenerList::iterator Subscription;

    typedef std::pair<std::string, std::string> DBusObjectInterfacePair;
    typedef std::unordered_map<DBusObjectInterfacePair, std::pair<AvailabilityStatus, DBusServiceListenerList> > DBusInstanceList;
    typedef std::unordered_map<std::string, std::pair<DBusServiceState, DBusInstanceList> > DBusServiceList;


    DBusServiceRegistry();

    DBusServiceRegistry(std::shared_ptr<DBusProxyConnection> connection);

    virtual ~DBusServiceRegistry();

    bool isServiceInstanceAlive(const std::string& dbusInterfaceName, const std::string& dbusConnectionName, const std::string& dbusObjectPath);

    Subscription subscribeAvailabilityListener(const std::string& commonApiAddress,
                                               DBusServiceListener serviceListener);
    void unsubscribeAvailabilityListener(const std::string& commonApiAddress,
                                         Subscription& listenerSubscription);

    virtual std::vector<std::string> getAvailableServiceInstances(const std::string& interfaceName,
                                                          const std::string& domainName = "local");

    virtual DBusServiceStatusEvent& getServiceStatusEvent();

 private:
    DBusServiceRegistry(const DBusServiceRegistry&) = delete;
    DBusServiceRegistry& operator=(const DBusServiceRegistry&) = delete;

    SubscriptionStatus onDBusDaemonProxyStatusEvent(const AvailabilityStatus& availabilityStatus);
    SubscriptionStatus onDBusDaemonProxyNameOwnerChangedEvent(const std::string& name, const std::string& oldOwner, const std::string& newOwner);

    void onListNamesCallback(const CommonAPI::CallStatus& callStatus, std::vector<std::string> dbusNames);

    void resolveDBusServiceInstances(DBusServiceList::iterator& dbusServiceIterator);
    void onGetManagedObjectsCallback(const CallStatus& status, DBusObjectToInterfaceDict managedObjects, const std::string& dbusServiceName);

    size_t getAvailableServiceInstances(const std::string& dbusInterfaceName, std::vector<std::string>& availableServiceInstances);

    bool waitDBusServicesAvailable(std::unique_lock<std::mutex>& lock, std::chrono::milliseconds& timeout);

    void onDBusServiceAvailabilityStatus(const std::string& dbusServiceName, const AvailabilityStatus& availabilityStatus);
    DBusServiceList::iterator onDBusServiceAvailabilityStatus(DBusServiceList::iterator& dbusServiceIterator, const AvailabilityStatus& availabilityStatus);
    DBusServiceList::iterator onDBusServiceOffline(DBusServiceList::iterator& dbusServiceIterator, const DBusServiceState& dbusServiceState);

    static DBusInstanceList::iterator addDBusServiceInstance(
                    DBusInstanceList& dbusInstanceList,
                    const std::string& dbusObjectPath,
                    const std::string& dbusInterfaceName,
                    const AvailabilityStatus& dbusInstanceAvailabilityStatus);

    static void notifyDBusServiceListeners(DBusServiceListenerList& dbusServiceListenerList, const AvailabilityStatus& availabilityStatus);

    static bool isDBusServiceName(const std::string& name);


    std::shared_ptr<DBusProxyConnection> dbusConnection_;

    DBusServiceList dbusServices_;
    AvailabilityStatus dbusServicesStatus_;
    std::condition_variable dbusServiceChanged_;

    std::mutex dbusServicesMutex_;

    ProxyStatusEvent::Subscription dbusDaemonProxyStatusEventSubscription_;
    NameOwnerChangedEvent::Subscription dbusDaemonProxyNameOwnerChangedEventSubscription_;

    DBusServiceStatusEvent serviceStatusEvent_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_SERVICE_REGISTRY_H_
