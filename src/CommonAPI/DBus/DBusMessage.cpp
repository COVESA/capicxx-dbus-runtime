/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusMessage.h"

#include <cassert>
#include <cstring>

namespace CommonAPI {
namespace DBus {

DBusMessage::DBusMessage():
		libdbusMessage_(NULL) {
}

DBusMessage::DBusMessage(::DBusMessage* libdbusMessage) {
	libdbusMessage_ = libdbusMessage != NULL ? dbus_message_ref(libdbusMessage) : NULL;
}

DBusMessage::DBusMessage(::DBusMessage* libdbusMessage, bool increaseReferenceCount) {
	assert(libdbusMessage);

	libdbusMessage_ = increaseReferenceCount ? dbus_message_ref(libdbusMessage) : libdbusMessage;
}

DBusMessage::DBusMessage(const DBusMessage& src) {
	libdbusMessage_ = src.libdbusMessage_ != NULL ? dbus_message_ref(src.libdbusMessage_) : NULL;
}

DBusMessage::DBusMessage(DBusMessage&& rsrc) {
	libdbusMessage_ = rsrc.libdbusMessage_;
	rsrc.libdbusMessage_ = NULL;
}

DBusMessage::~DBusMessage() {
	if (libdbusMessage_)
		dbus_message_unref(libdbusMessage_);
}

DBusMessage& DBusMessage::operator=(const DBusMessage& src) {
	if (this != &src) {
		if (libdbusMessage_)
			dbus_message_unref(libdbusMessage_);

		libdbusMessage_ = src.libdbusMessage_ != NULL ? dbus_message_ref(src.libdbusMessage_) : NULL;
	}

	return *this;
}

DBusMessage& DBusMessage::operator=(DBusMessage&& rsrc) {
	if (this != &rsrc) {
		if (libdbusMessage_)
			dbus_message_unref(libdbusMessage_);

		libdbusMessage_ = rsrc.libdbusMessage_;
		rsrc.libdbusMessage_ = NULL;
	}

	return *this;
}

DBusMessage::operator bool() const {
	const bool isNotNullDBusMessage = (libdbusMessage_ != NULL);
	return isNotNullDBusMessage;
}

DBusMessage DBusMessage::createOrgFreedesktopOrgMethodCall(const char* methodName, const char* signature) {
	return DBusMessage::createMethodCall("org.freedesktop.DBus",
										 "/",
										 "org.freedesktop.DBus",
										 methodName,
										 signature);
}

DBusMessage DBusMessage::createOrgFreedesktopOrgMethodCall(const std::string& methodName,
														   const std::string& signature) {
	assert(!methodName.empty());

	return createOrgFreedesktopOrgMethodCall(methodName.c_str(),
											 signature.empty() ? NULL : signature.c_str());
}

DBusMessage DBusMessage::createMethodCall(const char* busName,
                                          const char* objectPath,
                                          const char* interfaceName,
                                          const char* methodName,
                                          const char* signature) {
	assert(busName);
	assert(objectPath);
	assert(interfaceName);
	assert(methodName);

	::DBusMessage* libdbusMessageCall = dbus_message_new_method_call(busName,
																	 objectPath,
																	 interfaceName,
																	 methodName);
	assert(libdbusMessageCall);

	if (signature)
		dbus_message_set_signature(libdbusMessageCall, signature);

	const bool increaseLibdbusMessageReferenceCount = false;
	return DBusMessage(libdbusMessageCall, increaseLibdbusMessageReferenceCount);
}

DBusMessage DBusMessage::createMethodCall(const std::string& busName,
										  const std::string& objectPath,
										  const std::string& interfaceName,
										  const std::string& methodName,
										  const std::string& signature) {
	assert(!busName.empty());
	assert(!objectPath.empty());
	assert(!interfaceName.empty());
	assert(!methodName.empty());

	return createMethodCall(busName.c_str(),
							objectPath.c_str(),
							interfaceName.c_str(),
							methodName.c_str(),
							signature.empty() ? NULL : signature.c_str());
}

DBusMessage DBusMessage::createMethodReturn(const char* signature) const {
	::DBusMessage* libdbusMessageReturn = dbus_message_new_method_return(libdbusMessage_);
	assert(libdbusMessageReturn);

	if (signature)
		dbus_message_set_signature(libdbusMessageReturn, signature);

	const bool increaseLibdbusMessageReferenceCount = false;
	return DBusMessage(libdbusMessageReturn, increaseLibdbusMessageReferenceCount);
}

DBusMessage DBusMessage::createMethodReturn(const std::string& signature) const {
	return createMethodReturn(signature.empty() ? NULL : signature.c_str());
}

DBusMessage DBusMessage::createSignal(const char* objectPath,
                                      const char* interfaceName,
                                      const char* signalName,
                                      const char* signature) {
	assert(objectPath);
	assert(interfaceName);
	assert(signalName);

	::DBusMessage* libdbusMessageSignal = dbus_message_new_signal(objectPath,
																  interfaceName,
																  signalName);
	assert(libdbusMessageSignal);

	if (signature)
		dbus_message_set_signature(libdbusMessageSignal, signature);

	const bool increaseLibdbusMessageReferenceCount = false;
	return DBusMessage(libdbusMessageSignal, increaseLibdbusMessageReferenceCount);
}

DBusMessage DBusMessage::createSignal(const std::string& objectPath,
									  const std::string& interfaceName,
									  const std::string& signalName,
									  const std::string& signature) {
	assert(!objectPath.empty());
	assert(!interfaceName.empty());
	assert(!signalName.empty());

	return createSignal(objectPath.c_str(),
						interfaceName.c_str(),
						signalName.c_str(),
						signature.empty() ? NULL : signature.c_str());
}

const char* DBusMessage::getObjectPath() const {
	return dbus_message_get_path(libdbusMessage_);
}

const char* DBusMessage::getSenderName() const {
	return dbus_message_get_sender(libdbusMessage_);
}

const char* DBusMessage::getInterfaceName() const {
	return dbus_message_get_interface(libdbusMessage_);
}

const char* DBusMessage::getMemberName() const {
	return dbus_message_get_member(libdbusMessage_);
}

const char* DBusMessage::getSignatureString() const {
	return dbus_message_get_signature(libdbusMessage_);
}

const char* DBusMessage::getErrorName() const {
	assert(isErrorType());

	return dbus_message_get_error_name(libdbusMessage_);
}

bool DBusMessage::hasObjectPath(const char* objectPath) const {
    const char* dbusMessageObjectPath = getObjectPath();

    assert(objectPath);
    assert(dbusMessageObjectPath);

    return !strcmp(dbusMessageObjectPath, objectPath);
}

bool DBusMessage::hasInterfaceName(const char* interfaceName) const {
    const char* dbusMessageInterfaceName = getInterfaceName();

    assert(interfaceName);
    assert(dbusMessageInterfaceName);

    return !strcmp(dbusMessageInterfaceName, interfaceName);
}

bool DBusMessage::hasMemberName(const char* memberName) const {
    const char* dbusMessageMemberName = getMemberName();

    assert(memberName);
    assert(dbusMessageMemberName);

    return !strcmp(dbusMessageMemberName, memberName);
}

bool DBusMessage::hasSignature(const char* signature) const {
	const char* dbusMessageSignature = getSignatureString();

	assert(signature);
	assert(dbusMessageSignature);

	return !strcmp(dbusMessageSignature, signature);
}

const DBusMessage::Type DBusMessage::getType() const {
	const int libdbusType = dbus_message_get_type(libdbusMessage_);
	return static_cast<Type>(libdbusType);
}

char* DBusMessage::getBodyData() const {
	return dbus_message_get_body(libdbusMessage_);
}

int DBusMessage::getBodyLength() const {
	return dbus_message_get_body_length(libdbusMessage_);
}

int DBusMessage::getBodySize() const {
	return dbus_message_get_body_allocated(libdbusMessage_);
}

bool DBusMessage::setBodyLength(const int bodyLength) {
	return dbus_message_set_body_length(libdbusMessage_, bodyLength);
}

} // namespace DBus
} // namespace CommonAPI
