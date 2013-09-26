/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_CONNECTION_H_
#define COMMONAPI_DBUS_DBUS_CONNECTION_H_

#include "DBusProxyConnection.h"
#include "DBusDaemonProxy.h"
#include "DBusServiceRegistry.h"
#include "DBusObjectManager.h"
#include "DBusMainLoopContext.h"
#include "DBusConnectionBusType.h"

#include <dbus/dbus.h>

#include <atomic>


namespace CommonAPI {
namespace DBus {

class DBusObjectManager;

class DBusConnectionStatusEvent: public DBusProxyConnection::ConnectionStatusEvent {
    friend class DBusConnection;

 public:
    DBusConnectionStatusEvent(DBusConnection* dbusConnection);

 protected:
    virtual void onListenerAdded(const CancellableListener& listener);

    DBusConnection* dbusConnection_;
};

struct WatchContext {
    WatchContext(std::weak_ptr<MainLoopContext> mainLoopContext, DispatchSource* dispatchSource) :
            mainLoopContext_(mainLoopContext), dispatchSource_(dispatchSource) {
    }

    std::weak_ptr<MainLoopContext> mainLoopContext_;
    DispatchSource* dispatchSource_;
};

class DBusConnection: public DBusProxyConnection, public std::enable_shared_from_this<DBusConnection> {
 public:
    DBusConnection(BusType busType);

    inline static std::shared_ptr<DBusConnection> getBus(const BusType& dbusBusType);
    inline static std::shared_ptr<DBusConnection> wrapLibDBus(::DBusConnection* libDbusConnection);
    inline static std::shared_ptr<DBusConnection> getSessionBus();
    inline static std::shared_ptr<DBusConnection> getSystemBus();
    inline static std::shared_ptr<DBusConnection> getStarterBus();

    DBusConnection(const DBusConnection&) = delete;
    DBusConnection(::DBusConnection* libDbusConnection);

    DBusConnection& operator=(const DBusConnection&) = delete;
    virtual ~DBusConnection();

    BusType getBusType() const;

    bool connect(bool startDispatchThread = true);
    bool connect(DBusError& dbusError, bool startDispatchThread = true);
    void disconnect();

    virtual bool isConnected() const;

    virtual ConnectionStatusEvent& getConnectionStatusEvent();

    virtual bool requestServiceNameAndBlock(const std::string& serviceName) const;
    virtual bool releaseServiceName(const std::string& serviceName) const;

    bool sendDBusMessage(const DBusMessage& dbusMessage, uint32_t* allocatedSerial = NULL) const;

    static const int kDefaultSendTimeoutMs = 5000;

    std::future<CallStatus> sendDBusMessageWithReplyAsync(
            const DBusMessage& dbusMessage,
            std::unique_ptr<DBusMessageReplyAsyncHandler> dbusMessageReplyAsyncHandler,
            int timeoutMilliseconds = kDefaultSendTimeoutMs) const;

    DBusMessage sendDBusMessageWithReplyAndBlock(const DBusMessage& dbusMessage,
                                                 DBusError& dbusError,
                                                 int timeoutMilliseconds = kDefaultSendTimeoutMs) const;

    virtual bool addObjectManagerSignalMemberHandler(const std::string& dbusBusName,
                                                     DBusSignalHandler* dbusSignalHandler);
    virtual bool removeObjectManagerSignalMemberHandler(const std::string& dbusBusName,
                                                        DBusSignalHandler* dbusSignalHandler);

    DBusSignalHandlerToken addSignalMemberHandler(const std::string& objectPath,
                                                  const std::string& interfaceName,
                                                  const std::string& interfaceMemberName,
                                                  const std::string& interfaceMemberSignature,
                                                  DBusSignalHandler* dbusSignalHandler,
                                                  const bool justAddFilter = false);

    DBusProxyConnection::DBusSignalHandlerToken subscribeForSelectiveBroadcast(bool& subscriptionAccepted,
                                                                               const std::string& objectPath,
                                                                               const std::string& interfaceName,
                                                                               const std::string& interfaceMemberName,
                                                                               const std::string& interfaceMemberSignature,
                                                                               DBusSignalHandler* dbusSignalHandler,
                                                                               DBusProxy* callingProxy);

    void unsubsribeFromSelectiveBroadcast(const std::string& eventName,
                                          DBusProxyConnection::DBusSignalHandlerToken subscription,
                                          DBusProxy* callingProxy);

    void registerObjectPath(const std::string& objectPath);
    void unregisterObjectPath(const std::string& objectPath);

    bool removeSignalMemberHandler(const DBusSignalHandlerToken& dbusSignalHandlerToken);
    bool readWriteDispatch(int timeoutMilliseconds = -1);

    virtual const std::shared_ptr<DBusServiceRegistry> getDBusServiceRegistry();
    virtual const std::shared_ptr<DBusObjectManager> getDBusObjectManager();

    void setObjectPathMessageHandler(DBusObjectPathMessageHandler);
    bool isObjectPathMessageHandlerSet();

    virtual bool attachMainLoopContext(std::weak_ptr<MainLoopContext>);

    bool isDispatchReady();
    bool singleDispatch();

    typedef std::tuple<std::string, std::string, std::string> DBusSignalMatchRuleTuple;
    typedef std::pair<uint32_t, std::string> DBusSignalMatchRuleMapping;
    typedef std::unordered_map<DBusSignalMatchRuleTuple, DBusSignalMatchRuleMapping> DBusSignalMatchRulesMap;
 private:
    void dispatch();
    void suspendDispatching() const;
    void resumeDispatching() const;

    std::thread* dispatchThread_;
    bool stopDispatching_;

    std::weak_ptr<MainLoopContext> mainLoopContext_;
    DispatchSource* dispatchSource_;
    WatchContext* watchContext_;

    mutable bool pauseDispatching_;
    mutable std::mutex dispatchSuspendLock_;

    void addLibdbusSignalMatchRule(const std::string& objectPath,
            const std::string& interfaceName,
            const std::string& interfaceMemberName,
            const bool justAddFilter = false);

    void removeLibdbusSignalMatchRule(const std::string& objectPath,
            const std::string& interfaceName,
            const std::string& interfaceMemberName);

    void initLibdbusSignalFilterAfterConnect();
    ::DBusHandlerResult onLibdbusSignalFilter(::DBusMessage* libdbusMessage);

    void initLibdbusObjectPathHandlerAfterConnect();
    ::DBusHandlerResult onLibdbusObjectPathMessage(::DBusMessage* libdbusMessage);

    static void onLibdbusPendingCallNotifyThunk(::DBusPendingCall* libdbusPendingCall, void* userData);
    static void onLibdbusDataCleanup(void* userData);

    static ::DBusHandlerResult onLibdbusObjectPathMessageThunk(::DBusConnection* libdbusConnection,
            ::DBusMessage* libdbusMessage,
            void* userData);

    static ::DBusHandlerResult onLibdbusSignalFilterThunk(::DBusConnection* libdbusConnection,
            ::DBusMessage* libdbusMessage,
            void* userData);

    static dbus_bool_t onAddWatch(::DBusWatch* libdbusWatch, void* data);
    static void onRemoveWatch(::DBusWatch* libdbusWatch, void* data);
    static void onToggleWatch(::DBusWatch* libdbusWatch, void* data);

    static dbus_bool_t onAddTimeout(::DBusTimeout* dbus_timeout, void* data);
    static void onRemoveTimeout(::DBusTimeout* dbus_timeout, void* data);
    static void onToggleTimeout(::DBusTimeout* dbus_timeout, void* data);

    static void onWakeupMainContext(void* data);

    void enforceAsynchronousTimeouts() const;
    static const DBusObjectPathVTable* getDBusObjectPathVTable();

    ::DBusConnection* libdbusConnection_;
    mutable std::mutex libdbusConnectionGuard_;
    std::mutex signalGuard_;
    std::mutex objectManagerGuard_;
    std::mutex serviceRegistryGuard_;

    BusType busType_;

    std::weak_ptr<DBusServiceRegistry> dbusServiceRegistry_;
    std::shared_ptr<DBusObjectManager> dbusObjectManager_;

    DBusConnectionStatusEvent dbusConnectionStatusEvent_;

    DBusSignalMatchRulesMap dbusSignalMatchRulesMap_;

    DBusSignalHandlerTable dbusSignalHandlerTable_;

    std::unordered_map<std::string, size_t> dbusObjectManagerSignalMatchRulesMap_;
    std::unordered_multimap<std::string, DBusSignalHandler*> dbusObjectManagerSignalHandlerTable_;
    std::mutex dbusObjectManagerSignalGuard_;

    bool addObjectManagerSignalMatchRule(const std::string& dbusBusName);
    bool removeObjectManagerSignalMatchRule(const std::string& dbusBusName);

    bool addLibdbusSignalMatchRule(const std::string& dbusMatchRule);
    bool removeLibdbusSignalMatchRule(const std::string& dbusMatchRule);

    std::atomic_size_t libdbusSignalMatchRulesCount_;

    // objectPath, referenceCount
    typedef std::unordered_map<std::string, uint32_t> LibdbusRegisteredObjectPathHandlersTable;
    LibdbusRegisteredObjectPathHandlersTable libdbusRegisteredObjectPaths_;

    DBusObjectPathMessageHandler dbusObjectMessageHandler_;

    mutable std::unordered_map<std::string, uint16_t> connectionNameCount_;

    typedef std::pair<DBusPendingCall*, std::tuple<int, DBusMessageReplyAsyncHandler*, DBusMessage> > TimeoutMapElement;
    mutable std::map<DBusPendingCall*, std::tuple<int, DBusMessageReplyAsyncHandler*, DBusMessage>> timeoutMap_;
    mutable std::shared_ptr<std::thread> enforcerThread_;
    mutable std::mutex enforceTimeoutMutex_;
};

std::shared_ptr<DBusConnection> DBusConnection::getBus(const BusType& busType) {
    return std::make_shared<DBusConnection>(busType);
}

std::shared_ptr<DBusConnection> DBusConnection::wrapLibDBus(::DBusConnection* libDbusConnection) {
    return std::make_shared<DBusConnection>(libDbusConnection);
}

std::shared_ptr<DBusConnection> DBusConnection::getSessionBus() {
    return getBus(BusType::SESSION);
}

std::shared_ptr<DBusConnection> DBusConnection::getSystemBus() {
    return getBus(BusType::SYSTEM);
}

std::shared_ptr<DBusConnection> DBusConnection::getStarterBus() {
    return getBus(BusType::STARTER);
}

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_CONNECTION_H_
