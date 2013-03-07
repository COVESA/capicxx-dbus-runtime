/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusDaemonProxy.h"
#include "DBusProxyHelper.h"


namespace CommonAPI {
namespace DBus {

StaticInterfaceVersionAttribute::StaticInterfaceVersionAttribute(const uint32_t& majorValue, const uint32_t& minorValue):
                version_(majorValue, minorValue) {
}

CallStatus StaticInterfaceVersionAttribute::getValue(Version& version) const {
    version = version_;

    return CallStatus::SUCCESS;
}

std::future<CallStatus> StaticInterfaceVersionAttribute::getValueAsync(AttributeAsyncCallback attributeAsyncCallback) {
    attributeAsyncCallback(CallStatus::SUCCESS, version_);

    std::promise<CallStatus> versionPromise;
    versionPromise.set_value(CallStatus::SUCCESS);

    return versionPromise.get_future();
}


StaticInterfaceVersionAttribute DBusDaemonProxy::interfaceVersionAttribute_(1, 0);

DBusDaemonProxy::DBusDaemonProxy(const std::shared_ptr<DBusProxyConnection>& dbusConnection):
                DBusProxyBase(getInterfaceId(), "org.freedesktop.DBus", "/org/freedesktop/DBus", dbusConnection),
                nameOwnerChangedEvent_(*this, "NameOwnerChanged", "sss") {
}

bool DBusDaemonProxy::isAvailable() const {
    return getDBusConnection()->isConnected();
}

ProxyStatusEvent& DBusDaemonProxy::getProxyStatusEvent() {
    return getDBusConnection()->getConnectionStatusEvent();
}

InterfaceVersionAttribute& DBusDaemonProxy::getInterfaceVersionAttribute() {
    return interfaceVersionAttribute_;
}

DBusDaemonProxy::NameOwnerChangedEvent& DBusDaemonProxy::getNameOwnerChangedEvent() {
    return nameOwnerChangedEvent_;
}

void DBusDaemonProxy::listNames(CommonAPI::CallStatus& callStatus, std::vector<std::string>& busNames) const {
    DBusMessage dbusMethodCall = createMethodCall("ListNames", "");

    DBusError dbusError;
    DBusMessage dbusMessageReply = getDBusConnection()->sendDBusMessageWithReplyAndBlock(dbusMethodCall, dbusError);

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

std::future<CallStatus> DBusDaemonProxy::getManagedObjectsAsync(const std::string& forDBusServiceName, GetManagedObjectsAsyncCallback callback) const {
    // resolve remote objects
    auto dbusMethodCallMessage = DBusMessage::createMethodCall(
                    forDBusServiceName,
                    "/",
                    "org.freedesktop.DBus.ObjectManager",
                    "GetManagedObjects",
                    "");

    const int timeoutMilliseconds = 100;

    return getDBusConnection()->sendDBusMessageWithReplyAsync(
                    dbusMethodCallMessage,
                    DBusProxyAsyncCallbackHandler<DBusObjectToInterfaceDict>::create(callback),
                    timeoutMilliseconds);
}


} // namespace DBus
} // namespace CommonAPI
