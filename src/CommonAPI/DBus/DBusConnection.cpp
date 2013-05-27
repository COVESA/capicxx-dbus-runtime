/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusConnection.h"
#include "DBusInputStream.h"

#include <sstream>
#include <cassert>
#include <future>

namespace CommonAPI {
namespace DBus {

DBusConnectionStatusEvent::DBusConnectionStatusEvent(DBusConnection* dbusConnection):
                dbusConnection_(dbusConnection) {
}

void DBusConnectionStatusEvent::onListenerAdded(const CancellableListener& listener) {
    if (dbusConnection_->isConnected())
        listener(AvailabilityStatus::AVAILABLE);
}


DBusObjectPathVTable DBusConnection::libdbusObjectPathVTable_ = {
                NULL, // no need to handle unregister callbacks
                &DBusConnection::onLibdbusObjectPathMessageThunk
};

void DBusConnection::dispatch(std::shared_ptr<DBusConnection> selfReference) {
    while (!stopDispatching_ && readWriteDispatch(10) && !selfReference.unique()) {
        if(pauseDispatching_) {
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
                busType_(busType),
                libdbusConnection_(NULL),
                dbusConnectionStatusEvent_(this),
                stopDispatching_(false),
                pauseDispatching_(false),
                dispatchThread_(NULL),
                dbusObjectMessageHandler_(),
                watchContext_(NULL)  {
    dbus_threads_init_default();
}

DBusConnection::DBusConnection(::DBusConnection* libDbusConnection) :
                busType_(WRAPPED),
                libdbusConnection_(libDbusConnection),
                dbusConnectionStatusEvent_(this),
                stopDispatching_(false),
                pauseDispatching_(false),
                dispatchThread_(NULL),
                dbusObjectMessageHandler_(),
                watchContext_(NULL) {
    dbus_threads_init_default();
}

void DBusConnection::setObjectPathMessageHandler(DBusObjectPathMessageHandler handler) {
    dbusObjectMessageHandler_ = handler;
}

bool DBusConnection::isObjectPathMessageHandlerSet() {
    return dbusObjectMessageHandler_.operator bool();
}

DBusConnection::~DBusConnection() {
    if(auto lockedContext = mainLoopContext_.lock()) {
        dbus_connection_set_watch_functions(libdbusConnection_, NULL, NULL, NULL, NULL, NULL);
        dbus_connection_set_timeout_functions(libdbusConnection_, NULL, NULL, NULL, NULL, NULL);

        lockedContext->deregisterDispatchSource(dispatchSource_);
        delete watchContext_;
        delete dispatchSource_;
    }

    disconnect();
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

    if(startDispatchThread) {
        dispatchThread_ = new std::thread(&DBusConnection::dispatch, this, this->shared_from_this());
    }
    stopDispatching_ = !startDispatchThread;

    dbusConnectionStatusEvent_.notifyListeners(AvailabilityStatus::AVAILABLE);

    return true;
}

void DBusConnection::disconnect() {
    std::lock_guard<std::mutex> dbusConnectionLock(libdbusConnectionGuard_);
    if (isConnected()) {
        dbusConnectionStatusEvent_.notifyListeners(AvailabilityStatus::NOT_AVAILABLE);

        if (!dbusSignalMatchRulesMap_.empty()) {
            dbus_connection_remove_filter(libdbusConnection_, &onLibdbusSignalFilterThunk, this);
        }

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

    suspendDispatching();

    const int libdbusStatus = dbus_bus_request_name(libdbusConnection_,
                                                    serviceName.c_str(),
                                                    DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                                    &dbusError.libdbusError_);

    resumeDispatching();

    const bool isServiceNameAcquired = (libdbusStatus == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);

    return isServiceNameAcquired;
}

bool DBusConnection::releaseServiceName(const std::string& serviceName) const {
    DBusError dbusError;
    suspendDispatching();
    const int libdbusStatus = dbus_bus_release_name(libdbusConnection_,
                                                    serviceName.c_str(),
                                                    &dbusError.libdbusError_);
    resumeDispatching();
    const bool isServiceNameReleased = (libdbusStatus == DBUS_RELEASE_NAME_REPLY_RELEASED);

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

std::future<CallStatus> DBusConnection::sendDBusMessageWithReplyAsync(
		const DBusMessage& dbusMessage,
		std::unique_ptr<DBusMessageReplyAsyncHandler> dbusMessageReplyAsyncHandler,
		int timeoutMilliseconds) const {

    assert(dbusMessage);
    assert(isConnected());

    DBusPendingCall* libdbusPendingCall;
    dbus_bool_t libdbusSuccess;

    libdbusSuccess = dbus_connection_send_with_reply(
                    libdbusConnection_,
                    dbusMessage.libdbusMessage_,
                    &libdbusPendingCall,
                    timeoutMilliseconds);

    if (!libdbusSuccess || !libdbusPendingCall) {
    	dbusMessageReplyAsyncHandler->onDBusMessageReply(CallStatus::CONNECTION_FAILED, dbusMessage);
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
        return dbusMessageReplyAsyncHandler->getFuture();
    }

    return dbusMessageReplyAsyncHandler.release()->getFuture();
}


DBusMessage DBusConnection::sendDBusMessageWithReplyAndBlock(const DBusMessage& dbusMessage,
                                                             DBusError& dbusError,
                                                             int timeoutMilliseconds) const {
	auto selfReference = this->shared_from_this();

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


DBusProxyConnection::DBusSignalHandlerToken DBusConnection::addSignalMemberHandler(const std::string& objectPath,
                                                                                   const std::string& interfaceName,
                                                                                   const std::string& interfaceMemberName,
                                                                                   const std::string& interfaceMemberSignature,
                                                                                   DBusSignalHandler* dbusSignalHandler) {
    DBusSignalHandlerPath dbusSignalHandlerPath(
                    objectPath,
                    interfaceName,
                    interfaceMemberName,
                    interfaceMemberSignature);
    std::lock_guard<std::mutex> dbusSignalLock(signalGuard_);
    const bool isFirstSignalMemberHandler = dbusSignalHandlerTable_.find(dbusSignalHandlerPath) == dbusSignalHandlerTable_.end();
    dbusSignalHandlerTable_.insert(DBusSignalHandlerTable::value_type(dbusSignalHandlerPath, dbusSignalHandler));

    if (isFirstSignalMemberHandler) {
        addLibdbusSignalMatchRule(objectPath, interfaceName, interfaceMemberName);
    }

    return dbusSignalHandlerPath;
}

void DBusConnection::removeSignalMemberHandler(const DBusSignalHandlerToken& dbusSignalHandlerToken) {
    std::lock_guard<std::mutex> dbusSignalLock(signalGuard_);
    auto equalRangeIteratorPair = dbusSignalHandlerTable_.equal_range(dbusSignalHandlerToken);
    if (equalRangeIteratorPair.first != equalRangeIteratorPair.second) {
		// advance to the next element
		equalRangeIteratorPair.first++;

		// check if the first element was the only element
		const bool isLastSignalMemberHandler = equalRangeIteratorPair.first == equalRangeIteratorPair.second;

		if (isLastSignalMemberHandler) {
			const std::string& objectPath = std::get<0>(dbusSignalHandlerToken);
			const std::string& interfaceName = std::get<1>(dbusSignalHandlerToken);
			const std::string& interfaceMemberName = std::get<2>(dbusSignalHandlerToken);

			removeLibdbusSignalMatchRule(objectPath, interfaceName, interfaceMemberName);
		}

		dbusSignalHandlerTable_.erase(dbusSignalHandlerToken);
    }
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
                                                                                    &libdbusObjectPathVTable_,
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
                                               const std::string& interfaceMemberName) {
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

    // if not connected the filter and the rules will be added as soon as the connection is established
    if (isConnected()) {
        suspendDispatching();
        // add the libdbus message signal filter
        if (isFirstMatchRule) {
            const dbus_bool_t libdbusSuccess = dbus_connection_add_filter(libdbusConnection_,
                                                                          &onLibdbusSignalFilterThunk,
                                                                          this,
                                                                          NULL);
            assert(libdbusSuccess);
        }

        // finally add the match rule
        DBusError dbusError;
        dbus_bus_add_match(libdbusConnection_, matchRuleString.c_str(), &dbusError.libdbusError_);
        assert(!dbusError);

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

    const std::string& matchRuleString = matchRuleIterator->second.second;

    suspendDispatching();

    DBusError dbusError;
    dbus_bus_remove_match(libdbusConnection_, matchRuleString.c_str(), &dbusError.libdbusError_);

    assert(!dbusError);

    dbusSignalMatchRulesMap_.erase(matchRuleIterator);

    const bool isLastMatchRule = dbusSignalMatchRulesMap_.empty();
    if (isLastMatchRule) {
        dbus_connection_remove_filter(libdbusConnection_, &onLibdbusSignalFilterThunk, this);
    }

    resumeDispatching();
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
                                                                  &libdbusObjectPathVTable_,
                                                                  this,
                                                                  &dbusError.libdbusError_);
        assert(libdbusSuccess);
        assert(!dbusError);
    }
}

void DBusConnection::initLibdbusSignalFilterAfterConnect() {
    assert(isConnected());

    // nothing to do if there aren't any signal match rules
    if (dbusSignalMatchRulesMap_.empty())
        return;

    suspendDispatching();

    // first we add the libdbus message signal filter
    const dbus_bool_t libdbusSuccess = dbus_connection_add_filter(libdbusConnection_,
                                                                  &onLibdbusSignalFilterThunk,
                                                                  this,
                                                                  NULL);
    assert(libdbusSuccess);

    // then we upload all match rules to the dbus-daemon
    DBusError dbusError;
    for (auto iterator = dbusSignalMatchRulesMap_.begin(); iterator != dbusSignalMatchRulesMap_.end(); iterator++) {
        const std::string& matchRuleString = iterator->second.second;

        dbusError.clear();
        dbus_bus_add_match(libdbusConnection_, matchRuleString.c_str(), &dbusError.libdbusError_);
        assert(!dbusError);
    }
    resumeDispatching();
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

    std::lock_guard<std::mutex> dbusSignalLock(signalGuard_);
    DBusSignalHandlerPath dbusSignalHandlerPath(objectPath, interfaceName, interfaceMemberName, interfaceMemberSignature);
    auto equalRangeIteratorPair = dbusSignalHandlerTable_.equal_range(dbusSignalHandlerPath);

    if (equalRangeIteratorPair.first != equalRangeIteratorPair.second) {
        DBusMessage dbusMessage(libdbusMessage);

        while (equalRangeIteratorPair.first != equalRangeIteratorPair.second) {
            DBusSignalHandler* dbusSignalHandler = equalRangeIteratorPair.first->second;
            const SubscriptionStatus dbusSignalHandlerSubscriptionStatus = dbusSignalHandler->onSignalDBusMessage(dbusMessage);

            if (dbusSignalHandlerSubscriptionStatus == SubscriptionStatus::CANCEL) {
            	equalRangeIteratorPair.first = dbusSignalHandlerTable_.erase(equalRangeIteratorPair.first);
            } else {
            	equalRangeIteratorPair.first++;
            }
        }

        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
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

} // namespace DBus
} // namespace CommonAPI

