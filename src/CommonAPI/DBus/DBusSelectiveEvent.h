/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_SELECTIVE_EVENT_H_
#define COMMONAPI_DBUS_DBUS_SELECTIVE_EVENT_H_

#include "DBusEvent.h"

namespace CommonAPI {
namespace DBus {

template<typename _EventType, typename _DBusProxy = DBusProxyBase>
class DBusSelectiveEvent: public DBusEvent<_EventType, _DBusProxy> {
public:
    typedef typename DBusEvent<_EventType, _DBusProxy>::CancellableListener CancellableListener;
    typedef DBusEvent<_EventType, _DBusProxy> DBusEventBase;

    DBusSelectiveEvent(DBusProxy& associatedDbusProxy,
                       const char* eventName,
                       const char* eventSignature) :
                    DBusEventBase(associatedDbusProxy, eventName, eventSignature),
                    associatedDbusProxy_(associatedDbusProxy) {
    }

    DBusSelectiveEvent(DBusProxy& associatedDbusProxy, const char* eventName,
                       const char* eventSignature,
                       const char* objPath,
                       const char* interfaceName) :

                    DBusEvent<_EventType, _DBusProxy>(
                                    associatedDbusProxy,
                                    eventName,
                                    eventSignature,
                                    objPath,
                                    interfaceName),
                    associatedDbusProxy_(associatedDbusProxy) {
    }

    virtual ~DBusSelectiveEvent() { }

    typename _EventType::Subscription subscribe(typename _EventType::Listener listener, bool& success) {

        DBusEventBase::listenerListMutex_.lock();
        const bool firstListenerAdded = DBusEventBase::listenersList_.empty();

        typename _EventType::CancellableListenerWrapper wrappedListener(std::move(listener));
        DBusEventBase::listenersList_.emplace_front(std::move(wrappedListener));
        typename _EventType::Subscription listenerSubscription = DBusEventBase::listenersList_.begin();

        success = true;
        DBusProxyConnection::DBusSignalHandlerToken dbusSignalHandlerToken;

        if (firstListenerAdded) {
            dbusSignalHandlerToken = associatedDbusProxy_.subscribeForSelectiveBroadcastOnConnection(
                            success,
                            DBusEventBase::objectPath_,
                            DBusEventBase::interfaceName_,
                            DBusEventBase::eventName_,
                            DBusEventBase::eventSignature_,
                            this
                            );
        }

        if (success) {
            // actually add subscription
            DBusEventBase::subscription_ = dbusSignalHandlerToken;
            this->onListenerAdded(*listenerSubscription);
        }
        else {
            DBusEventBase::listenersList_.erase(listenerSubscription);
            listenerSubscription = DBusEventBase::listenersList_.end();
        }
        DBusEventBase::listenerListMutex_.unlock();

        return listenerSubscription;
    }

protected:
    virtual void onLastListenerRemoved(const CancellableListener&) {
        associatedDbusProxy_.unsubscribeFromSelectiveBroadcast(
                        DBusEventBase::eventName_,
                        DBusEventBase::subscription_,
                        this);
    }

private:
    DBusProxy& associatedDbusProxy_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_SELECTIVE_EVENT_H_
