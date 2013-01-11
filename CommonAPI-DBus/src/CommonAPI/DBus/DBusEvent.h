/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_DBUS_DBUS_EVENT_H_
#define COMMONAPI_DBUS_DBUS_EVENT_H_

#include "DBusProxyConnection.h"
#include "DBusMessage.h"
#include "DBusSerializableArguments.h"

#include <CommonAPI/Event.h>

namespace CommonAPI {
namespace DBus {

class DBusProxy;


template <typename _EventType, typename _DBusProxy = DBusProxy>
class DBusEvent: public _EventType, public DBusProxyConnection::DBusSignalHandler {
 public:
	typedef typename _EventType::ArgumentsTuple ArgumentsTuple;
	typedef typename _EventType::CancellableListener CancellableListener;

	DBusEvent(_DBusProxy& dbusProxy, const char* eventName, const char* eventSignature):
			dbusProxy_(dbusProxy),
			eventName_(eventName),
			eventSignature_(eventSignature) {
		assert(eventName);
		assert(eventSignature);
	}

	virtual ~DBusEvent() {
		if (this->hasListeners())
			dbusProxy_.removeSignalMemberHandler(subscription_);
	}

	virtual SubscriptionStatus onSignalDBusMessage(const DBusMessage& dbusMessage) {
		return unpackArgumentsAndHandleSignalDBusMessage(dbusMessage, ArgumentsTuple());
	}

 protected:
	virtual void onFirstListenerAdded(const CancellableListener&) {
	 	subscription_ = dbusProxy_.addSignalMemberHandler(eventName_, eventSignature_, this);
 	}

	virtual void onLastListenerRemoved(const CancellableListener&) {
	 	dbusProxy_.removeSignalMemberHandler(subscription_);
	}

 private:
	template <typename ... _Arguments>
	inline SubscriptionStatus unpackArgumentsAndHandleSignalDBusMessage(const DBusMessage& dbusMessage, std::tuple<_Arguments...> argTuple) {
		return handleSignalDBusMessage(dbusMessage, std::move(argTuple), typename make_sequence<sizeof...(_Arguments)>::type());
	}

	template <typename ... _Arguments, int... _ArgIndices>
	inline SubscriptionStatus handleSignalDBusMessage(const DBusMessage& dbusMessage, std::tuple<_Arguments...> argTuple, index_sequence<_ArgIndices...>) {
		DBusInputStream dbusInputStream(dbusMessage);
		const bool success = DBusSerializableArguments<_Arguments...>::deserialize(dbusInputStream, std::get<_ArgIndices>(argTuple)...);
		// Continue subscription if deserialization failed
		return success ? this->notifyListeners(std::get<_ArgIndices>(argTuple)...) : SubscriptionStatus::RETAIN;
	}

 	_DBusProxy& dbusProxy_;
	const char* eventName_;
	const char* eventSignature_;
	DBusProxyConnection::DBusSignalHandlerToken subscription_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_EVENT_H_
