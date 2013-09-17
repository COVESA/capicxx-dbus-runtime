/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_DAEMON_PROXY_H_
#define COMMONAPI_DBUS_DBUS_DAEMON_PROXY_H_

#include "DBusProxyBase.h"
#include "DBusEvent.h"

#include <functional>
#include <string>
#include <vector>


namespace CommonAPI {
namespace DBus {

class StaticInterfaceVersionAttribute: public InterfaceVersionAttribute {
 public:
    StaticInterfaceVersionAttribute(const uint32_t& majorValue, const uint32_t& minorValue);

    void getValue(CallStatus& callStatus, Version& version) const;
    std::future<CallStatus> getValueAsync(AttributeAsyncCallback attributeAsyncCallback);

 private:
    Version version_;
};


class DBusDaemonProxy: public DBusProxyBase {
 public:
    typedef Event<std::string, std::string, std::string> NameOwnerChangedEvent;

    typedef std::unordered_map<std::string, int> PropertyDictStub;
    typedef std::unordered_map<std::string, PropertyDictStub> InterfaceToPropertyDict;
    typedef std::unordered_map<std::string, InterfaceToPropertyDict> DBusObjectToInterfaceDict;

    typedef std::function<void(const CommonAPI::CallStatus&, std::vector<std::string>)> ListNamesAsyncCallback;
    typedef std::function<void(const CommonAPI::CallStatus&, bool)> NameHasOwnerAsyncCallback;
    typedef std::function<void(const CommonAPI::CallStatus&, DBusObjectToInterfaceDict)> GetManagedObjectsAsyncCallback;
    typedef std::function<void(const CommonAPI::CallStatus&, std::string)> GetNameOwnerAsyncCallback;

    DBusDaemonProxy(const std::shared_ptr<DBusProxyConnection>& dbusConnection);

    virtual bool isAvailable() const;
    virtual bool isAvailableBlocking() const;
    virtual ProxyStatusEvent& getProxyStatusEvent();
    virtual InterfaceVersionAttribute& getInterfaceVersionAttribute();

    void init();

    static inline const char* getInterfaceId();

    NameOwnerChangedEvent& getNameOwnerChangedEvent();

    void listNames(CommonAPI::CallStatus& callStatus, std::vector<std::string>& busNames) const;
    std::future<CallStatus> listNamesAsync(ListNamesAsyncCallback listNamesAsyncCallback) const;

    void nameHasOwner(const std::string& busName, CommonAPI::CallStatus& callStatus, bool& hasOwner) const;
    std::future<CallStatus> nameHasOwnerAsync(const std::string& busName,
                                              NameHasOwnerAsyncCallback nameHasOwnerAsyncCallback) const;

    std::future<CallStatus> getManagedObjectsAsync(const std::string& forDBusServiceName,
                                                   GetManagedObjectsAsyncCallback) const;

    /**
     * Get the unique connection/bus name of the primary owner of the name given
     *
     * @param busName Name to get the owner of
     * @param getNameOwnerAsyncCallback callback functor
     *
     * @return CallStatus::REMOTE_ERROR if the name is unknown, otherwise CallStatus::SUCCESS and the uniq name of the owner
     */
    std::future<CallStatus> getNameOwnerAsync(const std::string& busName, GetNameOwnerAsyncCallback getNameOwnerAsyncCallback) const;

    virtual std::string getAddress() const;
    virtual const std::string& getDomain() const;
    virtual const std::string& getServiceId() const;
    virtual const std::string& getInstanceId() const;

    virtual const std::string& getDBusBusName() const;
    virtual const std::string& getDBusObjectPath() const;
    virtual const std::string& getInterfaceName() const;

 private:
    DBusEvent<NameOwnerChangedEvent> nameOwnerChangedEvent_;
    StaticInterfaceVersionAttribute interfaceVersionAttribute_;
};

const char* DBusDaemonProxy::getInterfaceId() {
    static const char interfaceId[] = "org.freedesktop.DBus";
    return interfaceId;
}

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_DAEMON_PROXY_H_
