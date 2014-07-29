/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Workaround for libstdc++ bug
#ifndef _GLIBCXX_USE_NANOSLEEP
#define _GLIBCXX_USE_NANOSLEEP
#endif

#include "DBusConnection.h"
#include "DBusInputStream.h"
#include "DBusProxy.h"

#include <algorithm>
#include <sstream>
#include <cassert>
#include <future>
#include <chrono>
#include <thread>

namespace CommonAPI {
namespace DBus {

DBusConnectionStatusEvent::DBusConnectionStatusEvent(DBusConnection* dbusConnection):
                dbusConnection_(dbusConnection) {
}

void DBusConnectionStatusEvent::onListenerAdded(const CancellableListener& listener) {
    if (dbusConnection_->isConnected())
        listener(AvailabilityStatus::AVAILABLE);
}


const DBusObjectPathVTable* DBusConnection::getDBusObjectPathVTable() {
    static const DBusObjectPathVTable libdbusObjectPathVTable = {
                    NULL, // no need to handle unregister callbacks
                    &DBusConnection::onLibdbusObjectPathMessageThunk
    };
    return &libdbusObjectPathVTable;
}



//std::bind used to start the dispatch thread holds one reference, and the selfReference
//created within the thread is the second. If only those two remain, no one but the
//dispatch thread references the connection, which therefore can be finished.
const uint32_t ownUseCount = 2;

void DBusConnection::dispatch() {
    std::shared_ptr<DBusConnection> selfReference = this->shared_from_this();
    while (!stopDispatching_ && readWriteDispatch(10) && selfReference.use_count() > ownUseCount) {
        if (pauseDispatching_) {
            dispatchSuspendLock_.lock();
            dispatchSuspendLock_.unlock();
        }
    }
}

bool DBusConnection::readWriteDispatch(int timeoutMilliseconds) {
    if(isConnected()) {
        const dbus_bool_t libdbusSuccess = dbus_connection_read_write_dispatch(libdbusConnection_,
                                                                               timeoutMilliseconds);
        return libdbusSuccess;
    }
    return false;
}

void DBusConnection::suspendDispatching() const {
    dispatchSuspendLock_.lock();
    pauseDispatching_ = true;
}

void DBusConnection::resumeDispatching() const {
    pauseDispatching_ = false;
    dispatchSuspendLock_.unlock();
}

DBusConnection::DBusConnection(BusType busType) :
                dispatchThread_(NULL),
                stopDispatching_(false),
                mainLoopContext_(std::shared_ptr<MainLoopContext>(NULL)),
                dispatchSource_(),
                watchContext_(NULL),
                pauseDispatching_(false),
                libdbusConnection_(NULL),
                busType_(busType),
                dbusConnectionStatusEvent_(this),
                libdbusSignalMatchRulesCount_(0),
                dbusObjectMessageHandler_(),
                connectionNameCount_(),
                enforcerThread_(NULL)
                 {

    dbus_threads_init_default();
}

DBusConnection::DBusConnection(::DBusConnection* libDbusConnection) :
                dispatchThread_(NULL),
                stopDispatching_(false),
                mainLoopContext_(std::shared_ptr<MainLoopContext>(NULL)),
                dispatchSource_(),
                watchContext_(NULL),
                pauseDispatching_(false),
                libdbusConnection_(libDbusConnection),
                busType_(WRAPPED),
                dbusConnectionStatusEvent_(this),
                libdbusSignalMatchRulesCount_(0),
                dbusObjectMessageHandler_(),
                connectionNameCount_(),
                enforcerThread_(NULL) {
    dbus_threads_init_default();
}

void DBusConnection::setObjectPathMessageHandler(DBusObjectPathMessageHandler handler) {
    dbusObjectMessageHandler_ = handler;
}

bool DBusConnection::isObjectPathMessageHandlerSet() {
    return dbusObjectMessageHandler_.operator bool();
}

DBusConnection::~DBusConnection() {
    if (auto lockedContext = mainLoopContext_.lock()) {
        dbus_connection_set_watch_functions(libdbusConnection_, NULL, NULL, NULL, NULL, NULL);
        dbus_connection_set_timeout_functions(libdbusConnection_, NULL, NULL, NULL, NULL, NULL);

        lockedContext->deregisterDispatchSource(dispatchSource_);
        delete watchContext_;
        delete dispatchSource_;
    }

    disconnect();

    //Assert that the enforcerThread_ is in a position to finish itself correctly even after destruction
    //of the DBusConnection. Also assert all resources are cleaned up.
    if (enforcerThread_) {
        enforceTimeoutMutex_.lock();

        auto it = timeoutMap_.begin();
        while (it != timeoutMap_.end()) {
            DBusPendingCall* libdbusPendingCall = it->first;

            if (!dbus_pending_call_get_completed(libdbusPendingCall)) {
                dbus_pending_call_cancel(libdbusPendingCall);
                DBusMessageReplyAsyncHandler* asyncHandler = std::get<1>(it->second);
                DBusMessage& dbusMessageCall = std::get<2>(it->second);
                asyncHandler->onDBusMessageReply(CallStatus::REMOTE_ERROR, dbusMessageCall.createMethodError(DBUS_ERROR_TIMEOUT));
                delete asyncHandler;
            }
            it = timeoutMap_.erase(it);
            dbus_pending_call_unref(libdbusPendingCall);
        }

        enforceTimeoutMutex_.unlock();
    }
}


bool DBusConnection::attachMainLoopContext(std::weak_ptr<MainLoopContext> mainLoopContext) {
    mainLoopContext_ = mainLoopContext;

    if (auto lockedContext = mainLoopContext_.lock()) {
        dispatchSource_ = new DBusDispatchSource(this);
        watchContext_ = new WatchContext(mainLoopContext_, dispatchSource_);
        lockedContext->registerDispatchSource(dispatchSource_);

        dbus_connection_set_wakeup_main_function(
                        libdbusConnection_,
                        &DBusConnection::onWakeupMainContext,
                        &mainLoopContext_,
                        NULL);

        bool success = dbus_connection_set_watch_functions(
                libdbusConnection_,
                &DBusConnection::onAddWatch,
                &DBusConnection::onRemoveWatch,
                &DBusConnection::onToggleWatch,
                watchContext_,
                NULL);

        if (!success) {
            return false;
        }

        success = dbus_connection_set_timeout_functions(
                libdbusConnection_,
                &DBusConnection::onAddTimeout,
                &DBusConnection::onRemoveTimeout,
                &DBusConnection::onToggleTimeout,
                &mainLoopContext_,
                NULL);

        if (!success) {
            dbus_connection_set_watch_functions(libdbusConnection_, NULL, NULL, NULL, NULL, NULL);
            return false;
        }

        return true;
    }
    return false;
}

void DBusConnection::onWakeupMainContext(void* data) {
    std::weak_ptr<MainLoopContext>* mainloop = static_cast<std::weak_ptr<MainLoopContext>*>(data);
    assert(mainloop);

    if(auto lockedContext = mainloop->lock()) {
        lockedContext->wakeup();
    }
}


dbus_bool_t DBusConnection::onAddWatch(::DBusWatch* libdbusWatch, void* data) {
    WatchContext* watchContext = static_cast<WatchContext*>(data);
    assert(watchContext);

    DBusWatch* dbusWatch = new DBusWatch(libdbusWatch, watchContext->mainLoopContext_);
    dbusWatch->addDependentDispatchSource(watchContext->dispatchSource_);
    dbus_watch_set_data(libdbusWatch, dbusWatch, NULL);

    if (dbusWatch->isReadyToBeWatched()) {
        dbusWatch->startWatching();
    }

    return TRUE;
}

void DBusConnection::onRemoveWatch(::DBusWatch* libdbusWatch, void* data) {
    assert(static_cast<WatchContext*>(data));

    DBusWatch* dbusWatch = static_cast<DBusWatch*>(dbus_watch_get_data(libdbusWatch));
    if(dbusWatch->isReadyToBeWatched()) {
        dbusWatch->stopWatching();
    }
    dbus_watch_set_data(libdbusWatch, NULL, NULL);
    delete dbusWatch;
}

void DBusConnection::onToggleWatch(::DBusWatch* libdbusWatch, void* data) {
    assert(static_cast<WatchContext*>(data));

    DBusWatch* dbusWatch = static_cast<DBusWatch*>(dbus_watch_get_data(libdbusWatch));

    if (dbusWatch->isReadyToBeWatched()) {
        dbusWatch->startWatching();
    } else {
        dbusWatch->stopWatching();
    }
}


dbus_bool_t DBusConnection::onAddTimeout(::DBusTimeout* libdbusTimeout, void* data) {
    std::weak_ptr<MainLoopContext>* mainloop = static_cast<std::weak_ptr<MainLoopContext>*>(data);
    assert(mainloop);

    DBusTimeout* dbusTimeout = new DBusTimeout(libdbusTimeout, *mainloop);
    dbus_timeout_set_data(libdbusTimeout, dbusTimeout, NULL);

    if (dbusTimeout->isReadyToBeMonitored()) {
        dbusTimeout->startMonitoring();
    }

    return TRUE;
}

void DBusConnection::onRemoveTimeout(::DBusTimeout* libdbusTimeout, void* data) {
    assert(static_cast<std::weak_ptr<MainLoopContext>*>(data));

    DBusTimeout* dbusTimeout = static_cast<DBusTimeout*>(dbus_timeout_get_data(libdbusTimeout));
    dbusTimeout->stopMonitoring();
    dbus_timeout_set_data(libdbusTimeout, NULL, NULL);
    delete dbusTimeout;
}

void DBusConnection::onToggleTimeout(::DBusTimeout* dbustimeout, void* data) {
    assert(static_cast<std::weak_ptr<MainLoopContext>*>(data));

    DBusTimeout* timeout = static_cast<DBusTimeout*>(dbus_timeout_get_data(dbustimeout));

    if (timeout->isReadyToBeMonitored()) {
        timeout->startMonitoring();
    } else {
        timeout->stopMonitoring();
    }
}


bool DBusConnection::connect(bool startDispatchThread) {
    DBusError dbusError;
    return connect(dbusError, startDispatchThread);
}

bool DBusConnection::connect(DBusError& dbusError, bool startDispatchThread) {
    assert(!dbusError);

    if (isConnected()) {
        return true;
    }

    const ::DBusBusType libdbusType = static_cast<DBusBusType>(busType_);

    libdbusConnection_ = dbus_bus_get_private(libdbusType, &dbusError.libdbusError_);
    if (dbusError) {
        return false;
    }

    assert(libdbusConnection_);
    dbus_connection_set_exit_on_disconnect(libdbusConnection_, false);

    initLibdbusObjectPathHandlerAfterConnect();

    initLibdbusSignalFilterAfterConnect();

    if (startDispatchThread) {
        dispatchThread_ = new std::thread(std::bind(&DBusConnection::dispatch, this->shared_from_this()));
    }
    stopDispatching_ = !startDispatchThread;

    dbusConnectionStatusEvent_.notifyListeners(AvailabilityStatus::AVAILABLE);

    return true;
}

void DBusConnection::disconnect() {
    std::lock_guard<std::mutex> dbusConnectionLock(libdbusConnectionGuard_);
    if (isConnected()) {
        dbusConnectionStatusEvent_.notifyListeners(AvailabilityStatus::NOT_AVAILABLE);

        if (libdbusSignalMatchRulesCount_ > 0) {
            dbus_connection_remove_filter(libdbusConnection_, &onLibdbusSignalFilterThunk, this);
            libdbusSignalMatchRulesCount_ = 0;
        }

        connectionNameCount_.clear();

        stopDispatching_ = true;

        dbus_connection_close(libdbusConnection_);

        if(dispatchThread_) {
            //It is possible for the disconnect to be called from within a callback, i.e. from within the dispatch
            //thread. Self-join is prevented this way.
            if (dispatchThread_->joinable() && std::this_thread::get_id() != dispatchThread_->get_id()) {
                dispatchThread_->join();
            } else {
                dispatchThread_->detach();
            }
            delete dispatchThread_;
            dispatchThread_ = NULL;
        }

        dbus_connection_unref(libdbusConnection_);
        libdbusConnection_ = NULL;
    }
}

bool DBusConnection::isConnected() const {
    return (libdbusConnection_ != NULL);
}

DBusProxyConnection::ConnectionStatusEvent& DBusConnection::getConnectionStatusEvent() {
    return dbusConnectionStatusEvent_;
}

const std::shared_ptr<DBusServiceRegistry> DBusConnection::getDBusServiceRegistry() {
    std::shared_ptr<DBusServiceRegistry> serviceRegistry = dbusServiceRegistry_.lock();
    if (!serviceRegistry || dbusServiceRegistry_.expired()) {
        serviceRegistryGuard_.lock();
        if (!serviceRegistry || dbusServiceRegistry_.expired()) {
            serviceRegistry = std::make_shared<DBusServiceRegistry>(shared_from_this());
            serviceRegistry->init();
            dbusServiceRegistry_ = serviceRegistry;
        }
        serviceRegistryGuard_.unlock();
    }

    return serviceRegistry;
}

//Does this need to be a weak pointer?
const std::shared_ptr<DBusObjectManager> DBusConnection::getDBusObjectManager() {
    if (!dbusObjectManager_) {
        objectManagerGuard_.lock();
        if (!dbusObjectManager_) {
            dbusObjectManager_ = std::make_shared<DBusObjectManager>(shared_from_this());
        }
        objectManagerGuard_.unlock();
    }

    return dbusObjectManager_;
}

bool DBusConnection::requestServiceNameAndBlock(const std::string& serviceName) const {
    DBusError dbusError;
    bool isServiceNameAcquired = false;
    std::lock_guard<std::mutex> dbusConnectionLock(libdbusConnectionGuard_);
    auto conIter = connectionNameCount_.find(serviceName);
    if (conIter == connectionNameCount_.end()) {
        suspendDispatching();

        const int libdbusStatus = dbus_bus_request_name(libdbusConnection_,
                        serviceName.c_str(),
                        DBUS_NAME_FLAG_DO_NOT_QUEUE,
                        &dbusError.libdbusError_);

        resumeDispatching();

        isServiceNameAcquired = (libdbusStatus == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);
        if (isServiceNameAcquired) {
            connectionNameCount_.insert({serviceName, (uint16_t)1});
        }
    } else {
        conIter->second = conIter->second + 1;
        isServiceNameAcquired = true;
    }

    return isServiceNameAcquired;
}

bool DBusConnection::releaseServiceName(const std::string& serviceName) const {
    DBusError dbusError;
    bool isServiceNameReleased = false;
    std::lock_guard<std::mutex> dbusConnectionLock(libdbusConnectionGuard_);
    auto conIter = connectionNameCount_.find(serviceName);
    if (conIter != connectionNameCount_.end()) {
        if (conIter->second == 1) {
            suspendDispatching();
            const int libdbusStatus = dbus_bus_release_name(libdbusConnection_,
                            serviceName.c_str(),
                            &dbusError.libdbusError_);
            resumeDispatching();
            isServiceNameReleased = (libdbusStatus == DBUS_RELEASE_NAME_REPLY_RELEASED);
            if (isServiceNameReleased) {
                connectionNameCount_.erase(conIter);
            }
        } else {
            conIter->second = conIter->second - 1;
            isServiceNameReleased = true;
        }
    }
    return isServiceNameReleased;
}

bool DBusConnection::sendDBusMessage(const DBusMessage& dbusMessage, uint32_t* allocatedSerial) const {
    assert(dbusMessage);
    assert(isConnected());

    dbus_uint32_t* libdbusSerial = static_cast<dbus_uint32_t*>(allocatedSerial);
    const bool result = dbus_connection_send(libdbusConnection_, dbusMessage.libdbusMessage_, libdbusSerial);

    return result;
}

void DBusConnection::onLibdbusPendingCallNotifyThunk(::DBusPendingCall* libdbusPendingCall, void *userData) {
    assert(userData);
    assert(libdbusPendingCall);

    auto dbusMessageReplyAsyncHandler = reinterpret_cast<DBusMessageReplyAsyncHandler*>(userData);

    ::DBusMessage* libdbusMessage = dbus_pending_call_steal_reply(libdbusPendingCall);
    const bool increaseLibdbusMessageReferenceCount = false;
    DBusMessage dbusMessage(libdbusMessage, increaseLibdbusMessageReferenceCount);
    CallStatus callStatus = CallStatus::SUCCESS;

    if (!dbusMessage.isMethodReturnType()) {
        callStatus = CallStatus::REMOTE_ERROR;
    }

    dbusMessageReplyAsyncHandler->onDBusMessageReply(callStatus, dbusMessage);

    // libdbus calls the Cleanup method below
    dbus_pending_call_unref(libdbusPendingCall);
}

void DBusConnection::onLibdbusDataCleanup(void* userData) {
    auto dbusMessageReplyAsyncHandler = reinterpret_cast<DBusMessageReplyAsyncHandler*>(userData);
    delete dbusMessageReplyAsyncHandler;
}


//Would not be needed if libdbus would actually handle its timeouts for pending calls.
void DBusConnection::enforceAsynchronousTimeouts() const {
    enforceTimeoutMutex_.lock();

    //Assert that we DO have a reference to the executing thread, even if the DBusConnection is destroyed.
    //We need it to assess whether we still may access the members of the DBusConnection.
    std::shared_ptr<std::thread> threadPtr = enforcerThread_;

    while (!timeoutMap_.empty()) {
        auto minTimeoutElement = std::min_element(timeoutMap_.begin(), timeoutMap_.end(),
            [] (const TimeoutMapElement& lhs, const TimeoutMapElement& rhs) {
                    return std::get<0>(lhs.second) < std::get<0>(rhs.second);
        });

        int minTimeout = std::get<0>(minTimeoutElement->second);

        enforceTimeoutMutex_.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(minTimeout));

        //Do not access members if the DBusConnection was destroyed during the unlocked phase.
        if (!threadPtr.unique()) {
            enforceTimeoutMutex_.lock();
            auto it = timeoutMap_.begin();
            while (!threadPtr.unique() && it != timeoutMap_.end()) {
                int& currentTimeout = std::get<0>(it->second);
                currentTimeout -= minTimeout;
                if (currentTimeout <= 0) {
                    DBusPendingCall* libdbusPendingCall = it->first;

                    if (!dbus_pending_call_get_completed(libdbusPendingCall)) {
                        dbus_pending_call_cancel(libdbusPendingCall);
                                            DBusMessageReplyAsyncHandler* asyncHandler = std::get<1>(it->second);
                                            DBusMessage& dbusMessageCall = std::get<2>(it->second);
                                            enforceTimeoutMutex_.unlock(); // unlock before making callbacks to application to avoid deadlocks
                                            asyncHandler->onDBusMessageReply(CallStatus::REMOTE_ERROR, dbusMessageCall.createMethodError(DBUS_ERROR_TIMEOUT));
                                            enforceTimeoutMutex_.lock();
                                            delete asyncHandler;

                    }
                    it = timeoutMap_.erase(it);

                    //This unref MIGHT cause the destruction of the last callback object that references the DBusConnection.
                    //So after this unref has been called, it has to be ensured that continuation of the loop is an option.
                    dbus_pending_call_unref(libdbusPendingCall);
                } else {
                    ++it;
                }
            }
        }
    }

    //Normally there is at least the member of DBusConnection plus the local copy of this pointer.
    //If the local copy is the only one remaining, we have to assume that the DBusConnection was
    //destroyed and therefore we must no longer access its members.
    if (!threadPtr.unique()) {
        enforcerThread_.reset();
        enforceTimeoutMutex_.unlock();
    }

    threadPtr->detach();
}

std::future<CallStatus> DBusConnection::sendDBusMessageWithReplyAsync(
        const DBusMessage& dbusMessage,
        std::unique_ptr<DBusMessageReplyAsyncHandler> dbusMessageReplyAsyncHandler,
        int timeoutMilliseconds) const {

    assert(dbusMessage);
    assert(isConnected());

    DBusPendingCall* libdbusPendingCall;
    dbus_bool_t libdbusSuccess;

    suspendDispatching();
    libdbusSuccess = dbus_connection_send_with_reply(libdbusConnection_,
                                                     dbusMessage.libdbusMessage_,
                                                     &libdbusPendingCall,
                                                     timeoutMilliseconds);

    if (!libdbusSuccess || !libdbusPendingCall) {
        dbusMessageReplyAsyncHandler->onDBusMessageReply(CallStatus::CONNECTION_FAILED, dbusMessage.createMethodError(DBUS_ERROR_DISCONNECTED));
        resumeDispatching();
        return dbusMessageReplyAsyncHandler->getFuture();
    }

    libdbusSuccess = dbus_pending_call_set_notify(
                    libdbusPendingCall,
                    onLibdbusPendingCallNotifyThunk,
                    dbusMessageReplyAsyncHandler.get(),
                    onLibdbusDataCleanup);

    if (!libdbusSuccess) {
        dbusMessageReplyAsyncHandler->onDBusMessageReply(CallStatus::OUT_OF_MEMORY, dbusMessage);
        dbus_pending_call_unref(libdbusPendingCall);
        resumeDispatching();
        return dbusMessageReplyAsyncHandler->getFuture();
    }

    DBusMessageReplyAsyncHandler* replyAsyncHandler = dbusMessageReplyAsyncHandler.release();

    const bool mainloopContextIsPresent = (bool) mainLoopContext_.lock();
    if (!mainloopContextIsPresent && timeoutMilliseconds != DBUS_TIMEOUT_INFINITE) {
        dbus_pending_call_ref(libdbusPendingCall);
        std::tuple<int, DBusMessageReplyAsyncHandler*, DBusMessage> toInsert {timeoutMilliseconds, replyAsyncHandler, dbusMessage};

        enforceTimeoutMutex_.lock();
        timeoutMap_.insert( {libdbusPendingCall, toInsert } );
        if (!enforcerThread_) {
            enforcerThread_ = std::make_shared<std::thread>(std::bind(&DBusConnection::enforceAsynchronousTimeouts, this->shared_from_this()));
        }
        enforceTimeoutMutex_.unlock();
    }

    std::future<CallStatus> result = replyAsyncHandler->getFuture();

    resumeDispatching();

    return result;
}


DBusMessage DBusConnection::sendDBusMessageWithReplyAndBlock(const DBusMessage& dbusMessage,
                                                             DBusError& dbusError,
                                                             int timeoutMilliseconds) const {
    assert(dbusMessage);
    assert(!dbusError);
    assert(isConnected());

    suspendDispatching();

    ::DBusMessage* libdbusMessageReply = dbus_connection_send_with_reply_and_block(libdbusConnection_,
                                                                                   dbusMessage.libdbusMessage_,
                                                                                   timeoutMilliseconds,
                                                                                   &dbusError.libdbusError_);

    resumeDispatching();

    if (dbusError) {
        return DBusMessage();
    }

    const bool increaseLibdbusMessageReferenceCount = false;
    return DBusMessage(libdbusMessageReply, increaseLibdbusMessageReferenceCount);
}


bool DBusConnection::singleDispatch() {
    return (dbus_connection_dispatch(libdbusConnection_) == DBUS_DISPATCH_DATA_REMAINS);
}

bool DBusConnection::isDispatchReady() {
    return (dbus_connection_get_dispatch_status(libdbusConnection_) == DBUS_DISPATCH_DATA_REMAINS);
}

DBusProxyConnection::DBusSignalHandlerToken DBusConnection::subscribeForSelectiveBroadcast(
                    bool& subscriptionAccepted,
                    const std::string& objectPath,
                    const std::string& interfaceName,
                    const std::string& interfaceMemberName,
                    const std::string& interfaceMemberSignature,
                    DBusSignalHandler* dbusSignalHandler,
                    DBusProxy* callingProxy) {

    std::string methodName = "subscribeFor" + interfaceMemberName + "Selective";

    subscriptionAccepted = false;
    CommonAPI::CallStatus callStatus;
    DBusProxyHelper<CommonAPI::DBus::DBusSerializableArguments<>,
                    CommonAPI::DBus::DBusSerializableArguments<bool>>::callMethodWithReply(
                    *callingProxy, methodName.c_str(), "", callStatus, subscriptionAccepted);

    DBusProxyConnection::DBusSignalHandlerToken subscriptionToken;

    if (callStatus == CommonAPI::CallStatus::SUCCESS && subscriptionAccepted) {
        subscriptionToken = addSignalMemberHandler(
                        objectPath,
                        interfaceName,
                        interfaceMemberName,
                        interfaceMemberSignature,
                        dbusSignalHandler,
                        true);

        subscriptionAccepted = true;
    }

    return (subscriptionToken);
}

void DBusConnection::unsubscribeFromSelectiveBroadcast(const std::string& eventName,
                                                      DBusProxyConnection::DBusSignalHandlerToken subscription,
                                                      DBusProxy* callingProxy,
                                                      const DBusSignalHandler* dbusSignalHandler) {
    bool lastListenerOnConnectionRemoved = removeSignalMemberHandler(subscription, dbusSignalHandler);

    if (lastListenerOnConnectionRemoved) {
        // send unsubscribe message to stub
        std::string methodName = "unsubscribeFrom" + eventName + "Selective";
        CommonAPI::CallStatus callStatus;
        DBusProxyHelper<CommonAPI::DBus::DBusSerializableArguments<>,
                        CommonAPI::DBus::DBusSerializableArguments<>>::callMethodWithReply(
                        *callingProxy, methodName.c_str(), "", callStatus);
    }
}

DBusProxyConnection::DBusSignalHandlerToken DBusConnection::addSignalMemberHandler(const std::string& objectPath,
                                                                                   const std::string& interfaceName,
                                                                                   const std::string& interfaceMemberName,
                                                                                   const std::string& interfaceMemberSignature,
                                                                                   DBusSignalHandler* dbusSignalHandler,
                                                                                   const bool justAddFilter) {
    DBusSignalHandlerPath dbusSignalHandlerPath(
                    objectPath,
                    interfaceName,
                    interfaceMemberName,
                    interfaceMemberSignature);
    std::lock_guard < std::mutex > dbusSignalLock(signalGuard_);
    auto signalEntry = dbusSignalHandlerTable_.find(dbusSignalHandlerPath);
    const bool isFirstSignalMemberHandler = (signalEntry == dbusSignalHandlerTable_.end());

    if (isFirstSignalMemberHandler) {
        addLibdbusSignalMatchRule(objectPath, interfaceName, interfaceMemberName, justAddFilter);
        std::set<DBusSignalHandler*> handlerList;
        handlerList.insert(dbusSignalHandler);

        dbusSignalHandlerTable_.insert({dbusSignalHandlerPath,
            std::make_pair(std::make_shared<std::mutex>(), std::move(handlerList)) } );

    } else {

        signalEntry->second.first->lock();
        signalEntry->second.second.insert(dbusSignalHandler);
        signalEntry->second.first->unlock();
    }

    return dbusSignalHandlerPath;
}

bool DBusConnection::removeSignalMemberHandler(const DBusSignalHandlerToken& dbusSignalHandlerToken,
                                               const DBusSignalHandler* dbusSignalHandler) {
    bool lastHandlerRemoved = false;

    auto signalEntry = dbusSignalHandlerTable_.find(dbusSignalHandlerToken);
    if (signalEntry != dbusSignalHandlerTable_.end()) {

        signalEntry->second.first->lock();

        auto selectedHandler = signalEntry->second.second.find(const_cast<DBusSignalHandler*>(dbusSignalHandler));
        if (selectedHandler != signalEntry->second.second.end()) {
            signalEntry->second.second.erase(selectedHandler);
            lastHandlerRemoved = (signalEntry->second.second.empty());
        }
        signalEntry->second.first->unlock();
    }
    return lastHandlerRemoved;
}

bool DBusConnection::addObjectManagerSignalMemberHandler(const std::string& dbusBusName,
                                                         DBusSignalHandler* dbusSignalHandler) {
    if (dbusBusName.length() < 2) {
        return false;
    }

    std::lock_guard<std::mutex> dbusSignalLock(dbusObjectManagerSignalGuard_);
    auto dbusSignalMatchRuleIterator = dbusObjectManagerSignalMatchRulesMap_.find(dbusBusName);
    const bool isDBusSignalMatchRuleFound = (dbusSignalMatchRuleIterator != dbusObjectManagerSignalMatchRulesMap_.end());

    if (!isDBusSignalMatchRuleFound) {
        if (isConnected() && !addObjectManagerSignalMatchRule(dbusBusName)) {
            return false;
        }

        auto insertResult = dbusObjectManagerSignalMatchRulesMap_.insert({ dbusBusName, 0 });
        const bool isInsertSuccessful = insertResult.second;

        if (!isInsertSuccessful) {
            if (isConnected()) {
                const bool isRemoveSignalMatchRuleSuccessful = removeObjectManagerSignalMatchRule(dbusBusName);
                assert(isRemoveSignalMatchRuleSuccessful);
            }

            return false;
        }

        dbusSignalMatchRuleIterator = insertResult.first;
    }

    size_t& dbusSignalMatchRuleRefernceCount = dbusSignalMatchRuleIterator->second;
    dbusSignalMatchRuleRefernceCount++;

    dbusObjectManagerSignalHandlerTable_.insert({ dbusBusName, dbusSignalHandler });

    return true;
}

bool DBusConnection::removeObjectManagerSignalMemberHandler(const std::string& dbusBusName,
                                                            DBusSignalHandler* dbusSignalHandler) {
    assert(!dbusBusName.empty());

    std::lock_guard<std::mutex> dbusSignalLock(dbusObjectManagerSignalGuard_);
    auto dbusSignalMatchRuleIterator = dbusObjectManagerSignalMatchRulesMap_.find(dbusBusName);
    const bool isDBusSignalMatchRuleFound = (dbusSignalMatchRuleIterator != dbusObjectManagerSignalMatchRulesMap_.end());

    if (!isDBusSignalMatchRuleFound) {
        return true;
    }

    auto dbusObjectManagerSignalHandlerRange = dbusObjectManagerSignalHandlerTable_.equal_range(dbusBusName);
    auto dbusObjectManagerSignalHandlerIterator = std::find_if(
        dbusObjectManagerSignalHandlerRange.first,
        dbusObjectManagerSignalHandlerRange.second,
        [&](decltype(*dbusObjectManagerSignalHandlerRange.first)& it) { return it.second == dbusSignalHandler; });
    const bool isDBusSignalHandlerFound = (dbusObjectManagerSignalHandlerIterator != dbusObjectManagerSignalHandlerRange.second);

    if (!isDBusSignalHandlerFound) {
        return false;
    }

    size_t& dbusSignalMatchRuleReferenceCount = dbusSignalMatchRuleIterator->second;

    assert(dbusSignalMatchRuleReferenceCount > 0);
    dbusSignalMatchRuleReferenceCount--;

    const bool isLastDBusSignalMatchRuleReference = (dbusSignalMatchRuleReferenceCount == 0);
    if (isLastDBusSignalMatchRuleReference) {
        if (isConnected() && !removeObjectManagerSignalMatchRule(dbusBusName)) {
            return false;
        }

        dbusObjectManagerSignalMatchRulesMap_.erase(dbusSignalMatchRuleIterator);
    }

    dbusObjectManagerSignalHandlerTable_.erase(dbusObjectManagerSignalHandlerIterator);

    return true;
}

bool DBusConnection::addObjectManagerSignalMatchRule(const std::string& dbusBusName) {
    std::ostringstream dbusMatchRuleStringStream;

    dbusMatchRuleStringStream << "type='signal'"
                                << ",sender='" << dbusBusName << "'"
                                << ",interface='org.freedesktop.DBus.ObjectManager'";

    return addLibdbusSignalMatchRule(dbusMatchRuleStringStream.str());
}

bool DBusConnection::removeObjectManagerSignalMatchRule(const std::string& dbusBusName) {
    std::ostringstream dbusMatchRuleStringStream;

    dbusMatchRuleStringStream << "type='signal'"
                                << ",sender='" << dbusBusName << "'"
                                << ",interface='org.freedesktop.DBus.ObjectManager'";

    return removeLibdbusSignalMatchRule(dbusMatchRuleStringStream.str());
}

/**
 * Called only if connected
 *
 * @param dbusMatchRule
 * @return
 */
bool DBusConnection::addLibdbusSignalMatchRule(const std::string& dbusMatchRule) {
    bool libdbusSuccess = true;

    suspendDispatching();

    // add the libdbus message signal filter
    if (!libdbusSignalMatchRulesCount_) {

        libdbusSuccess = (bool) dbus_connection_add_filter(
            libdbusConnection_,
            &onLibdbusSignalFilterThunk,
            this,
            NULL);
    }

    // finally add the match rule
    if (libdbusSuccess) {
        DBusError dbusError;
        dbus_bus_add_match(libdbusConnection_, dbusMatchRule.c_str(), &dbusError.libdbusError_);
        libdbusSuccess = !dbusError;
    }

    if (libdbusSuccess) {
        libdbusSignalMatchRulesCount_++;
    }

    resumeDispatching();

    return libdbusSuccess;
}

/**
 * Called only if connected
 *
 * @param dbusMatchRule
 * @return
 */
bool DBusConnection::removeLibdbusSignalMatchRule(const std::string& dbusMatchRule) {

    if(libdbusSignalMatchRulesCount_ == 0)
        return true;

    suspendDispatching();

    DBusError dbusError;
    dbus_bus_remove_match(libdbusConnection_, dbusMatchRule.c_str(), &dbusError.libdbusError_);

    if (!dbusError) {
        libdbusSignalMatchRulesCount_--;
        if (libdbusSignalMatchRulesCount_ == 0) {
            dbus_connection_remove_filter(libdbusConnection_, &onLibdbusSignalFilterThunk, this);
        }
    }

    resumeDispatching();

    return true;
}

void DBusConnection::registerObjectPath(const std::string& objectPath) {
    assert(!objectPath.empty());
    assert(objectPath[0] == '/');

    auto handlerIterator = libdbusRegisteredObjectPaths_.find(objectPath);
    const bool foundRegisteredObjectPathHandler = handlerIterator != libdbusRegisteredObjectPaths_.end();

    if (foundRegisteredObjectPathHandler) {
        uint32_t& referenceCount = handlerIterator->second;
        referenceCount++;
        return;
    }

    libdbusRegisteredObjectPaths_.insert(LibdbusRegisteredObjectPathHandlersTable::value_type(objectPath, 1));

    if (isConnected()) {
        DBusError dbusError;
        const dbus_bool_t libdbusSuccess = dbus_connection_try_register_object_path(libdbusConnection_,
                                                                                    objectPath.c_str(),
                                                                                    getDBusObjectPathVTable(),
                                                                                    this,
                                                                                    &dbusError.libdbusError_);
        assert(libdbusSuccess);
        assert(!dbusError);
    }
}

void DBusConnection::unregisterObjectPath(const std::string& objectPath) {
    assert(!objectPath.empty());
    assert(objectPath[0] == '/');

    auto handlerIterator = libdbusRegisteredObjectPaths_.find(objectPath);
    const bool foundRegisteredObjectPathHandler = handlerIterator != libdbusRegisteredObjectPaths_.end();

    assert(foundRegisteredObjectPathHandler);

    uint32_t& referenceCount = handlerIterator->second;
    if (referenceCount > 1) {
        referenceCount--;
        return;
    }

    libdbusRegisteredObjectPaths_.erase(handlerIterator);

    if (isConnected()) {
        dbus_bool_t libdbusSuccess = dbus_connection_unregister_object_path(libdbusConnection_,
                                                                            objectPath.c_str());

        assert(libdbusSuccess);
    }
}

void DBusConnection::addLibdbusSignalMatchRule(const std::string& objectPath,
                                               const std::string& interfaceName,
                                               const std::string& interfaceMemberName,
                                               const bool justAddFilter) {
    DBusSignalMatchRuleTuple dbusSignalMatchRuleTuple(objectPath, interfaceName, interfaceMemberName);
    auto matchRuleIterator = dbusSignalMatchRulesMap_.find(dbusSignalMatchRuleTuple);
    const bool matchRuleFound = matchRuleIterator != dbusSignalMatchRulesMap_.end();

    if (matchRuleFound) {
        uint32_t& matchRuleReferenceCount = matchRuleIterator->second.first;
        matchRuleReferenceCount++;
        return;
    }

    const bool isFirstMatchRule = dbusSignalMatchRulesMap_.empty();

    // generate D-Bus match rule string
    std::ostringstream matchRuleStringStream;

    matchRuleStringStream << "type='signal'";
    matchRuleStringStream << ",path='" << objectPath << "'";
    matchRuleStringStream << ",interface='" << interfaceName << "'";
    matchRuleStringStream << ",member='" << interfaceMemberName << "'";

    // add the match rule string to the map with reference count set to 1
    std::string matchRuleString = matchRuleStringStream.str();
    auto success = dbusSignalMatchRulesMap_.insert(
                    DBusSignalMatchRulesMap::value_type(dbusSignalMatchRuleTuple,
                                    DBusSignalMatchRuleMapping(1, matchRuleString)));
    assert(success.second);

    if (isConnected()) {
        bool libdbusSuccess = true;
        suspendDispatching();
        // add the libdbus message signal filter
        if (isFirstMatchRule) {

            libdbusSuccess = dbus_connection_add_filter(libdbusConnection_,
                            &onLibdbusSignalFilterThunk,
                            this,
                            NULL);
            assert(libdbusSuccess);
        }

        if (!justAddFilter)
        {
            // finally add the match rule
            DBusError dbusError;
            dbus_bus_add_match(libdbusConnection_, matchRuleString.c_str(), &dbusError.libdbusError_);
            assert(!dbusError);
        }

        if (libdbusSuccess) {
            libdbusSignalMatchRulesCount_++;
        }

        resumeDispatching();
    }
}

void DBusConnection::removeLibdbusSignalMatchRule(const std::string& objectPath,
                                                  const std::string& interfaceName,
                                                  const std::string& interfaceMemberName) {
    auto selfReference = this->shared_from_this();
    DBusSignalMatchRuleTuple dbusSignalMatchRuleTuple(objectPath, interfaceName, interfaceMemberName);
    auto matchRuleIterator = dbusSignalMatchRulesMap_.find(dbusSignalMatchRuleTuple);
    const bool matchRuleFound = matchRuleIterator != dbusSignalMatchRulesMap_.end();

    assert(matchRuleFound);

    uint32_t& matchRuleReferenceCount = matchRuleIterator->second.first;
    if (matchRuleReferenceCount > 1) {
        matchRuleReferenceCount--;
        return;
    }

    if (isConnected()) {
        const std::string& matchRuleString = matchRuleIterator->second.second;
        const bool libdbusSuccess = removeLibdbusSignalMatchRule(matchRuleString);
        assert(libdbusSuccess);
    }

    dbusSignalMatchRulesMap_.erase(matchRuleIterator);
}

void DBusConnection::initLibdbusObjectPathHandlerAfterConnect() {
    assert(isConnected());

    // nothing to do if there aren't any registered object path handlers
    if (libdbusRegisteredObjectPaths_.empty()) {
        return;
    }

    DBusError dbusError;
    dbus_bool_t libdbusSuccess;

    for (    auto handlerIterator = libdbusRegisteredObjectPaths_.begin();
             handlerIterator != libdbusRegisteredObjectPaths_.end();
             handlerIterator++) {
        const std::string& objectPath = handlerIterator->first;

        dbusError.clear();

        libdbusSuccess = dbus_connection_try_register_object_path(libdbusConnection_,
                                                                  objectPath.c_str(),
                                                                  getDBusObjectPathVTable(),
                                                                  this,
                                                                  &dbusError.libdbusError_);
        assert(libdbusSuccess);
        assert(!dbusError);
    }
}

void DBusConnection::initLibdbusSignalFilterAfterConnect() {
    assert(isConnected());

    // proxy/stub match rules
    for (const auto& dbusSignalMatchRuleIterator : dbusSignalMatchRulesMap_) {
        const auto& dbusSignalMatchRuleMapping = dbusSignalMatchRuleIterator.second;
        const std::string& dbusMatchRuleString = dbusSignalMatchRuleMapping.second;
        const bool libdbusSuccess = addLibdbusSignalMatchRule(dbusMatchRuleString);
        assert(libdbusSuccess);
    }

    // object manager match rules (see DBusServiceRegistry)
    for (const auto& dbusObjectManagerSignalMatchRuleIterator : dbusObjectManagerSignalMatchRulesMap_) {
        const std::string& dbusBusName = dbusObjectManagerSignalMatchRuleIterator.first;
        const bool libdbusSuccess = addObjectManagerSignalMatchRule(dbusBusName);
        assert(libdbusSuccess);
    }
}

::DBusHandlerResult DBusConnection::onLibdbusObjectPathMessage(::DBusMessage* libdbusMessage) {
    assert(libdbusMessage);

    // handle only method call messages
    if (dbus_message_get_type(libdbusMessage) != DBUS_MESSAGE_TYPE_METHOD_CALL) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    bool isDBusMessageHandled = dbusObjectMessageHandler_(DBusMessage(libdbusMessage));
    return isDBusMessageHandled ? DBUS_HANDLER_RESULT_HANDLED : DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

template<typename DBusSignalHandlersTable>
void notifyDBusSignalHandlers(DBusSignalHandlersTable& dbusSignalHandlerstable,
                              typename DBusSignalHandlersTable::iterator& signalEntry,
                              const CommonAPI::DBus::DBusMessage& dbusMessage,
                              ::DBusHandlerResult& dbusHandlerResult) {
    if (signalEntry == dbusSignalHandlerstable.end() || signalEntry->second.second.empty()) {
        dbusHandlerResult = DBUS_HANDLER_RESULT_HANDLED;
        return;
    }

    signalEntry->second.first->lock();

    auto handlerEntry = signalEntry->second.second.begin();
    while (handlerEntry != signalEntry->second.second.end()) {
        DBusProxyConnection::DBusSignalHandler* dbusSignalHandler = *handlerEntry;

        auto dbusSignalHandlerSubscriptionStatus = dbusSignalHandler->onSignalDBusMessage(dbusMessage);

        if (dbusSignalHandlerSubscriptionStatus == SubscriptionStatus::CANCEL) {
            handlerEntry = signalEntry->second.second.erase(handlerEntry);
        } else {
            handlerEntry++;
        }
    }
    dbusHandlerResult = DBUS_HANDLER_RESULT_HANDLED;
    signalEntry->second.first->unlock();
}

template<typename DBusSignalHandlersTable>
void notifyDBusOMSignalHandlers(DBusSignalHandlersTable& dbusSignalHandlerstable,
                              std::pair<typename DBusSignalHandlersTable::iterator,
                                        typename DBusSignalHandlersTable::iterator>& equalRange,
                              const CommonAPI::DBus::DBusMessage& dbusMessage,
                              ::DBusHandlerResult& dbusHandlerResult) {
    if (equalRange.first != equalRange.second) {
        dbusHandlerResult = DBUS_HANDLER_RESULT_HANDLED;
    }

    while (equalRange.first != equalRange.second) {
        DBusProxyConnection::DBusSignalHandler* dbusSignalHandler = equalRange.first->second;

        auto dbusSignalHandlerSubscriptionStatus = dbusSignalHandler->onSignalDBusMessage(dbusMessage);

        if (dbusSignalHandlerSubscriptionStatus == SubscriptionStatus::CANCEL) {
            equalRange.first = dbusSignalHandlerstable.erase(equalRange.first);
        } else {
            equalRange.first++;
        }
    }
}

::DBusHandlerResult DBusConnection::onLibdbusSignalFilter(::DBusMessage* libdbusMessage) {
    assert(libdbusMessage);

    auto selfReference = this->shared_from_this();

    // handle only signal messages
    if (dbus_message_get_type(libdbusMessage) != DBUS_MESSAGE_TYPE_SIGNAL) {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

    const char* objectPath = dbus_message_get_path(libdbusMessage);
    const char* interfaceName = dbus_message_get_interface(libdbusMessage);
    const char* interfaceMemberName = dbus_message_get_member(libdbusMessage);
    const char* interfaceMemberSignature = dbus_message_get_signature(libdbusMessage);

    assert(objectPath);
    assert(interfaceName);
    assert(interfaceMemberName);
    assert(interfaceMemberSignature);

    DBusMessage dbusMessage(libdbusMessage);
    ::DBusHandlerResult dbusHandlerResult = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    signalGuard_.lock();
    auto signalEntry = dbusSignalHandlerTable_.find(DBusSignalHandlerPath(
        objectPath,
        interfaceName,
        interfaceMemberName,
        interfaceMemberSignature));
    signalGuard_.unlock();

    notifyDBusSignalHandlers(dbusSignalHandlerTable_,
                                    signalEntry, dbusMessage, dbusHandlerResult);


    if (dbusMessage.hasInterfaceName("org.freedesktop.DBus.ObjectManager")) {
        const char* dbusSenderName = dbusMessage.getSenderName();
        assert(dbusSenderName);

        dbusObjectManagerSignalGuard_.lock();
        auto dbusObjectManagerSignalHandlerIteratorPair = dbusObjectManagerSignalHandlerTable_.equal_range(dbusSenderName);
        notifyDBusOMSignalHandlers(dbusObjectManagerSignalHandlerTable_,
                                 dbusObjectManagerSignalHandlerIteratorPair,
                                 dbusMessage,
                                 dbusHandlerResult);
        dbusObjectManagerSignalGuard_.unlock();
    }

    return dbusHandlerResult;
}

::DBusHandlerResult DBusConnection::onLibdbusSignalFilterThunk(::DBusConnection* libdbusConnection,
                                                               ::DBusMessage* libdbusMessage,
                                                               void* userData) {
    assert(libdbusConnection);
    assert(libdbusMessage);
    assert(userData);

    DBusConnection* dbusConnection = reinterpret_cast<DBusConnection*>(userData);

    assert(dbusConnection->libdbusConnection_ == libdbusConnection);

    return dbusConnection->onLibdbusSignalFilter(libdbusMessage);
}

::DBusHandlerResult DBusConnection::onLibdbusObjectPathMessageThunk(::DBusConnection* libdbusConnection,
                                                                    ::DBusMessage* libdbusMessage,
                                                                    void* userData) {
    assert(libdbusConnection);
    assert(libdbusMessage);
    assert(userData);

    DBusConnection* dbusConnection = reinterpret_cast<DBusConnection*>(userData);

    assert(dbusConnection->libdbusConnection_ == libdbusConnection);

    return dbusConnection->onLibdbusObjectPathMessage(libdbusMessage);
}


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
