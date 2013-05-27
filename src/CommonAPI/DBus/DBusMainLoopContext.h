/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DBUS_MAINLOOPCONTEXT_H_
#define DBUS_MAINLOOPCONTEXT_H_

#include <list>
#include <memory>
#include <poll.h>

#include <dbus/dbus.h>

#include <CommonAPI/MainLoopContext.h>


namespace CommonAPI {
namespace DBus {

class DBusConnection;



class DBusDispatchSource: public DispatchSource {
 public:
    DBusDispatchSource(DBusConnection* dbusConnection);
    ~DBusDispatchSource();

    bool prepare(int64_t& timeout);
    bool check();
    bool dispatch();

 private:
    DBusConnection* dbusConnection_;
};

class DBusWatch: public Watch {
 public:
    DBusWatch(::DBusWatch* libdbusWatch, std::weak_ptr<MainLoopContext>& mainLoopContext);

    bool isReadyToBeWatched();
    void startWatching();
    void stopWatching();

    void dispatch(unsigned int eventFlags);

    const pollfd& getAssociatedFileDescriptor();

    const std::vector<DispatchSource*>& getDependentDispatchSources();
    void addDependentDispatchSource(DispatchSource* dispatchSource);

 private:
    bool isReady();

    ::DBusWatch* libdbusWatch_;
    pollfd pollFileDescriptor_;
    std::vector<DispatchSource*> dependentDispatchSources_;

    std::weak_ptr<MainLoopContext> mainLoopContext_;

    //XXX Necessary? Are they not covered by the fd-events?
    unsigned int channelFlags_;
};


class DBusTimeout: public Timeout {
 public:
    DBusTimeout(::DBusTimeout* libdbusTimeout, std::weak_ptr<MainLoopContext>& mainLoopContext);

    bool isReadyToBeMonitored();
    void startMonitoring();
    void stopMonitoring();

    bool dispatch();

    int64_t getTimeoutInterval() const;
    int64_t getReadyTime() const;

 private:
    void recalculateDueTime();

    int64_t dueTimeInMs_;
    ::DBusTimeout* libdbusTimeout_;
    std::weak_ptr<MainLoopContext> mainLoopContext_;
};


} // namespace DBus
} // namespace CommonAPI

#endif
