// Copyright (C) 2013-2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.hpp> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_EVENT_HPP_
#define COMMONAPI_DBUS_DBUS_EVENT_HPP_

#include <CommonAPI/Event.hpp>
#include <CommonAPI/DBus/DBusAddress.hpp>
#include <CommonAPI/DBus/DBusHelper.hpp>
#include <CommonAPI/DBus/DBusMessage.hpp>
#include <CommonAPI/DBus/DBusProxyBase.hpp>
#include <CommonAPI/DBus/DBusProxyConnection.hpp>
#include <CommonAPI/DBus/DBusSerializableArguments.hpp>

namespace CommonAPI {
namespace DBus {

template <typename _Event, typename... _Arguments>
class DBusEvent: public _Event, public DBusProxyConnection::DBusSignalHandler {
public:
    typedef typename _Event::Listener Listener;

    DBusEvent(DBusProxyBase &_proxy,
    		  const std::string &_name, const std::string &_signature,
    		  std::tuple<_Arguments...> _arguments)
    	: proxy_(_proxy),
          name_(_name), signature_(_signature),
          arguments_(_arguments) {

        interface_ = proxy_.getDBusAddress().getInterface();
        path_ = proxy_.getDBusAddress().getObjectPath();
    }

    DBusEvent(DBusProxyBase &_proxy,
    		  const std::string &_name, const std::string &_signature,
    		  const std::string &_path, const std::string &_interface,
    		  std::tuple<_Arguments...> _arguments)
    	: proxy_(_proxy),
          name_(_name), signature_(_signature),
          path_(_path), interface_(_interface),
          arguments_(_arguments) {
    }

    virtual ~DBusEvent() {
        proxy_.removeSignalMemberHandler(subscription_, this);
    }

    virtual void onSignalDBusMessage(const DBusMessage &_message) {
        handleSignalDBusMessage(_message, typename make_sequence<sizeof...(_Arguments)>::type());
    }
 protected:
    virtual void onFirstListenerAdded(const Listener&) {
        subscription_ = proxy_.addSignalMemberHandler(
        					path_, interface_, name_, signature_, this);
    }

    virtual void onLastListenerRemoved(const Listener&) {
        proxy_.removeSignalMemberHandler(subscription_, this);
    }

    template<int ... _Indices>
    inline void handleSignalDBusMessage(const DBusMessage &_message, index_sequence<_Indices...>) {
        DBusInputStream input(_message);
        if (DBusSerializableArguments<
        		_Arguments...
        	>::deserialize(input, std::get<_Indices>(arguments_)...)) {
        	this->notifyListeners(std::get<_Indices>(arguments_)...);
        }
    }

    DBusProxyBase &proxy_;

    std::string name_;
    std::string signature_;
    std::string path_;
    std::string interface_;

    DBusProxyConnection::DBusSignalHandlerToken subscription_;
    std::tuple<_Arguments...> arguments_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_EVENT_HPP_
