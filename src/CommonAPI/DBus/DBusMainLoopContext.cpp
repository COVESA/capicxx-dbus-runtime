// Copyright (C) 2013-2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifdef WIN32
#include <WinSock2.h>
#else
#include <poll.h>
#include <unistd.h>
#endif

#include <fcntl.h>
#include <cstdio>

#include <chrono>

#include <CommonAPI/DBus/DBusMainLoopContext.hpp>
#include <CommonAPI/DBus/DBusConnection.hpp>

namespace CommonAPI {
namespace DBus {

DBusDispatchSource::DBusDispatchSource(DBusConnection* dbusConnection):
    dbusConnection_(dbusConnection) {
}

DBusDispatchSource::~DBusDispatchSource() {
}

bool DBusDispatchSource::prepare(int64_t &_timeout) {
    _timeout = -1;
    return dbusConnection_->isDispatchReady();
}

bool DBusDispatchSource::check() {
    return dbusConnection_->isDispatchReady();
}

bool DBusDispatchSource::dispatch() {
    return dbusConnection_->singleDispatch();
}

DBusQueueDispatchSource::DBusQueueDispatchSource(DBusQueueWatch* watch) :
    watch_(watch) {
    watch_->addDependentDispatchSource(this);
}

DBusQueueDispatchSource::~DBusQueueDispatchSource() {
    std::unique_lock<std::mutex> itsLock(watchMutex_);
    watch_->removeDependentDispatchSource(this);
}

bool DBusQueueDispatchSource::prepare(int64_t& timeout) {
    std::unique_lock<std::mutex> itsLock(watchMutex_);
    timeout = -1;
    return !watch_->emptyQueue();
}

bool DBusQueueDispatchSource::check() {
    std::unique_lock<std::mutex> itsLock(watchMutex_);
    return !watch_->emptyQueue();
}

bool DBusQueueDispatchSource::dispatch() {
    std::unique_lock<std::mutex> itsLock(watchMutex_);
    if (!watch_->emptyQueue()) {
        auto queueEntry = watch_->frontQueue();
        watch_->popQueue();
        watch_->processQueueEntry(queueEntry);
    }

    return !watch_->emptyQueue();
}

DBusWatch::DBusWatch(::DBusWatch* libdbusWatch, std::weak_ptr<MainLoopContext>& mainLoopContext,
                     std::weak_ptr<DBusConnection>& dbusConnection):
                libdbusWatch_(libdbusWatch),
                mainLoopContext_(mainLoopContext),
                dbusConnection_(dbusConnection) {
    if (NULL == libdbusWatch_) {
        COMMONAPI_ERROR(std::string(__FUNCTION__) + " libdbusWatch_ == NULL");
    }
}

bool DBusWatch::isReadyToBeWatched() {
    return 0 != dbus_watch_get_enabled(libdbusWatch_);
}

void DBusWatch::startWatching() {
    if(!dbus_watch_get_enabled(libdbusWatch_)) stopWatching();

    unsigned int channelFlags_ = dbus_watch_get_flags(libdbusWatch_);
    short int pollFlags = 0;

    if(channelFlags_ & DBUS_WATCH_READABLE) {
        pollFlags |= POLLIN;
    }
    if(channelFlags_ & DBUS_WATCH_WRITABLE) {
        pollFlags |= POLLOUT;
    }

#ifdef WIN32
    pollFileDescriptor_.fd = dbus_watch_get_socket(libdbusWatch_);
    wsaEvent_ = WSACreateEvent();
    WSAEventSelect(pollFileDescriptor_.fd, wsaEvent_, FD_READ);
#else
    pollFileDescriptor_.fd = dbus_watch_get_unix_fd(libdbusWatch_);
#endif

    pollFileDescriptor_.events = pollFlags;
    pollFileDescriptor_.revents = 0;

    auto lockedContext = mainLoopContext_.lock();
    if (NULL == lockedContext) {
        COMMONAPI_ERROR(std::string(__FUNCTION__) + " lockedContext == NULL");
    } else {
        lockedContext->registerWatch(this);
    }
}

void DBusWatch::stopWatching() {
    auto lockedContext = mainLoopContext_.lock();
    if (lockedContext) {
        lockedContext->deregisterWatch(this);
    }
}

const pollfd& DBusWatch::getAssociatedFileDescriptor() {
    return pollFileDescriptor_;
}

#ifdef WIN32
const HANDLE& DBusWatch::getAssociatedEvent() {
    return wsaEvent_;
}
#endif

void DBusWatch::dispatch(unsigned int eventFlags) {
#ifdef WIN32
    unsigned int dbusWatchFlags = 0;

    if (eventFlags & (POLLRDBAND | POLLRDNORM)) {
        dbusWatchFlags |= DBUS_WATCH_READABLE;
    }
    if (eventFlags & POLLWRNORM) {
        dbusWatchFlags |= DBUS_WATCH_WRITABLE;
    }
    if (eventFlags & (POLLERR | POLLNVAL)) {
        dbusWatchFlags |= DBUS_WATCH_ERROR;
    }
    if (eventFlags & POLLHUP) {
        dbusWatchFlags |= DBUS_WATCH_HANGUP;
    }
#else
    // Pollflags do not correspond directly to DBus watch flags
    unsigned int dbusWatchFlags = (eventFlags & POLLIN) |
                            ((eventFlags & POLLOUT) >> 1) |
                            ((eventFlags & POLLERR) >> 1) |
                            ((eventFlags & POLLHUP) >> 1);
#endif
    std::shared_ptr<DBusConnection> itsConnection = dbusConnection_.lock();
    if(itsConnection) {
        if(itsConnection->setDispatching(true)) {
            dbus_bool_t response = dbus_watch_handle(libdbusWatch_, dbusWatchFlags);
            if (!response) {
                printf("dbus_watch_handle returned FALSE!");
            }
            itsConnection->setDispatching(false);
        }
    }
}

const std::vector<DispatchSource*>& DBusWatch::getDependentDispatchSources() {
    return dependentDispatchSources_;
}

void DBusWatch::addDependentDispatchSource(DispatchSource* dispatchSource) {
    dependentDispatchSources_.push_back(dispatchSource);
}

DBusQueueWatch::DBusQueueWatch(std::shared_ptr<DBusConnection> _connection) : pipeValue_(4) {
#ifdef WIN32
    std::string pipeName = "\\\\.\\pipe\\CommonAPI-DBus-";

    UUID uuid;
    CHAR* uuidString = NULL;
    UuidCreate(&uuid);
    UuidToString(&uuid, (RPC_CSTR*)&uuidString);
    pipeName += uuidString;
    RpcStringFree((RPC_CSTR*)&uuidString);

    HANDLE hPipe = ::CreateNamedPipe(
        pipeName.c_str(),
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
        1,
        4096,
        4096,
        100,
        nullptr);

    if (hPipe == INVALID_HANDLE_VALUE) {
        if (GetLastError() != ERROR_PIPE_BUSY)
        {
            printf("Could not open pipe %d\n", GetLastError());
        }

        // All pipe instances are busy, so wait for sometime.
        else if (!WaitNamedPipe(pipeName.c_str(), NMPWAIT_USE_DEFAULT_WAIT))
        {
            printf("Could not open pipe: wait timed out.\n");
        }
    }

    HANDLE hPipe2 = CreateFile(
        pipeName.c_str(),   // pipe name
        GENERIC_READ |  // read and write access
        GENERIC_WRITE,
        0,              // no sharing
        NULL,           // default security attributes
        OPEN_EXISTING,  // opens existing pipe
        0,              // default attributes
        NULL);          // no template file

    if (hPipe2 == INVALID_HANDLE_VALUE) {
        if (GetLastError() != ERROR_PIPE_BUSY)
        {
            printf("Could not open pipe2 %d\n", GetLastError());
        }

        // All pipe instances are busy, so wait for sometime.
        else if (!WaitNamedPipe(pipeName.c_str(), NMPWAIT_USE_DEFAULT_WAIT))
        {
            printf("Could not open pipe2: wait timed out.\n");
        }
    }

    pipeFileDescriptors_[0] = (int)hPipe;
    pipeFileDescriptors_[1] = (int)hPipe2;

    wsaEvent_ = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);

    if (wsaEvent_ == WSA_INVALID_EVENT) {
        printf("Invalid Event Created!\n");
    }

    ov = { 0 };
    ov.hEvent = wsaEvent_;

    BOOL retVal = ::ConnectNamedPipe(hPipe, &ov);

    if (retVal == 0) {
        int error = GetLastError();

        if (error != 535) {
            printf("ERROR: ConnectNamedPipe failed with (%d)\n", error);
        }
    }
#else
    if(pipe2(pipeFileDescriptors_, O_NONBLOCK) == -1) {
        std::perror(__func__);
    }
#endif
    pollFileDescriptor_.fd = pipeFileDescriptors_[0];
    pollFileDescriptor_.events = POLLIN;

    connection_ = _connection;
}

DBusQueueWatch::~DBusQueueWatch() {
#ifdef WIN32
    BOOL retVal = DisconnectNamedPipe((HANDLE)pipeFileDescriptors_[0]);

    if (!retVal) {
        printf(TEXT("DisconnectNamedPipe failed. GLE=%d\n"), GetLastError());
    }

    retVal = CloseHandle((HANDLE)pipeFileDescriptors_[0]);

    if (!retVal) {
        printf(TEXT("CloseHandle failed. GLE=%d\n"), GetLastError());
    }

    retVal = CloseHandle((HANDLE)pipeFileDescriptors_[1]);

    if (!retVal) {
        printf(TEXT("CloseHandle2 failed. GLE=%d\n"), GetLastError());
    }
#else
    close(pipeFileDescriptors_[0]);
    close(pipeFileDescriptors_[1]);
#endif

    std::unique_lock<std::mutex> itsLock(queueMutex_);
    while(!queue_.empty()) {
        auto queueEntry = queue_.front();
        queue_.pop();
        queueEntry->clear();
    }
}

void DBusQueueWatch::dispatch(unsigned int) {
}

const pollfd& DBusQueueWatch::getAssociatedFileDescriptor() {
    return pollFileDescriptor_;
}

#ifdef WIN32
const HANDLE& DBusQueueWatch::getAssociatedEvent() {
    return wsaEvent_;
}
#endif

const std::vector<DispatchSource*>& DBusQueueWatch::getDependentDispatchSources() {
    return dependentDispatchSources_;
}

void DBusQueueWatch::addDependentDispatchSource(CommonAPI::DispatchSource* _dispatchSource) {
    dependentDispatchSources_.push_back(_dispatchSource);
}

void DBusQueueWatch::removeDependentDispatchSource(CommonAPI::DispatchSource* _dispatchSource) {
    std::vector<CommonAPI::DispatchSource*>::iterator it;

    for (it = dependentDispatchSources_.begin(); it != dependentDispatchSources_.end(); it++) {
        if ( (*it) == _dispatchSource ) {
            dependentDispatchSources_.erase(it);
            break;
        }
    }
}

void DBusQueueWatch::pushQueue(std::shared_ptr<QueueEntry> _queueEntry) {
    std::unique_lock<std::mutex> itsLock(queueMutex_);
    queue_.push(_queueEntry);

#ifdef WIN32
    char writeValue[sizeof(pipeValue_)];
    *reinterpret_cast<int*>(writeValue) = pipeValue_;
    DWORD cbWritten;

    int fSuccess = WriteFile(
        (HANDLE)pipeFileDescriptors_[1],                  // pipe handle
        writeValue,             // message
        sizeof(pipeValue_),              // message length
        &cbWritten,             // bytes written
        &ov);                  // overlapped

    if (!fSuccess)
    {
        printf(TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError());
    }
#else
    if(write(pipeFileDescriptors_[1], &pipeValue_, sizeof(pipeValue_)) == -1) {
        std::perror(__func__);
    }
#endif
}

void DBusQueueWatch::popQueue() {
    std::unique_lock<std::mutex> itsLock(queueMutex_);

#ifdef WIN32
    char readValue[sizeof(pipeValue_)];
    DWORD cbRead;

    int fSuccess = ReadFile(
        (HANDLE)pipeFileDescriptors_[0],    // pipe handle
        readValue,    // buffer to receive reply
        sizeof(pipeValue_),  // size of buffer
        &cbRead,  // number of bytes read
        &ov);    // overlapped

    if (!fSuccess)
    {
        printf(TEXT("ReadFile to pipe failed. GLE=%d\n"), GetLastError());
    }
#else
    int readValue = 0;
    if(read(pipeFileDescriptors_[0], &readValue, sizeof(readValue)) == -1) {
        std::perror(__func__);
    }
#endif

    queue_.pop();
}

std::shared_ptr<QueueEntry> DBusQueueWatch::frontQueue() {
    std::unique_lock<std::mutex> itsLock(queueMutex_);

    return queue_.front();
}

bool DBusQueueWatch::emptyQueue() {
    std::unique_lock<std::mutex> itsLock(queueMutex_);

    return queue_.empty();
}

void DBusQueueWatch::processQueueEntry(std::shared_ptr<QueueEntry> _queueEntry) {
    std::shared_ptr<DBusConnection> itsConnection = connection_.lock();
    if(itsConnection) {
        _queueEntry->process(itsConnection);
    }
}

#ifdef WIN32
__declspec(thread) DBusTimeout* DBusTimeout::currentTimeout_ = NULL;
#else
thread_local DBusTimeout* DBusTimeout::currentTimeout_ = NULL;
#endif

DBusTimeout::DBusTimeout(::DBusTimeout* libdbusTimeout, std::weak_ptr<MainLoopContext>& mainLoopContext,
                         std::weak_ptr<DBusConnection>& dbusConnection) :
                dueTimeInMs_(TIMEOUT_INFINITE),
                libdbusTimeout_(libdbusTimeout),
                mainLoopContext_(mainLoopContext),
                dbusConnection_(dbusConnection),
                pendingCall_(NULL) {
    currentTimeout_ = this;
}

bool DBusTimeout::isReadyToBeMonitored() {
    return 0 != dbus_timeout_get_enabled(libdbusTimeout_);
}

void DBusTimeout::startMonitoring() {
    auto lockedContext = mainLoopContext_.lock();
    if (NULL == lockedContext) {
        COMMONAPI_ERROR(std::string(__FUNCTION__) + " lockedContext == NULL");
    } else {
        recalculateDueTime();
        lockedContext->registerTimeoutSource(this);
    }
}

void DBusTimeout::stopMonitoring() {
    dueTimeInMs_ = TIMEOUT_INFINITE;
    auto lockedContext = mainLoopContext_.lock();
    if (lockedContext) {
        lockedContext->deregisterTimeoutSource(this);
    }
}

bool DBusTimeout::dispatch() {
    std::shared_ptr<DBusConnection> itsConnection = dbusConnection_.lock();
    if(itsConnection) {
        if(itsConnection->setDispatching(true)) {
            recalculateDueTime();
            itsConnection->setPendingCallTimedOut(pendingCall_, libdbusTimeout_);
            itsConnection->setDispatching(false);
            return true;
        }
    }
    return false;
}

int64_t DBusTimeout::getTimeoutInterval() const {
    return dbus_timeout_get_interval(libdbusTimeout_);
}

int64_t DBusTimeout::getReadyTime() const {
    return dueTimeInMs_;
}

void DBusTimeout::recalculateDueTime() {
    if(dbus_timeout_get_enabled(libdbusTimeout_)) {
        int intervalInMs = dbus_timeout_get_interval(libdbusTimeout_);
        dueTimeInMs_ = getCurrentTimeInMs() + intervalInMs;
    } else {
        dueTimeInMs_ = TIMEOUT_INFINITE;
    }
}

void DBusTimeout::setPendingCall(DBusPendingCall* _pendingCall) {
    pendingCall_ = _pendingCall;
}

} // namespace DBus
} // namespace CommonAPI
