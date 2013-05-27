/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
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


	DBusDaemonProxy(const std::shared_ptr<DBusProxyConnection>& dbusConnection);

	virtual bool isAvailable() const;
	virtual ProxyStatusEvent& getProxyStatusEvent();
	virtual InterfaceVersionAttribute& getInterfaceVersionAttribute();

	static inline const char* getInterfaceId();

    NameOwnerChangedEvent& getNameOwnerChangedEvent();

    void listNames(CommonAPI::CallStatus& callStatus, std::vector<std::string>& busNames) const;
    std::future<CallStatus> listNamesAsync(ListNamesAsyncCallback listNamesAsyncCallback) const;

    void nameHasOwner(const std::string& busName, CommonAPI::CallStatus& callStatus, bool& hasOwner) const;
    std::future<CallStatus> nameHasOwnerAsync(const std::string& busName, NameHasOwnerAsyncCallback nameHasOwnerAsyncCallback) const;

    std::future<CallStatus> getManagedObjectsAsync(const std::string& forDBusServiceName, GetManagedObjectsAsyncCallback) const;

    virtual std::string getAddress() const;
    virtual const std::string& getDomain() const;
    virtual const std::string& getServiceId() const;
    virtual const std::string& getInstanceId() const;

    virtual const std::string& getDBusBusName() const;
    virtual const std::string& getDBusObjectPath() const;
    virtual const std::string& getInterfaceName() const;

 private:
    DBusEvent<NameOwnerChangedEvent> nameOwnerChangedEvent_;
    static StaticInterfaceVersionAttribute interfaceVersionAttribute_;

    static const std::string dbusBusName_;
    static const std::string dbusObjectPath_;
    static const std::string commonApiParticipantId_;
    static const std::string dbusInterfaceName_;
};

const char* DBusDaemonProxy::getInterfaceId() {
    return "org.freedesktop.DBus";
}

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_DAEMON_PROXY_H_
