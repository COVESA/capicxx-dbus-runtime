/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_DBUS_DBUS_ATTRIBUTE_H_
#define COMMONAPI_DBUS_DBUS_ATTRIBUTE_H_

#include "DBusProxyHelper.h"
#include "DBusEvent.h"

#include <cassert>

namespace CommonAPI {
namespace DBus {


class DBusProxy;


template <typename _AttributeType, typename _DBusProxyType = DBusProxy>
class DBusReadonlyAttribute: public _AttributeType {
 public:
	typedef typename _AttributeType::ValueType ValueType;
	typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;

	DBusReadonlyAttribute(_DBusProxyType& dbusProxy, const char* getMethodName):
			dbusProxy_(dbusProxy),
			getMethodName_(getMethodName) {
		assert(getMethodName);
	}

	void getValue(CallStatus& callStatus, ValueType& value) const {
		DBusProxyHelper<DBusSerializableArguments<>,
						DBusSerializableArguments<ValueType> >::callMethodWithReply(dbusProxy_, getMethodName_, "", callStatus, value);
	}

	std::future<CallStatus> getValueAsync(AttributeAsyncCallback attributeAsyncCallback) {
		return DBusProxyHelper<DBusSerializableArguments<>,
							   DBusSerializableArguments<ValueType> >::callMethodAsync(dbusProxy_, getMethodName_, "", std::move(attributeAsyncCallback));
	}

 protected:
	_DBusProxyType& dbusProxy_;
	const char* getMethodName_;
};


template <typename _AttributeType, typename _DBusProxyType = DBusProxy>
class DBusAttribute: public DBusReadonlyAttribute<_AttributeType> {
 public:
	typedef typename _AttributeType::ValueType ValueType;
	typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;

	DBusAttribute(_DBusProxyType& dbusProxy, const char* setMethodName, const char* setMethodSignature, const char* getMethodName):
		DBusReadonlyAttribute<_AttributeType>(dbusProxy, getMethodName),
			setMethodName_(setMethodName),
			setMethodSignature_(setMethodSignature) {
		assert(setMethodName);
		assert(setMethodSignature);
	}

	void setValue(const ValueType& requestValue, CallStatus& callStatus, ValueType& responseValue) {
		DBusProxyHelper<DBusSerializableArguments<ValueType>,
						DBusSerializableArguments<ValueType> >::callMethodWithReply(
								this->dbusProxy_,
								setMethodName_,
								setMethodSignature_,
								requestValue,
								callStatus,
								responseValue);
	}

	std::future<CallStatus> setValueAsync(const ValueType& requestValue, AttributeAsyncCallback attributeAsyncCallback) {
		return DBusProxyHelper<DBusSerializableArguments<ValueType>,
							   DBusSerializableArguments<ValueType> >::callMethodAsync(
									   this->dbusProxy_,
									   setMethodName_,
									   setMethodSignature_,
									   requestValue,
									   attributeAsyncCallback);
	}

 protected:
	const char* setMethodName_;
	const char* setMethodSignature_;
};


template <typename _AttributeType, typename _DBusProxyType = DBusProxy>
class DBusObservableAttribute: public _AttributeType {
 public:
	typedef typename _AttributeType::ValueType ValueType;
	typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;
	typedef typename _AttributeType::ChangedEvent ChangedEvent;

	template <typename... _AttributeTypeArguments>
	DBusObservableAttribute(_DBusProxyType& dbusProxy, const char* changedEventName, _AttributeTypeArguments... arguments):
		_AttributeType(dbusProxy, arguments...),
		changedEvent_(dbusProxy, changedEventName, this->setMethodSignature_) {
	}

	ChangedEvent& getChangedEvent() {
		return changedEvent_;
	}

 protected:
	DBusEvent<ChangedEvent> changedEvent_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_ATTRIBUTE_H_
