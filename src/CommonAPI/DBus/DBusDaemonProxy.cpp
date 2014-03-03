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

void StaticInterfaceVersionAttribute::getValue(CallStatus& callStatus, Version& version) const {
    version = version_;
    callStatus = CallStatus::SUCCESS;
}

std::future<CallStatus> StaticInterfaceVersionAttribute::getValueAsync(AttributeAsyncCallback attributeAsyncCallback) {
    attributeAsyncCallback(CallStatus::SUCCESS, version_);

    std::promise<CallStatus> versionPromise;
    versionPromise.set_value(CallStatus::SUCCESS);

    return versionPromise.get_future();
}


static const std::string dbusDaemonBusName_ = "org.freedesktop.DBus";
static const std::string dbusDaemonObjectPath_ = "/org/freedesktop/DBus";
static const std::string dbusDaemonInterfaceName_ = DBusDaemonProxy::getInterfaceId();
static const std::string commonApiParticipantId_ = "org.freedesktop.DBus-/org/freedesktop/DBus";


DBusDaemonProxy::DBusDaemonProxy(const std::shared_ptr<DBusProxyConnection>& dbusConnection):
                DBusProxyBase(dbusConnection),
                nameOwnerChangedEvent_(*this, "NameOwnerChanged", "sss"),
                interfaceVersionAttribute_(1, 0) {
}

void DBusDaemonProxy::init() {

}

std::string DBusDaemonProxy::getAddress() const {
    return getDomain() + ":" + getServiceId() + ":" + getInstanceId();
}
const std::string& DBusDaemonProxy::getDomain() const {
    return commonApiDomain_;
}
const std::string& DBusDaemonProxy::getServiceId() const {
    return dbusDaemonInterfaceName_;
}
const std::string& DBusDaemonProxy::getInstanceId() const {
    return commonApiParticipantId_;
}

const std::string& DBusDaemonProxy::getDBusBusName() const {
    return dbusDaemonBusName_;
}
const std::string& DBusDaemonProxy::getDBusObjectPath() const {
    return dbusDaemonObjectPath_;
}
const std::string& DBusDaemonProxy::getInterfaceName() const {
    return dbusDaemonInterfaceName_;
}

bool DBusDaemonProxy::isAvailable() const {
    return getDBusConnection()->isConnected();
}

bool DBusDaemonProxy::isAvailableBlocking() const {
    return isAvailable();
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
                    DBusProxyAsyncCallbackHandler<std::vector<std::string>>::create(listNamesAsyncCallback),
                    2000);
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
                    DBusProxyAsyncCallbackHandler<bool>::create(nameHasOwnerAsyncCallback),
                    2000);
}

std::future<CallStatus> DBusDaemonProxy::getManagedObjectsAsync(const std::string& forDBusServiceName, GetManagedObjectsAsyncCallback callback) const {
    // resolve remote objects
    auto dbusMethodCallMessage = DBusMessage::createMethodCall(
                    forDBusServiceName,
                    "/",
                    "org.freedesktop.DBus.ObjectManager",
                    "GetManagedObjects",
                    "");

    return getDBusConnection()->sendDBusMessageWithReplyAsync(
                    dbusMethodCallMessage,
                    DBusProxyAsyncCallbackHandler<DBusObjectToInterfaceDict>::create(callback),
                    2000);
}

std::future<CallStatus> DBusDaemonProxy::getNameOwnerAsync(const std::string& busName, GetNameOwnerAsyncCallback getNameOwnerAsyncCallback) const {
    DBusMessage dbusMessage = createMethodCall("GetNameOwner", "s");

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
                    DBusProxyAsyncCallbackHandler<std::string>::create(getNameOwnerAsyncCallback),
                    2000);
}

const char* DBusDaemonProxy::getInterfaceId() {
    static const char interfaceId[] = "org.freedesktop.DBus";
    return interfaceId;
}

} // namespace DBus
} // namespace CommonAPI
