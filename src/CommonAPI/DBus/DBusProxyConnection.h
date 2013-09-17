/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_PROXY_CONNECTION_H_
#define COMMONAPI_DBUS_DBUS_PROXY_CONNECTION_H_

#include "DBusError.h"
#include "DBusMessage.h"

#include "DBusFunctionalHash.h"

#include <CommonAPI/types.h>
#include <CommonAPI/Attribute.h>
#include <CommonAPI/Event.h>

#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CommonAPI {
namespace DBus {


typedef std::function<void(const DBusMessage&)> DBusMessageHandler;

class DBusDaemonProxy;
class DBusServiceRegistry;
class DBusObjectManager;
class DBusProxy;


class DBusProxyConnection {
 public:
    class DBusMessageReplyAsyncHandler {
     public:
       virtual ~DBusMessageReplyAsyncHandler() { }
       virtual std::future<CallStatus> getFuture() = 0;
       virtual void onDBusMessageReply(const CallStatus&, const DBusMessage&) = 0;
    };

    class DBusSignalHandler {
     public:
        virtual ~DBusSignalHandler() { }
        virtual SubscriptionStatus onSignalDBusMessage(const DBusMessage&) = 0;
    };

    // objectPath, interfaceName, interfaceMemberName, interfaceMemberSignature
    typedef std::tuple<std::string, std::string, std::string, std::string> DBusSignalHandlerPath;
    typedef std::unordered_multimap<DBusSignalHandlerPath, DBusSignalHandler*> DBusSignalHandlerTable;
    typedef DBusSignalHandlerPath DBusSignalHandlerToken;

    typedef Event<AvailabilityStatus> ConnectionStatusEvent;

    virtual ~DBusProxyConnection() {
    }

    virtual bool isConnected() const = 0;

    virtual ConnectionStatusEvent& getConnectionStatusEvent() = 0;

    virtual bool sendDBusMessage(const DBusMessage& dbusMessage, uint32_t* allocatedSerial = NULL) const = 0;

    static const int kDefaultSendTimeoutMs = 100 * 1000;

    virtual std::future<CallStatus> sendDBusMessageWithReplyAsync(
            const DBusMessage& dbusMessage,
            std::unique_ptr<DBusMessageReplyAsyncHandler> dbusMessageReplyAsyncHandler,
            int timeoutMilliseconds = kDefaultSendTimeoutMs) const = 0;

    virtual DBusMessage sendDBusMessageWithReplyAndBlock(
            const DBusMessage& dbusMessage,
            DBusError& dbusError,
            int timeoutMilliseconds = kDefaultSendTimeoutMs) const = 0;

    virtual DBusSignalHandlerToken addSignalMemberHandler(
            const std::string& objectPath,
            const std::string& interfaceName,
            const std::string& interfaceMemberName,
            const std::string& interfaceMemberSignature,
            DBusSignalHandler* dbusSignalHandler,
            const bool justAddFilter = false) = 0;

    virtual DBusSignalHandlerToken subscribeForSelectiveBroadcast(bool& subscriptionAccepted,
                                                                  const std::string& objectPath,
                                                                  const std::string& interfaceName,
                                                                  const std::string& interfaceMemberName,
                                                                  const std::string& interfaceMemberSignature,
                                                                  DBusSignalHandler* dbusSignalHandler,
                                                                  DBusProxy* callingProxy) = 0;

    virtual void unsubsribeFromSelectiveBroadcast(const std::string& eventName,
                                                  DBusProxyConnection::DBusSignalHandlerToken subscription,
                                                  DBusProxy* callingProxy) = 0;
    virtual bool removeSignalMemberHandler(const DBusSignalHandlerToken& dbusSignalHandlerToken) = 0;

    virtual bool addObjectManagerSignalMemberHandler(const std::string& dbusBusName,
                                                     DBusSignalHandler* dbusSignalHandler) = 0;
    virtual bool removeObjectManagerSignalMemberHandler(const std::string& dbusBusName,
                                                        DBusSignalHandler* dbusSignalHandler) = 0;

    virtual const std::shared_ptr<DBusServiceRegistry> getDBusServiceRegistry() = 0;
    virtual const std::shared_ptr<DBusObjectManager> getDBusObjectManager() = 0;

    virtual void registerObjectPath(const std::string& objectPath) = 0;
    virtual void unregisterObjectPath(const std::string& objectPath) = 0;

    virtual bool requestServiceNameAndBlock(const std::string& serviceName) const = 0;
    virtual bool releaseServiceName(const std::string& serviceName) const = 0;

    typedef std::function<bool(const DBusMessage&)> DBusObjectPathMessageHandler;

    virtual void setObjectPathMessageHandler(DBusObjectPathMessageHandler) = 0;
    virtual bool isObjectPathMessageHandlerSet() = 0;
};


} // namespace DBus
} // namespace CommonAPI

#endif //COMMONAPI_DBUS_DBUS_PROXY_CONNECTION_H_
