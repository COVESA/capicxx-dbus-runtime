/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_MESSAGE_H_
#define COMMONAPI_DBUS_DBUS_MESSAGE_H_

#include <string>

#include <dbus/dbus.h>

namespace CommonAPI {
namespace DBus {

class DBusConnection;

class DBusMessage {
 public:
    DBusMessage();
    DBusMessage(::DBusMessage* libdbusMessage);
    DBusMessage(::DBusMessage* libdbusMessage, bool increaseReferenceCount);
    DBusMessage(const DBusMessage& src);
    DBusMessage(DBusMessage&& src);

    ~DBusMessage();

    DBusMessage& operator=(const DBusMessage& src);
    DBusMessage& operator=(DBusMessage&& rsrc);
    operator bool() const;

    static DBusMessage createOrgFreedesktopOrgMethodCall(const char* methodName,
                                                         const char* signature = NULL);

    static DBusMessage createOrgFreedesktopOrgMethodCall(const std::string& methodName,
                                                         const std::string& signature = "");

    static DBusMessage createMethodCall(const char* busName,
                                        const char* objectPath,
                                        const char* interfaceName,
                                        const char* methodName,
                                        const char* signature = NULL);

    static DBusMessage createMethodCall(const std::string& busName,
                                        const std::string& objectPath,
                                        const std::string& interfaceName,
                                        const std::string& methodName,
                                        const std::string& signature = "");

    DBusMessage createMethodReturn(const char* signature = NULL) const;

    DBusMessage createMethodReturn(const std::string& signature) const;

    DBusMessage createMethodError(const std::string& name, const std::string& reason = "") const;

    static DBusMessage createSignal(const char* objectPath,
                                    const char* interfaceName,
                                    const char* signalName,
                                    const char* signature = NULL);

    static DBusMessage createSignal(const std::string& objectPath,
                                    const std::string& interfaceName,
                                    const std::string& signalName,
                                    const std::string& signature = "");

    const char* getSenderName() const;
    const char* getObjectPath() const;
    const char* getInterfaceName() const;
    const char* getMemberName() const;
    const char* getSignatureString() const;
    const char* getErrorName() const;
    const char* getDestination() const;

    inline bool hasObjectPath(const std::string& objectPath) const;

    bool hasObjectPath(const char* objectPath) const;
    bool hasInterfaceName(const char* interfaceName) const;
    bool hasMemberName(const char* memberName) const;
    bool hasSignature(const char* signature) const;

    enum class Type: int {
        Invalid = DBUS_MESSAGE_TYPE_INVALID,
        MethodCall = DBUS_MESSAGE_TYPE_METHOD_CALL,
        MethodReturn = DBUS_MESSAGE_TYPE_METHOD_RETURN,
        Error = DBUS_MESSAGE_TYPE_ERROR,
        Signal = DBUS_MESSAGE_TYPE_SIGNAL
    };
    const Type getType() const;
    inline bool isInvalidType() const;
    inline bool isMethodCallType() const;
    inline bool isMethodReturnType() const;
    inline bool isErrorType() const;
    inline bool isSignalType() const;

    char* getBodyData() const;
    int getBodyLength() const;
    int getBodySize() const;

    bool setBodyLength(const int bodyLength);
    bool setDestination(const char* destination);

 private:
    ::DBusMessage* libdbusMessage_;

    friend class DBusConnection;
};

bool DBusMessage::hasObjectPath(const std::string& objectPath) const {
    return hasObjectPath(objectPath.c_str());
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

#endif // COMMONAPI_DBUS_DBUS_MESSAGE_H_
