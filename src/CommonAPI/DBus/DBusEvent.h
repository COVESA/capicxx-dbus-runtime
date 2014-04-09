/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_EVENT_H_
#define COMMONAPI_DBUS_DBUS_EVENT_H_

#include "DBusProxyConnection.h"
#include "DBusMessage.h"
#include "DBusSerializableArguments.h"
#include "DBusHelper.h"

#include <CommonAPI/Event.h>

namespace CommonAPI {
namespace DBus {

class DBusProxyBase;


template <typename _EventType, typename _DBusProxy = DBusProxyBase>
class DBusEvent: public _EventType, public DBusProxyConnection::DBusSignalHandler {
 public:
    typedef typename _EventType::ArgumentsTuple ArgumentsTuple;
    typedef typename _EventType::CancellableListener CancellableListener;


    DBusEvent(_DBusProxy& dbusProxy, const char* eventName, const char* eventSignature):
            dbusProxy_(dbusProxy),
            eventName_(eventName),
            eventSignature_(eventSignature) {
        interfaceName_ = dbusProxy.getInterfaceName().c_str();
        objectPath_ = dbusProxy_.getDBusObjectPath().c_str();
        assert(eventName_);
        assert(eventSignature_);
        assert(objectPath_);
        assert(interfaceName_);
    }

    DBusEvent(_DBusProxy& dbusProxy, const char* eventName, const char* eventSignature, const char* objPath, const char* interfaceName) :
                    dbusProxy_(dbusProxy),
                    eventName_(eventName),
                    eventSignature_(eventSignature),
                    objectPath_(objPath),
                    interfaceName_(interfaceName) {
        assert(eventName);
        assert(eventSignature);
        assert(objPath);
        assert(interfaceName);
    }

    virtual ~DBusEvent() {
        if (this->hasListeners())
            dbusProxy_.removeSignalMemberHandler(subscription_, this);
    }

    virtual SubscriptionStatus onSignalDBusMessage(const DBusMessage& dbusMessage) {
        return unpackArgumentsAndHandleSignalDBusMessage(dbusMessage, ArgumentsTuple());
    }
 protected:
    virtual void onFirstListenerAdded(const CancellableListener&) {
        subscription_ = dbusProxy_.addSignalMemberHandler(objectPath_,
                                                          interfaceName_,
                                                          eventName_,
                                                          eventSignature_,
                                                          this);
    }

    virtual void onLastListenerRemoved(const CancellableListener&) {
        dbusProxy_.removeSignalMemberHandler(subscription_, this);
    }

    template<typename ... _Arguments>
    inline SubscriptionStatus unpackArgumentsAndHandleSignalDBusMessage(const DBusMessage& dbusMessage,
                                                                        std::tuple<_Arguments...> argTuple) {
        return handleSignalDBusMessage(dbusMessage, std::move(argTuple), typename make_sequence<sizeof...(_Arguments)>::type());
    }

    template<typename ... _Arguments, int ... _ArgIndices>
    inline SubscriptionStatus handleSignalDBusMessage(const DBusMessage& dbusMessage,
                                                      std::tuple<_Arguments...> argTuple,
                                                      index_sequence<_ArgIndices...>) {
        DBusInputStream dbusInputStream(dbusMessage);
        const bool success = DBusSerializableArguments<_Arguments...>::deserialize(
                        dbusInputStream,
                        std::get<_ArgIndices>(argTuple)...);
        // Continue subscription if deserialization failed
        return success ? this->notifyListeners(std::get<_ArgIndices>(argTuple)...) : SubscriptionStatus::RETAIN;
    }

    _DBusProxy& dbusProxy_;
    const char* eventName_;
    const char* eventSignature_;
    const char* objectPath_;
    const char* interfaceName_;
    DBusProxyConnection::DBusSignalHandlerToken subscription_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_EVENT_H_

