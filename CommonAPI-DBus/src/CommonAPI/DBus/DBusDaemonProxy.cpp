/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusDaemonProxy.h"
#include "DBusProxyHelper.h"
#include <iostream>


namespace CommonAPI {
namespace DBus {

DBusDaemonProxy::DBusDaemonProxy(const std::shared_ptr<DBusProxyConnection>& connection):
                DBusProxy("org.freedesktop.DBus", "/org/freedesktop/DBus", getInterfaceName(), connection, true),
                nameOwnerChangedEvent_(*this, "NameOwnerChanged", "sss") {
}

const char* DBusDaemonProxy::getInterfaceName() const {
	return "org.freedesktop.DBus";
}

DBusDaemonProxy::NameOwnerChangedEvent& DBusDaemonProxy::getNameOwnerChangedEvent() {
    return nameOwnerChangedEvent_;
}

void DBusDaemonProxy::listNames(CommonAPI::CallStatus& callStatus, std::vector<std::string>& busNames) const {
    DBusMessage dbusMethodCall = createMethodCall("ListNames", "");

    DBusError dbusError;
    DBusMessage dbusMessageReply = getDBusConnection()->sendDBusMessageWithReplyAndBlock(
                    dbusMethodCall,
                    dbusError);

    if (dbusError || !dbusMessageReply.isMethodReturnType()) {
        callStatus = CallStatus::REMOTE_ERROR;
        return;
    }

    DBusInputStream inputStream(dbusMessageReply);
    const bool success = DBusSerializableArguments<std::vector<std::string>>::deserialize(inputStream, busNames);
    if (!success) {
        callStatus = CallStatus::REMOTE_ERROR;
        return;
    }
    callStatus = CallStatus::SUCCESS;

}

std::future<CallStatus> DBusDaemonProxy::listNamesAsync(ListNamesAsyncCallback listNamesAsyncCallback) const {
    DBusMessage dbusMessage = createMethodCall("ListNames", "");

    return getDBusConnection()->sendDBusMessageWithReplyAsync(
                    dbusMessage,
                    DBusProxyAsyncCallbackHandler<std::vector<std::string>>::create(listNamesAsyncCallback));
}

void DBusDaemonProxy::nameHasOwner(const std::string& busName, CommonAPI::CallStatus& callStatus, bool& hasOwner) const {

    DBusMessage dbusMethodCall = createMethodCall("NameHasOwner", "s");

    DBusOutputStream outputStream(dbusMethodCall);
    bool success = DBusSerializableArguments<std::string>::serialize(outputStream, busName);
    if (!success) {
        callStatus = CallStatus::OUT_OF_MEMORY;
        return;
    }
    outputStream.flush();

    DBusError dbusError;
    DBusMessage dbusMessageReply = getDBusConnection()->sendDBusMessageWithReplyAndBlock(
                    dbusMethodCall,
                    dbusError);
    if (dbusError || !dbusMessageReply.isMethodReturnType()) {
        callStatus = CallStatus::REMOTE_ERROR;
        return;
    }

    DBusInputStream inputStream(dbusMessageReply);
    success = DBusSerializableArguments<bool>::deserialize(inputStream, hasOwner);
    if (!success) {
        callStatus = CallStatus::REMOTE_ERROR;
        return;
    }
    callStatus = CallStatus::SUCCESS;

}

std::future<CallStatus> DBusDaemonProxy::nameHasOwnerAsync(const std::string& busName, NameHasOwnerAsyncCallback nameHasOwnerAsyncCallback) const {

    DBusMessage dbusMessage = createMethodCall("NameHasOwner", "s");

    DBusOutputStream outputStream(dbusMessage);
    const bool success = DBusSerializableArguments<std::string>::serialize(outputStream, busName);
    if (!success) {
        std::promise<CallStatus> promise;
        promise.set_value(CallStatus::OUT_OF_MEMORY);
        return promise.get_future();
    }
    outputStream.flush();

    return getDBusConnection()->sendDBusMessageWithReplyAsync(
                    dbusMessage,
                    DBusProxyAsyncCallbackHandler<bool>::create(nameHasOwnerAsyncCallback));

}

void DBusDaemonProxy::getOwnVersion(uint16_t& ownVersionMajor, uint16_t& ownVersionMinor) const {
}

} // namespace DBus
} // namespace CommonAPI
