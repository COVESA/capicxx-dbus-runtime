/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_DBUS_DBUS_PROXY_H_
#define COMMONAPI_DBUS_DBUS_PROXY_H_

#include "DBusProxyConnection.h"
#include "DBusAttribute.h"

#include <CommonAPI/Proxy.h>
#include <CommonAPI/types.h>

#include <functional>
#include <memory>
#include <string>

namespace CommonAPI {
namespace DBus {

class DBusProxy;

typedef Event<AvailabilityStatus> ProxyStatusEvent;

class DBusProxyStatusEvent: public ProxyStatusEvent {
 public:
    DBusProxyStatusEvent(DBusProxy* dbusProxy);

    void onFirstListenerAdded(const Listener& listener);
    void onLastListenerRemoved(const Listener& listener);

    Subscription subscribe(Listener listener);

 private:
    SubscriptionStatus onServiceAvailableSignalHandler(const std::string& name, const AvailabilityStatus& availabilityStatus);

    DBusProxy* dbusProxy_;
    DBusServiceStatusEvent::Subscription subscription_;

    friend class DBusProxy;

};


class DBusProxy: public virtual CommonAPI::Proxy {
 public:
    DBusProxy(const std::string& dbusBusName,
              const std::string& dbusObjectPath,
              const std::string& interfaceName,
              const std::shared_ptr<DBusProxyConnection>& dbusProxyConnection);

    virtual std::string getAddress() const;
    virtual const std::string& getDomain() const;
    virtual const std::string& getServiceId() const;
    virtual const std::string& getInstanceId() const;
    virtual bool isAvailable() const;
    virtual bool isAvailableBlocking() const;
    virtual ProxyStatusEvent& getProxyStatusEvent();
    virtual InterfaceVersionAttribute& getInterfaceVersionAttribute();

    inline const std::string& getDBusBusName() const;
    inline const std::string& getDBusObjectPath() const;
    inline const std::string& getInterfaceName() const;
    inline const std::shared_ptr<DBusProxyConnection>& getDBusConnection() const;

    DBusMessage createMethodCall(const char* methodName,
                                 const char* methodSignature = NULL) const;

    inline DBusProxyConnection::DBusSignalHandlerToken addSignalMemberHandler(
    		const std::string& signalName,
    		const std::string& signalSignature,
    		DBusProxyConnection::DBusSignalHandler* dbusSignalHandler);

    inline void removeSignalMemberHandler(const DBusProxyConnection::DBusSignalHandlerToken& dbusSignalHandlerToken);

 protected:
    DBusProxy(const DBusProxy& abstractProxy) = delete;

    DBusProxy(const std::string& busName,
              const std::string& objectId,
              const std::string& interfaceName,
              const std::shared_ptr<DBusProxyConnection>& connection,
              const bool isAlwaysAvailable);

    virtual void getOwnVersion(uint16_t& ownVersionMajor, uint16_t& ownVersionMinor) const = 0;

    DBusProxyStatusEvent statusEvent_;
    DBusProxyStatusEvent::Subscription remoteStatusSubscription_;

 private:
    void onServiceAlive(bool alive);

    const std::string dbusBusName_;
    const std::string dbusObjectPath_;
    const std::string interfaceName_;

    mutable bool available_;
    mutable bool availableSet_;

    std::shared_ptr<DBusProxyConnection> connection_;

    DBusReadonlyAttribute<InterfaceVersionAttribute> interfaceVersionAttribute_;

    static const std::string domain_;

    friend class DBusProxyStatusEvent;
};

const std::string& DBusProxy::getDBusBusName() const {
    return dbusBusName_;
}

const std::string& DBusProxy::getDBusObjectPath() const {
    return dbusObjectPath_;
}

const std::string& DBusProxy::getInterfaceName() const {
    return interfaceName_;
}

const std::shared_ptr<DBusProxyConnection>& DBusProxy::getDBusConnection() const {
    return connection_;
}

DBusProxyConnection::DBusSignalHandlerToken DBusProxy::addSignalMemberHandler(
        const std::string& signalName,
        const std::string& signalSignature,
        DBusProxyConnection::DBusSignalHandler* dbusSignalHandler) {
    return connection_->addSignalMemberHandler(dbusObjectPath_, getInterfaceName(), signalName, signalSignature, dbusSignalHandler);
}

void DBusProxy::removeSignalMemberHandler(const DBusProxyConnection::DBusSignalHandlerToken& dbusSignalHandlerToken) {
    return connection_->removeSignalMemberHandler(dbusSignalHandlerToken);
}

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_PROXY_H_

