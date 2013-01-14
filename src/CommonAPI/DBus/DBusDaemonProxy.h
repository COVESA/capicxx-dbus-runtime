/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_DBUS_DBUS_DAEMON_PROXY_H_
#define COMMONAPI_DBUS_DBUS_DAEMON_PROXY_H_

#include "DBusProxy.h"
#include "DBusEvent.h"

#include "DBusServiceStatusEvent.h"

#include <string>
#include <vector>

namespace CommonAPI {
namespace DBus {

class DBusServiceStatusEvent;

class DBusDaemonProxy: public DBusProxy {

 public:
	typedef Event<std::string, std::string, std::string> NameOwnerChangedEvent;
	typedef std::function<void(const CommonAPI::CallStatus&, std::vector<std::string>)> ListNamesAsyncCallback;
	typedef std::function<void(const CommonAPI::CallStatus&, bool)> NameHasOwnerAsyncCallback;


	DBusDaemonProxy(const std::shared_ptr<DBusProxyConnection>& connection);

    const char* getInterfaceName() const;

    NameOwnerChangedEvent& getNameOwnerChangedEvent();

    void listNames(CommonAPI::CallStatus& callStatus, std::vector<std::string>& busNames) const;
    std::future<CallStatus> listNamesAsync(ListNamesAsyncCallback listNamesAsyncCallback) const;

    void nameHasOwner(const std::string& busName, CommonAPI::CallStatus& callStatus, bool& hasOwner) const;
    std::future<CallStatus> nameHasOwnerAsync(const std::string& busName, NameHasOwnerAsyncCallback nameHasOwnerAsyncCallback) const;

 protected:
    void getOwnVersion(uint16_t& ownVersionMajor, uint16_t& ownVersionMinor) const;

 private:
    DBusEvent<NameOwnerChangedEvent> nameOwnerChangedEvent_;

};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_DAEMON_PROXY_H_
