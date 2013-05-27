/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "DBusMainLoopContext.h"
#include "DBusConnection.h"

#include <poll.h>
#include <chrono>


namespace CommonAPI {
namespace DBus {


DBusDispatchSource::DBusDispatchSource(DBusConnection* dbusConnection):
    dbusConnection_(dbusConnection) {
}

DBusDispatchSource::~DBusDispatchSource() {
}

bool DBusDispatchSource::prepare(int64_t& timeout) {
    return dbusConnection_->isDispatchReady();
}

bool DBusDispatchSource::check() {
    return dbusConnection_->isDispatchReady();
}

bool DBusDispatchSource::dispatch() {
    return dbusConnection_->singleDispatch();
}


DBusWatch::DBusWatch(::DBusWatch* libdbusWatch, std::weak_ptr<MainLoopContext>& mainLoopContext):
                libdbusWatch_(libdbusWatch),
                mainLoopContext_(mainLoopContext),
                channelFlags_(0) {
    assert(libdbusWatch_);
}

bool DBusWatch::isReadyToBeWatched() {
    return dbus_watch_get_enabled(libdbusWatch_);
}

void DBusWatch::startWatching() {
    channelFlags_ = dbus_watch_get_flags(libdbusWatch_);
    short int pollFlags = POLLERR | POLLHUP;
    if(channelFlags_ & DBUS_WATCH_READABLE) {
        pollFlags |= POLLIN;
    }
    if(channelFlags_ & DBUS_WATCH_WRITABLE) {
        pollFlags |= POLLOUT;
    }

    pollFileDescriptor_.fd = dbus_watch_get_unix_fd(libdbusWatch_);
    pollFileDescriptor_.events = pollFlags;
    pollFileDescriptor_.revents = 0;

    auto lockedContext = mainLoopContext_.lock();
    assert(lockedContext);
    lockedContext->registerWatch(this);
}

void DBusWatch::stopWatching() {
    auto lockedContext = mainLoopContext_.lock();
    assert(lockedContext);
    lockedContext->deregisterWatch(this);
}

const pollfd& DBusWatch::getAssociatedFileDescriptor() {
    return pollFileDescriptor_;
}

//XXX Default hierfür die revent-flags?
void DBusWatch::dispatch(unsigned int eventFlags) {
    dbus_watch_handle(libdbusWatch_, eventFlags);
}

const std::vector<DispatchSource*>& DBusWatch::getDependentDispatchSources() {
    return dependentDispatchSources_;
}

void DBusWatch::addDependentDispatchSource(DispatchSource* dispatchSource) {
    dependentDispatchSources_.push_back(dispatchSource);
}


DBusTimeout::DBusTimeout(::DBusTimeout* libdbusTimeout, std::weak_ptr<MainLoopContext>& mainLoopContext) :
                libdbusTimeout_(libdbusTimeout),
                mainLoopContext_(mainLoopContext),
                dueTimeInMs_(TIMEOUT_INFINITE) {
}

bool DBusTimeout::isReadyToBeMonitored() {
    return dbus_timeout_get_enabled(libdbusTimeout_);
}

void DBusTimeout::startMonitoring() {
    auto lockedContext = mainLoopContext_.lock();
    assert(lockedContext);
    recalculateDueTime();
    lockedContext->registerTimeoutSource(this);
}

void DBusTimeout::stopMonitoring() {
    dueTimeInMs_ = TIMEOUT_INFINITE;
    auto lockedContext = mainLoopContext_.lock();
    assert(lockedContext);
    lockedContext->deregisterTimeoutSource(this);
}

bool DBusTimeout::dispatch() {
    recalculateDueTime();
    dbus_timeout_handle(libdbusTimeout_);
    return true;
}

int64_t DBusTimeout::getTimeoutInterval() const {
    return dbus_timeout_get_interval(libdbusTimeout_);
}

int64_t DBusTimeout::getReadyTime() const {
    return dueTimeInMs_;
}

void DBusTimeout::recalculateDueTime() {
    if(dbus_timeout_get_enabled(libdbusTimeout_)) {
        unsigned int intervalInMs = dbus_timeout_get_interval(libdbusTimeout_);
        dueTimeInMs_ = getCurrentTimeInMs() + intervalInMs;
    } else {
        dueTimeInMs_ = TIMEOUT_INFINITE;
    }
}


} // namespace DBus
} // namespace CommonAPI
