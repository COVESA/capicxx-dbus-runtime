// Copyright (C) 2013-2017 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstring>

#include <CommonAPI/Logger.hpp>
#include <CommonAPI/DBus/DBusAddress.hpp>
#include <CommonAPI/DBus/DBusMessage.hpp>

namespace CommonAPI {
namespace DBus {

DBusMessage::DBusMessage()
    : message_(NULL) {
}

DBusMessage::DBusMessage(::DBusMessage *_message) {
    message_ = (_message != nullptr ? dbus_message_ref(_message) : nullptr);
}

DBusMessage::DBusMessage(::DBusMessage *_message, bool reference) {
    if (NULL == _message) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " NULL _message");
    }
    message_ = (_message != nullptr ? (reference ? dbus_message_ref(message_) : _message) : nullptr);
}

DBusMessage::DBusMessage(const DBusMessage &_source) {
    message_ = (_source.message_ != nullptr ?
                    dbus_message_ref(_source.message_) : nullptr);
}

DBusMessage::DBusMessage(DBusMessage &&_source) {
    message_ = _source.message_;
    _source.message_ = nullptr;
}

DBusMessage::~DBusMessage() {
    if (message_)
        dbus_message_unref(message_);
}

DBusMessage &
DBusMessage::operator=(const DBusMessage &_source) {
    if (this != &_source) {
        if (message_)
            dbus_message_unref(message_);

        message_ = (_source.message_ != nullptr ?
                        dbus_message_ref(_source.message_) : nullptr);
    }
    return (*this);
}

DBusMessage &
DBusMessage::operator=(DBusMessage &&_source) {
    if (this != &_source) {
        if (message_)
            dbus_message_unref(message_);

        message_ = _source.message_;
        _source.message_ = NULL;
    }
    return (*this);
}

DBusMessage::operator bool() const {
    return (nullptr != message_);
}

DBusMessage
DBusMessage::createOrgFreedesktopOrgMethodCall(
    const std::string &_method, const std::string &_signature) {

    static DBusAddress address("org.freedesktop.DBus", "/", "org.freedesktop.DBus");
    return DBusMessage::createMethodCall(address, _method, _signature);
}

DBusMessage
DBusMessage::createMethodCall(
    const DBusAddress &_address,
    const std::string &_method, const std::string &_signature) {

    std::string service = _address.getService();
    std::string path = _address.getObjectPath();
    std::string interface = _address.getInterface();

    ::DBusMessage *methodCall = dbus_message_new_method_call(
                                    service.c_str(), path.c_str(),
                                    interface.c_str(), _method.c_str());
    if (NULL == methodCall) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " dbus_message_new_method_call() returned NULL");
    }

    if ("" != _signature)
        dbus_message_set_signature(methodCall, _signature.c_str());

    return DBusMessage(methodCall, false);
}

DBusMessage
DBusMessage::createMethodReturn(const std::string &_signature) const {
    ::DBusMessage *methodReturn = dbus_message_new_method_return(message_);
    if (NULL == methodReturn) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " dbus_message_new_method_return() returned NULL");
    }

    if ("" != _signature)
       dbus_message_set_signature(methodReturn, _signature.c_str());

    return DBusMessage(methodReturn, false);
}

DBusMessage
DBusMessage::createMethodError(
    const std::string &_code, const std::string &_signature, const std::string &_info) const {

    ::DBusMessage *methodError
          = dbus_message_new_error(message_, _code.c_str(), _info.c_str());
    if (NULL == methodError) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " dbus_message_new_error() returned NULL");
    }

    dbus_message_set_signature(methodError, _signature.c_str());

    return DBusMessage(methodError, false);
}

DBusMessage
DBusMessage::createSignal(
    const std::string &_path, const std::string &_interface,
    const std::string &_signal, const std::string &_signature) {

    ::DBusMessage *messageSignal
          = dbus_message_new_signal(_path.c_str(), _interface.c_str(), _signal.c_str());
    if (NULL == messageSignal) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " dbus_message_new_signal() returned NULL");
    }

    if ("" != _signature)
        dbus_message_set_signature(messageSignal, _signature.c_str());

    return DBusMessage(messageSignal, false);
}

const char *
DBusMessage::getObjectPath() const {
    return dbus_message_get_path(message_);
}

const char *
DBusMessage::getSender() const {
    return dbus_message_get_sender(message_);
}

const char *
DBusMessage::getInterface() const {
    return dbus_message_get_interface(message_);
}

const char *
DBusMessage::getMember() const {
    return dbus_message_get_member(message_);
}

const char *
DBusMessage::getSignature() const {
    return dbus_message_get_signature(message_);
}

const char *
DBusMessage::getError() const {
    if (!isErrorType()) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " !isErrorType");
    }
    return dbus_message_get_error_name(message_);
}

const char *
DBusMessage::getDestination() const {
    return dbus_message_get_destination(message_);
}

uint32_t DBusMessage::getSerial() const {
    return dbus_message_get_serial(message_);
}

bool
DBusMessage::hasObjectPath(const char *_path) const {
    const char *path = getObjectPath();
    if (NULL == _path) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " _path == NULL");
    }
    if (NULL == path) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " path == NULL");
    }

    return (((NULL != path) && (NULL != _path))? !strcmp(path, _path) : false);
}

bool DBusMessage::hasInterfaceName(const char *_interface) const {
    const char *interface = getInterface();

    if (NULL == _interface) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " _interface == NULL");
    }
    if (NULL == interface) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " interface == NULL");
    }

    return (((NULL != interface) && (NULL != _interface))? !strcmp(interface, _interface) : false);
}

bool DBusMessage::hasMemberName(const char *_member) const {
    const char *member = getMember();

    if (NULL == _member) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " _member == NULL");
    }
    if (NULL == member) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " member == NULL");
    }

    return (((NULL != member) && (NULL != _member))? !strcmp(member, _member) : false);
}

bool DBusMessage::hasSignature(const char *_signature) const {
    const char *signature = getSignature();

    if (NULL == _signature) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " _signature == NULL");
    }
    if (NULL == signature) {
        COMMONAPI_ERROR(std::string(__FUNCTION__), " signature == NULL");
    }

    return (((NULL != signature) && (NULL != _signature))? !strcmp(signature, _signature) : false);
}

DBusMessage::Type DBusMessage::getType() const {
    return static_cast<Type>(dbus_message_get_type(message_));
}

char * DBusMessage::getBodyData() const {
    return dbus_message_get_body(message_);
}

int DBusMessage::getBodyLength() const {
    return dbus_message_get_body_length(message_);
}

int DBusMessage::getBodySize() const {
    return dbus_message_get_body_allocated(message_);
}

bool DBusMessage::setBodyLength(const int _length) {
    return 0 != dbus_message_set_body_length(message_, _length);
}

bool DBusMessage::setDestination(const char *_destination)
{
    return 0 != dbus_message_set_destination(message_, _destination);
}

void DBusMessage::setSerial(const unsigned int _serial) const {
    dbus_message_set_serial(message_, _serial);
}

bool DBusMessage::hasObjectPath(const std::string &_path) const {
    return hasObjectPath(_path.c_str());
}

bool DBusMessage::isInvalidType() const {
    return (getType() == Type::Invalid);
}

bool DBusMessage::isMethodCallType() const {
    return (getType() == Type::MethodCall);
}

bool DBusMessage::isMethodReturnType() const {
    return (getType() == Type::MethodReturn);
}

bool DBusMessage::isErrorType() const {
    return (getType() == Type::Error);
}

bool DBusMessage::isSignalType() const {
    return (getType() == Type::Signal);
}

} // namespace DBus
} // namespace CommonAPI
