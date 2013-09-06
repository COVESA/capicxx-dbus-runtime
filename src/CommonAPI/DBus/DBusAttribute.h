/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_ATTRIBUTE_H_
#define COMMONAPI_DBUS_DBUS_ATTRIBUTE_H_

#include "DBusProxyHelper.h"
#include "DBusEvent.h"
#include <stdint.h>
#include "DBusLegacyVariant.h"

#include <cassert>

namespace CommonAPI {
namespace DBus {


class DBusProxy;


template <typename _AttributeType, typename _DBusProxyType = DBusProxy>
class DBusReadonlyAttribute: public _AttributeType {
 public:
	typedef typename _AttributeType::ValueType ValueType;
	typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;

	DBusReadonlyAttribute(_DBusProxyType& dbusProxy, const char* setMethodSignature, const char* getMethodName):
			dbusProxy_(dbusProxy),
			getMethodName_(getMethodName),
			setMethodSignature_(setMethodSignature) {
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
	const char* setMethodSignature_;
};

template <typename _AttributeType, typename _DBusProxyType = DBusProxy>
class DBusFreedesktopReadonlyAttribute: public _AttributeType {
 public:
    typedef typename _AttributeType::ValueType ValueType;
    typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;

    DBusFreedesktopReadonlyAttribute(_DBusProxyType& dbusProxy, const char* interfaceName, const char* propertyName):
            dbusProxy_(dbusProxy),
            interfaceName_(interfaceName),
            propertyName_(propertyName)
            {
        assert(interfaceName);
        assert(propertyName);
    }

    void getValue(CallStatus& callStatus, ValueType& value) const {
        DBusLegacyVariantWrapper<Variant<ValueType> > variantVal;
        DBusProxyHelper<DBusSerializableArguments<std::string, std::string>,
                        DBusSerializableArguments<DBusLegacyVariantWrapper<Variant<ValueType> > > >::callMethodWithReply(
                        dbusProxy_,
                        dbusProxy_.getDBusBusName().c_str(),
                        dbusProxy_.getDBusObjectPath().c_str(),
                        "org.freedesktop.DBus.Properties",
                        "Get",
                        "ss",
                        std::string(interfaceName_),
                        std::string(propertyName_),
                        callStatus,
                        variantVal);
        value = variantVal.contained_.template get<ValueType>();

    }

    std::future<CallStatus> getValueAsync(AttributeAsyncCallback attributeAsyncCallback) {
        return DBusProxyHelper<DBusSerializableArguments<std::string, std::string>,
                        DBusSerializableArguments<DBusLegacyVariantWrapper<Variant<ValueType> > > >::callMethodAsync(
                        dbusProxy_,
                        dbusProxy_.getDBusBusName().c_str(),
                        dbusProxy_.getDBusObjectPath().c_str(),
                        "org.freedesktop.DBus.Properties",
                        "Get",
                        "ss",
                        std::string(interfaceName_),
                        std::string(propertyName_),
                        std::bind(
                                        &CommonAPI::DBus::DBusFreedesktopReadonlyAttribute<_AttributeType>::AsyncVariantStripper,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2,
                                        std::move(attributeAsyncCallback)));
    }

    void AsyncVariantStripper(const CommonAPI::CallStatus& status,
                              const DBusLegacyVariantWrapper<Variant<ValueType> >& value,
                              AttributeAsyncCallback attributeAsyncCallback) {
        attributeAsyncCallback(status, value.contained_.template get<ValueType>());
    }

 protected:
    _DBusProxyType& dbusProxy_;
    const char* interfaceName_;
    const char* propertyName_;
};

template <typename _AttributeType, typename _DBusProxyType = DBusProxy>
class DBusFreedesktopUnionReadonlyAttribute: public _AttributeType {
 public:
    typedef typename _AttributeType::ValueType ValueType;
    typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;

    DBusFreedesktopUnionReadonlyAttribute(_DBusProxyType& dbusProxy, const char* interfaceName, const char* propertyName):
            dbusProxy_(dbusProxy),
            interfaceName_(interfaceName),
            propertyName_(propertyName)
            {
        assert(interfaceName);
        assert(propertyName);
    }

    void getValue(CallStatus& callStatus, ValueType& value) const {
        DBusLegacyVariantWrapper<ValueType> variantVal(value);
        DBusProxyHelper<DBusSerializableArguments<std::string, std::string>,
                        DBusSerializableArguments<DBusLegacyVariantWrapper<ValueType> > >::callMethodWithReply(
                        dbusProxy_,
                        dbusProxy_.getDBusBusName().c_str(),
                        dbusProxy_.getDBusObjectPath().c_str(),
                        "org.freedesktop.DBus.Properties",
                        "Get",
                        "ss",
                        std::string(interfaceName_),
                        std::string(propertyName_),
                        callStatus,
                        variantVal);
        value = variantVal.contained_;
    }

    std::future<CallStatus> getValueAsync(AttributeAsyncCallback attributeAsyncCallback) {
        return DBusProxyHelper<DBusSerializableArguments<std::string, std::string>,
                        DBusSerializableArguments<DBusLegacyVariantWrapper<ValueType> > >::callMethodAsync(
                        dbusProxy_,
                        dbusProxy_.getDBusBusName().c_str(),
                        dbusProxy_.getDBusObjectPath().c_str(),
                        "org.freedesktop.DBus.Properties",
                        "Get",
                        "ss",
                        std::string(interfaceName_),
                        std::string(propertyName_),
                        std::bind(
                                        &CommonAPI::DBus::DBusFreedesktopUnionReadonlyAttribute<_AttributeType>::AsyncVariantStripper,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2,
                                        std::move(attributeAsyncCallback)));
    }

    void AsyncVariantStripper(const CommonAPI::CallStatus& status,
                              const DBusLegacyVariantWrapper<ValueType>& value,
                              AttributeAsyncCallback attributeAsyncCallback) {
        attributeAsyncCallback(status, value.contained_);
    }

 protected:
    _DBusProxyType& dbusProxy_;
    const char* interfaceName_;
    const char* propertyName_;
};

template <typename _AttributeType, typename _DBusProxyType = DBusProxy>
class DBusAttribute: public DBusReadonlyAttribute<_AttributeType> {
 public:
	typedef typename _AttributeType::ValueType ValueType;
	typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;

	DBusAttribute(_DBusProxyType& dbusProxy, const char* setMethodName, const char* setMethodSignature, const char* getMethodName):
	    DBusReadonlyAttribute<_AttributeType>(dbusProxy, setMethodSignature, getMethodName),
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
class DBusFreedesktopAttribute: public DBusFreedesktopReadonlyAttribute<_AttributeType, _DBusProxyType> {
 public:
    typedef typename _AttributeType::ValueType ValueType;
    typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;

    DBusFreedesktopAttribute(_DBusProxyType& dbusProxy,
                             const char* interfaceName,
                             const char* propertyName) :
                    DBusFreedesktopReadonlyAttribute<_AttributeType>(dbusProxy, interfaceName, propertyName)
    {
        assert(interfaceName);
        assert(propertyName);
    }


    void setValue(const ValueType& requestValue, CallStatus& callStatus, ValueType& responseValue) {
        DBusLegacyVariantWrapper<Variant<ValueType> > variantVal;
        variantVal.contained_ = Variant<ValueType>(requestValue);
        DBusProxyHelper<DBusSerializableArguments<std::string, std::string, DBusLegacyVariantWrapper<Variant<ValueType> > >,
                        DBusSerializableArguments<> >::callMethodWithReply(
                        DBusFreedesktopReadonlyAttribute<_AttributeType>::dbusProxy_,
                        DBusFreedesktopReadonlyAttribute<_AttributeType>::dbusProxy_.getDBusBusName().c_str(),
                        DBusFreedesktopReadonlyAttribute<_AttributeType>::dbusProxy_.getDBusObjectPath().c_str(),
                        "org.freedesktop.DBus.Properties",
                        "Set",
                        "ssv",
                        std::string(DBusFreedesktopReadonlyAttribute<_AttributeType>::interfaceName_),
                        std::string(DBusFreedesktopReadonlyAttribute<_AttributeType>::propertyName_),
                        variantVal,
                        callStatus);
        responseValue = requestValue;
    }

    std::future<CallStatus> setValueAsync(const ValueType& requestValue, AttributeAsyncCallback attributeAsyncCallback) {
        DBusLegacyVariantWrapper<Variant<ValueType> > variantVal;
        variantVal.contained_ = Variant<ValueType>(requestValue);
        return DBusProxyHelper<DBusSerializableArguments<std::string, std::string, DBusLegacyVariantWrapper<Variant<ValueType> > >,
                            DBusSerializableArguments<> >::callMethodAsync(
                            DBusFreedesktopReadonlyAttribute<_AttributeType>::dbusProxy_,
                            DBusFreedesktopReadonlyAttribute<_AttributeType>::dbusProxy_.getDBusBusName().c_str(),
                            DBusFreedesktopReadonlyAttribute<_AttributeType>::dbusProxy_.getDBusObjectPath().c_str(),
                            "org.freedesktop.DBus.Properties",
                            "Set",
                            "ssv",
                            std::string(DBusFreedesktopReadonlyAttribute<_AttributeType>::interfaceName_),
                            std::string(DBusFreedesktopReadonlyAttribute<_AttributeType>::propertyName_),
                            variantVal,
                            std::bind(
                                            &CommonAPI::DBus::DBusFreedesktopAttribute<_AttributeType>::AsyncVariantStripper,
                                            this,
                                            std::placeholders::_1,
                                            variantVal,
                                            std::move(attributeAsyncCallback)));
        }

    void AsyncVariantStripper(const CommonAPI::CallStatus& status,
                              const DBusLegacyVariantWrapper<Variant<ValueType> >& value,
                              AttributeAsyncCallback attributeAsyncCallback) {
        attributeAsyncCallback(status, value.contained_.template get<ValueType>());
    }
};

template <typename _AttributeType, typename _DBusProxyType = DBusProxy>
class DBusFreedesktopUnionAttribute: public DBusFreedesktopUnionReadonlyAttribute<_AttributeType, _DBusProxyType> {
 public:
    typedef typename _AttributeType::ValueType ValueType;
    typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;

    DBusFreedesktopUnionAttribute(_DBusProxyType& dbusProxy,
                             const char* interfaceName,
                             const char* propertyName) :
                    DBusFreedesktopUnionReadonlyAttribute<_AttributeType>(dbusProxy, interfaceName, propertyName)
    {
        assert(interfaceName);
        assert(propertyName);
    }


    void setValue(const ValueType& requestValue, CallStatus& callStatus, ValueType& responseValue) {
        DBusLegacyVariantWrapper<ValueType> variantVal;
        variantVal.contained_ = requestValue;
        DBusProxyHelper<DBusSerializableArguments<std::string, std::string, DBusLegacyVariantWrapper<ValueType> >,
                        DBusSerializableArguments<> >::callMethodWithReply(
                        DBusFreedesktopUnionAttribute<_AttributeType>::dbusProxy_,
                        DBusFreedesktopUnionAttribute<_AttributeType>::dbusProxy_.getDBusBusName().c_str(),
                        DBusFreedesktopUnionAttribute<_AttributeType>::dbusProxy_.getDBusObjectPath().c_str(),
                        "org.freedesktop.DBus.Properties",
                        "Set",
                        "ssv",
                        std::string(DBusFreedesktopUnionReadonlyAttribute<_AttributeType>::interfaceName_),
                        std::string(DBusFreedesktopUnionReadonlyAttribute<_AttributeType>::propertyName_),
                        variantVal,
                        callStatus);
        responseValue = requestValue;
    }

    std::future<CallStatus> setValueAsync(const ValueType& requestValue, AttributeAsyncCallback attributeAsyncCallback) {
        DBusLegacyVariantWrapper<ValueType> variantVal;
        variantVal.contained_ = requestValue;
        return DBusProxyHelper<DBusSerializableArguments<std::string, std::string, DBusLegacyVariantWrapper<ValueType> >,
                            DBusSerializableArguments<> >::callMethodAsync(
                            DBusFreedesktopUnionAttribute<_AttributeType>::dbusProxy_,
                            DBusFreedesktopUnionAttribute<_AttributeType>::dbusProxy_.getDBusBusName().c_str(),
                            DBusFreedesktopUnionAttribute<_AttributeType>::dbusProxy_.getDBusObjectPath().c_str(),
                            "org.freedesktop.DBus.Properties",
                            "Set",
                            "ssv",
                            std::string(DBusFreedesktopUnionReadonlyAttribute<_AttributeType>::interfaceName_),
                            std::string(DBusFreedesktopUnionReadonlyAttribute<_AttributeType>::propertyName_),
                            variantVal,
                            std::bind(
                                            &CommonAPI::DBus::DBusFreedesktopUnionAttribute<_AttributeType>::AsyncVariantStripper,
                                            this,
                                            std::placeholders::_1,
                                            variantVal,
                                            std::move(attributeAsyncCallback)));
        }

    void AsyncVariantStripper(const CommonAPI::CallStatus& status,
                              const DBusLegacyVariantWrapper<ValueType>& value,
                              AttributeAsyncCallback attributeAsyncCallback) {
        attributeAsyncCallback(status, value.contained_);
    }
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

template< class, class >
class LegacyEvent;

template <template <class...> class _Type, class _Types, typename _DBusProxy>
class LegacyEvent<_Type<_Types>, _DBusProxy> : public _Type<_Types> {
public:
    typedef _Types ValueType;
    typedef _Type<ValueType> CommonAPIEvent;
    typedef typename CommonAPIEvent::CancellableListener CancellableListener;

    LegacyEvent(_DBusProxy& dbusProxy, const char* interfaceName, const char* propName) :
        interfaceName_(interfaceName),
        propertyName_(propName),
        subSet_(false),
        internalEvent_(dbusProxy, "PropertiesChanged", "sa{sv}as", dbusProxy.getDBusObjectPath().c_str(), "org.freedesktop.DBus.Properties") {
    }

protected:
    void onInternalEvent(const std::string& interface,
                    const std::unordered_map<std::string, DBusLegacyVariantWrapper<Variant<_Types> > >& props,
                    const std::vector<std::string>& invalid) {
        if (std::string(interfaceName_) == interface) {
            auto mapIter = props.find(std::string(propertyName_));
            if (mapIter != props.end()) {
                notifyListeners(mapIter->second.contained_.template get<ValueType>());
            }
        }
    }

    void onFirstListenerAdded(const CancellableListener& listener) {
        sub = internalEvent_.subscribe(
                                      std::bind(
                                                &LegacyEvent<_Type<_Types>, _DBusProxy>::onInternalEvent,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2,
                                                std::placeholders::_3));
        subSet_ = true;
    }

    void onLastListenerRemoved(const CancellableListener& listener) {
        if (subSet_) {
            internalEvent_.unsubscribe(sub);
            subSet_ = false;
        }
    }

    typedef DBusLegacyVariantWrapper<Variant<_Types> > ContainedVariant;
    typedef std::unordered_map<std::string, ContainedVariant> PropertyMap;
    typedef std::vector<std::string> InvalidArray;
    typedef Event<std::string, PropertyMap, InvalidArray> SignalEvent;

    DBusEvent<SignalEvent> internalEvent_;

    typename DBusEvent<SignalEvent>::Subscription sub;

    const char* interfaceName_;
    const char* propertyName_;

    bool subSet_;

};

template <typename _AttributeType, typename _DBusProxyType = DBusProxy>
class DBusFreedesktopObservableAttribute: public _AttributeType {
 public:
    typedef typename _AttributeType::ValueType ValueType;
    typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;
    typedef typename _AttributeType::ChangedEvent ChangedEvent;

    template <typename... _AttributeTypeArguments>
    DBusFreedesktopObservableAttribute(_DBusProxyType& dbusProxy,
                    const char* interfaceName,
                    const char* propertyName,
                    _AttributeTypeArguments... arguments):
        _AttributeType(dbusProxy, interfaceName, propertyName, arguments...),
        propertyName_(propertyName),
        interfaceName_(interfaceName),
        externalChangedEvent_(dbusProxy, interfaceName, propertyName) {

    }

    ChangedEvent& getChangedEvent() {
        return externalChangedEvent_;
    }

 protected:
    LegacyEvent<ChangedEvent, _DBusProxyType> externalChangedEvent_;
    const char * propertyName_;
    const char * interfaceName_;
};

template< class, class >
class LegacyUnionEvent;

template <template <class...> class _Type, class _Types, typename _DBusProxy>
class LegacyUnionEvent<_Type<_Types>, _DBusProxy> : public _Type<_Types> {
public:
    typedef _Types ValueType;
    typedef _Type<ValueType> CommonAPIEvent;
    typedef typename CommonAPIEvent::CancellableListener CancellableListener;

    LegacyUnionEvent(_DBusProxy& dbusProxy, const char* interfaceName, const char* propName) :
        interfaceName_(interfaceName),
        propertyName_(propName),
        subSet_(false),
        internalEvent_(dbusProxy, "PropertiesChanged", "sa{sv}as", dbusProxy.getDBusObjectPath().c_str(), "org.freedesktop.DBus.Properties") {
    }

protected:
    void onInternalEvent(const std::string& interface,
                    const std::unordered_map<std::string, DBusLegacyVariantWrapper<ValueType> >& props,
                    const std::vector<std::string>& invalid) {
        if (std::string(interfaceName_) == interface) {
            auto mapIter = props.find(std::string(propertyName_));
            if (mapIter != props.end()) {
                notifyListeners(mapIter->second.contained_);
            }
        }
    }

    void onFirstListenerAdded(const CancellableListener& listener) {
        sub = internalEvent_.subscribe(
                                      std::bind(
                                                &LegacyUnionEvent<_Type<_Types>, _DBusProxy>::onInternalEvent,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2,
                                                std::placeholders::_3));
        subSet_ = true;
    }

    void onLastListenerRemoved(const CancellableListener& listener) {
        if (subSet_) {
            internalEvent_.unsubscribe(sub);
            subSet_ = false;
        }
    }

    typedef DBusLegacyVariantWrapper<ValueType> ContainedVariant;
    typedef std::unordered_map<std::string, ContainedVariant> PropertyMap;
    typedef std::vector<std::string> InvalidArray;
    typedef Event<std::string, PropertyMap, InvalidArray> SignalEvent;

    DBusEvent<SignalEvent> internalEvent_;

    typename DBusEvent<SignalEvent>::Subscription sub;

    const char* interfaceName_;
    const char* propertyName_;

    bool subSet_;

};

template <typename _AttributeType, typename _DBusProxyType = DBusProxy>
class DBusFreedesktopUnionObservableAttribute: public _AttributeType {
 public:
    typedef typename _AttributeType::ValueType ValueType;
    typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;
    typedef typename _AttributeType::ChangedEvent ChangedEvent;

    template <typename... _AttributeTypeArguments>
    DBusFreedesktopUnionObservableAttribute(_DBusProxyType& dbusProxy,
                    const char* interfaceName,
                    const char* propertyName,
                    _AttributeTypeArguments... arguments):
        _AttributeType(dbusProxy, interfaceName, propertyName, arguments...),
        propertyName_(propertyName),
        interfaceName_(interfaceName),
        externalChangedEvent_(dbusProxy, interfaceName, propertyName) {

    }

    ChangedEvent& getChangedEvent() {
        return externalChangedEvent_;
    }

 protected:
    LegacyUnionEvent<ChangedEvent, _DBusProxyType> externalChangedEvent_;
    const char * propertyName_;
    const char * interfaceName_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_ATTRIBUTE_H_
