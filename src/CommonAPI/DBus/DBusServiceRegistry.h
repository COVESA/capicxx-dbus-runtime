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

#include "DBusProxyConnection.h"
#include "DBusAddressTranslator.h"

#include <unordered_map>
#include <map>
#include <unordered_set>
#include <string>
#include <vector>
#include <memory>
#include <list>
#include <mutex>
#include <algorithm>
#include <set>

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
    static constexpr const char* getManagedObjectsDBusSignature_ = "a{oa{sa{sv}}}";

    DBusServiceRegistry();
    DBusServiceRegistry(const DBusServiceRegistry&) = delete;
    DBusServiceRegistry& operator=(const DBusServiceRegistry&) = delete;

    DBusServiceRegistry(std::shared_ptr<DBusProxyConnection> connection);
    virtual ~DBusServiceRegistry();

    virtual std::vector<std::string> getAvailableServiceInstances(const std::string& interfaceName,
                                                          const std::string& domainName = "local");

    virtual bool isServiceInstanceAlive(const std::string& dbusInterfaceName, const std::string& dbusConnectionName, const std::string& dbusObjectPath);

    virtual bool isReady() const;

    virtual bool isReadyBlocking() const;

    virtual void registerAvailabilityListener(const std::string& service, const std::function<void(bool)>& listener);

    virtual std::future<bool>& getReadyFuture();

    virtual DBusServiceStatusEvent& getServiceStatusEvent();

 private:
    void cacheAllServiceInstances();
    void cacheExistingBusNames();

    void removeProvidedServiceInstancesFromCache(const std::string& serviceBusName);
    void addProvidedServiceInstancesToCache(const std::string& dbusNames);
    void addAllProvidedServiceInstancesToCache(const std::vector<std::string>& serviceBusNames);

    void getManagedObjects(const std::string& serviceBusName, std::promise<bool>* returnPromise = 0);

    void onDBusNameOwnerChangedEvent(const std::string& name, const std::string& oldOwner, const std::string& newOwner);

    bool isRemoteServiceVersionMatchingLocalVersion(const std::string& serviceBusName, const std::string& serviceInterfaceName);

    void onManagedPaths(const CallStatus& status, DBusObjectToInterfaceDict replyMessage, std::string dbusWellKnownBusName, std::promise<bool>* returnPromise = 0);
    void onManagedPathsList(const CallStatus& status, DBusObjectToInterfaceDict managedObjects, std::list<std::string>::iterator iter, std::shared_ptr<std::list<std::string>> list);

    void onListNames(const CommonAPI::CallStatus&, std::vector<std::string>);
    void updateListeners(const std::string& conName, const std::string& objName, const std::string& intName , bool available);

    std::multimap<std::string, DBusInstanceId> dbusCachedProvidersForInterfaces_;
    std::set<std::string> dbusLivingServiceBusNames_;

    std::shared_ptr<DBusProxyConnection> dbusConnection_;

    std::unordered_multimap<std::string, std::function<void(bool)>> availabilityCallbackList;

    NameOwnerChangedEvent::Subscription dbusNameOwnerChangedEventSubscription_;

    mutable bool ready;
    mutable std::future<bool> readyFuture_;
    mutable std::promise<bool> readyPromise_;

    mutable std::mutex readyMutex_;

    DBusServiceStatusEvent serviceStatusEvent_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_SERVICE_REGISTRY_H_
