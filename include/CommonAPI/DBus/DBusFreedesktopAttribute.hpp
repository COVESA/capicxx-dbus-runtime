// Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.hpp> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_FREEDESKTOPATTRIBUTE_HPP_
#define COMMONAPI_DBUS_DBUS_FREEDESKTOPATTRIBUTE_HPP_

#include <CommonAPI/DBus/DBusDeployment.hpp>

namespace CommonAPI {
namespace DBus {

template <typename _AttributeType, typename _AttributeDepl = EmptyDeployment>
class DBusFreedesktopReadonlyAttribute: public _AttributeType {
public:
    typedef typename _AttributeType::ValueType ValueType;
    typedef _AttributeDepl ValueTypeDepl;
    typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;

    DBusFreedesktopReadonlyAttribute(DBusProxy &_proxy, const std::string &_interfaceName, const std::string &_propertyName,
        _AttributeDepl *_depl = nullptr)
    	: proxy_(_proxy),
          interfaceName_(_interfaceName),
          propertyName_(_propertyName),
          depl_(_depl) {
    }

    void getValue(CommonAPI::CallStatus &_status, ValueType &_value, const CommonAPI::CallInfo *_info) const {
        CommonAPI::Deployable<Variant<ValueType>, VariantDeployment<>> deployedValue(&freedesktopVariant);
        DBusProxyHelper<
			DBusSerializableArguments<
				std::string, std::string
			>,
            DBusSerializableArguments<
				CommonAPI::Deployable<Variant<ValueType>, VariantDeployment<>>
			>
        >::callMethodWithReply(
        		proxy_,
                "org.freedesktop.DBus.Properties",
                "Get",
                "ss",
				(_info ? _info : &defaultCallInfo),
				interfaceName_,
                propertyName_,
                _status,
                deployedValue);

        _value = deployedValue.getValue().template get<ValueType>();
    }

    std::future<CallStatus> getValueAsync(AttributeAsyncCallback _callback, const CommonAPI::CallInfo *_info) {
        CommonAPI::Deployable<Variant<ValueType>, VariantDeployment<>> deployedValue(&freedesktopVariant);
        return DBusProxyHelper<
        			DBusSerializableArguments<
						std::string, std::string
					>,
                    DBusSerializableArguments<
						CommonAPI::Deployable<Variant<ValueType>, VariantDeployment<>>
        			>
        	   >::callMethodAsync(
                        proxy_,
						"org.freedesktop.DBus.Properties",
                        "Get",
                        "ss",
						(_info ? _info : &defaultCallInfo),
						interfaceName_,
                        propertyName_,
						[_callback](CommonAPI::CallStatus _status, CommonAPI::Deployable<Variant<ValueType>, VariantDeployment<>> _value) {
        					_callback(_status, _value.getValue().template get<ValueType>());
        				},
						std::make_tuple(deployedValue)
				);
    }

protected:
    DBusProxy &proxy_;
    std::string interfaceName_;
    std::string propertyName_;
    _AttributeDepl *depl_;
};

template <typename _AttributeType>
class DBusFreedesktopUnionReadonlyAttribute: public _AttributeType {
public:
    typedef typename _AttributeType::ValueType ValueType;
    typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;

    DBusFreedesktopUnionReadonlyAttribute(DBusProxy &_proxy, const std::string &_interfaceName, const std::string &_propertyName)
    	: proxy_(_proxy),
          interfaceName_(_interfaceName),
          propertyName_(_propertyName) {
    }

    void getValue(CommonAPI::CallStatus &_status, ValueType &_value, const CommonAPI::CallInfo *_info) const {
        CommonAPI::Deployable<ValueType, VariantDeployment<>> deployedValue(&freedesktopVariant);
        DBusProxyHelper<
			DBusSerializableArguments<
				std::string, std::string
			>,
            DBusSerializableArguments<
				CommonAPI::Deployable<ValueType, VariantDeployment<>>
			>
        >::callMethodWithReply(
        		proxy_,
				"org.freedesktop.DBus.Properties",
                "Get",
                "ss",
				(_info ? _info : &defaultCallInfo),
                interfaceName_,
                propertyName_,
                _status,
                deployedValue);

        _value = deployedValue.getValue().template get<ValueType>();
    }

    std::future<CommonAPI::CallStatus> getValueAsync(AttributeAsyncCallback _callback, const CommonAPI::CallInfo *_info) {
        CommonAPI::Deployable<ValueType, VariantDeployment<>> deployedValue(&freedesktopVariant);
        return DBusProxyHelper<
        			DBusSerializableArguments<
						std::string, std::string
					>,
                    DBusSerializableArguments<
						CommonAPI::Deployable<ValueType, VariantDeployment<>>
        			>
        	   >::callMethodAsync(
                        proxy_,
						"org.freedesktop.DBus.Properties",
                        "Get",
                        "ss",
						(_info ? _info : &defaultCallInfo),
                        interfaceName_,
                        propertyName_,
						[_callback](CommonAPI::CallStatus _status, CommonAPI::Deployable<ValueType, VariantDeployment<>> _value) {
        					_callback(_status, _value.getValue().template get<ValueType>());
        				},
						std::make_tuple(deployedValue)
				);
    }

protected:
    DBusProxy &proxy_;
    std::string interfaceName_;
    std::string propertyName_;
};

template <typename _AttributeType, typename _AttributeDepl = EmptyDeployment>
class DBusFreedesktopAttribute
		: public DBusFreedesktopReadonlyAttribute<_AttributeType, _AttributeDepl> {
 public:
    typedef typename _AttributeType::ValueType ValueType;
    typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;

    DBusFreedesktopAttribute(DBusProxy &_proxy, const std::string &_interfaceName, const std::string &_propertyName, _AttributeDepl *_depl = nullptr)
        : DBusFreedesktopReadonlyAttribute<_AttributeType, _AttributeDepl>(_proxy, _interfaceName, _propertyName, _depl) {
    }

    void setValue(const ValueType &_request, CommonAPI::CallStatus &_status, ValueType &_response, const CommonAPI::CallInfo *_info) {
        CommonAPI::Deployable<Variant<ValueType>, VariantDeployment<>> deployedVariant(_request, &freedesktopVariant);
        DBusProxyHelper<
			DBusSerializableArguments<
				std::string, std::string, CommonAPI::Deployable<Variant<ValueType>, VariantDeployment<>>
			>,
            DBusSerializableArguments<
			>
        >::callMethodWithReply(
				DBusFreedesktopReadonlyAttribute<_AttributeType, _AttributeDepl>::proxy_,
				"org.freedesktop.DBus.Properties",
				"Set",
				"ssv",
				(_info ? _info : &defaultCallInfo),
				DBusFreedesktopReadonlyAttribute<_AttributeType, _AttributeDepl>::interfaceName_,
				DBusFreedesktopReadonlyAttribute<_AttributeType, _AttributeDepl>::propertyName_,
				deployedVariant,
				_status);
        _response = _request;
    }

    std::future<CommonAPI::CallStatus> setValueAsync(const ValueType &_request, AttributeAsyncCallback _callback, const CommonAPI::CallInfo *_info) {
    	CommonAPI::Deployable<Variant<ValueType>, VariantDeployment<>> deployedVariant(_request, &freedesktopVariant);
        return DBusProxyHelper<
        			DBusSerializableArguments<
						std::string, std::string, CommonAPI::Deployable<Variant<ValueType>, VariantDeployment<>>
        			>,
                    DBusSerializableArguments<
					>
        	   >::callMethodAsync(
					DBusFreedesktopReadonlyAttribute<_AttributeType, _AttributeDepl>::proxy_,
					"org.freedesktop.DBus.Properties",
					"Set",
					"ssv",
					(_info ? _info : &defaultCallInfo),
					DBusFreedesktopReadonlyAttribute<_AttributeType, _AttributeDepl>::interfaceName_,
					DBusFreedesktopReadonlyAttribute<_AttributeType, _AttributeDepl>::propertyName_,
					deployedVariant,
					[_callback, deployedVariant](CommonAPI::CallStatus _status) {
        				_callback(_status, deployedVariant.getValue().template get<ValueType>());
        			},
					std::tuple<>());
    }
};

template <typename _AttributeType>
class DBusFreedesktopUnionAttribute
		: public DBusFreedesktopReadonlyAttribute<_AttributeType> {
 public:
    typedef typename _AttributeType::ValueType ValueType;
    typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;

    DBusFreedesktopUnionAttribute(DBusProxy &_proxy, const std::string &_interfaceName, const std::string &_propertyName)
    	: DBusFreedesktopUnionReadonlyAttribute<_AttributeType>(_proxy, _interfaceName, _propertyName) {
    }

    void setValue(const ValueType &_request, CommonAPI::CallStatus &_status, ValueType &_response, const CommonAPI::CallInfo *_info) {
        CommonAPI::Deployable<ValueType, VariantDeployment<>> deployedVariant(_request, &freedesktopVariant);
        DBusProxyHelper<
			DBusSerializableArguments<
				std::string, std::string, CommonAPI::Deployable<ValueType, VariantDeployment<>>
			>,
            DBusSerializableArguments<
			>
        >::callMethodWithReply(
				DBusFreedesktopReadonlyAttribute<_AttributeType>::proxy_,
				"org.freedesktop.DBus.Properties",
				"Set",
				"ssv",
				(_info ? _info : &defaultCallInfo),
				DBusFreedesktopReadonlyAttribute<_AttributeType>::interfaceName_,
				DBusFreedesktopReadonlyAttribute<_AttributeType>::propertyName_,
				deployedVariant,
				_status);
        _response = _request;
    }

    std::future<CallStatus> setValueAsync(const ValueType &_request, AttributeAsyncCallback _callback, const CommonAPI::CallInfo *_info) {
    	CommonAPI::Deployable<ValueType, VariantDeployment<>> deployedVariant(_request, &freedesktopVariant);
        return DBusProxyHelper<
        			DBusSerializableArguments<
						std::string, std::string, CommonAPI::Deployable<ValueType, VariantDeployment<>>
        			>,
                    DBusSerializableArguments<
					>
        	   >::callMethodAsync(
					DBusFreedesktopReadonlyAttribute<_AttributeType>::proxy_,
					"org.freedesktop.DBus.Properties",
					"Set",
					"ssv",
					(_info ? _info : &defaultCallInfo),
					DBusFreedesktopReadonlyAttribute<_AttributeType>::interfaceName_,
					DBusFreedesktopReadonlyAttribute<_AttributeType>::propertyName_,
					deployedVariant,
					[_callback](CommonAPI::CallStatus _status, CommonAPI::Deployable<ValueType, VariantDeployment<>> _value) {
        				_callback(_status, _value.getValue().template get<ValueType>());
        			},
					std::make_tuple(deployedVariant));
    }
};

template<class, class>
class LegacyEvent;

template <template <class...> class _Type, class _Types, class _Variant>
class LegacyEvent<_Type<_Types>, _Variant>: public _Type<_Types> {
public:
    typedef _Types ValueType;
    typedef typename _Type<ValueType>::Listener Listener;
    typedef std::unordered_map<std::string, _Variant> PropertyMap;
    typedef MapDeployment<EmptyDeployment, VariantDeployment<>> PropertyMapDeployment;
    typedef Deployable<PropertyMap, PropertyMapDeployment> DeployedPropertyMap;
    typedef std::vector<std::string> InvalidArray;
    typedef Event<std::string, DeployedPropertyMap, InvalidArray> SignalEvent;

    LegacyEvent(DBusProxy &_proxy, const std::string &_interfaceName, const std::string &_propertyName)
    	: interfaceName_(_interfaceName),
		  propertyName_(_propertyName),
		  isSubcriptionSet_(false),
		  internalEvent_(_proxy,
				  	  	 "PropertiesChanged",
						 "sa{sv}as",
						 _proxy.getDBusAddress().getObjectPath(),
						 "org.freedesktop.DBus.Properties",
						 std::make_tuple("", getDeployedMap(), InvalidArray())) {
    }

protected:
    void onFirstListenerAdded(const Listener &) {
    	if (!isSubcriptionSet_) {
			subscription_ = internalEvent_.subscribe(
								[this](const std::string &_interfaceName,
									   const PropertyMap &_properties,
									   const InvalidArray &_invalid) {
									if (interfaceName_ == _interfaceName) {
										auto iter = _properties.find(propertyName_);
										if (iter != _properties.end()) {
											const ValueType &value = iter->second.template get<ValueType>();
											this->notifyListeners(value);
										}
									}
								});

			isSubcriptionSet_ = true;
    	}
    }

    void onLastListenerRemoved(const Listener &) {
        if (isSubcriptionSet_) {
            internalEvent_.unsubscribe(subscription_);
            isSubcriptionSet_ = false;
        }
    }

    std::string interfaceName_;
    std::string propertyName_;

    typename DBusEvent<SignalEvent, std::string, DeployedPropertyMap, InvalidArray>::Subscription subscription_;
    bool isSubcriptionSet_;

    DBusEvent<SignalEvent, std::string, DeployedPropertyMap, InvalidArray> internalEvent_;

private:
    static DeployedPropertyMap &getDeployedMap() {
    	static PropertyMapDeployment itsDeployment(nullptr, &freedesktopVariant);
    	static DeployedPropertyMap itsDeployedMap(&itsDeployment);
    	return itsDeployedMap;
    }
};

template <typename _AttributeType, typename _Variant>
class DBusFreedesktopObservableAttribute: public _AttributeType {
 public:
    typedef typename _AttributeType::ValueType ValueType;
    typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;
    typedef typename _AttributeType::ChangedEvent ChangedEvent;

    template <typename... _AttributeTypeArguments>
    DBusFreedesktopObservableAttribute(DBusProxy &_proxy,
    								   const std::string &_interfaceName,
									   const std::string &_propertyName,
									   _AttributeTypeArguments... _arguments)
	    : _AttributeType(_proxy, _interfaceName, _propertyName, _arguments...),
		  interfaceName_(_interfaceName),
		  propertyName_(_propertyName),
          externalChangedEvent_(_proxy, _interfaceName, _propertyName) {
    }

    ChangedEvent &getChangedEvent() {
        return externalChangedEvent_;
    }

 protected:
    std::string interfaceName_;
    std::string propertyName_;
    LegacyEvent<ChangedEvent, _Variant> externalChangedEvent_;
};

template<class, class>
class LegacyUnionEvent;

template <template <class...> class _Type, class _Types, class _Variant>
class LegacyUnionEvent<_Type<_Types>, _Variant>: public _Type<_Types> {
public:
    typedef _Types ValueType;
    typedef typename _Type<ValueType>::Listener Listener;
    typedef std::unordered_map<std::string, _Variant> PropertyMap;
    typedef MapDeployment<EmptyDeployment, VariantDeployment<>> PropertyMapDeployment;
    typedef CommonAPI::Deployable<PropertyMap, PropertyMapDeployment> DeployedPropertyMap;
    typedef std::vector<std::string> InvalidArray;
    typedef Event<std::string, DeployedPropertyMap, InvalidArray> SignalEvent;

    LegacyUnionEvent(DBusProxy &_proxy, const std::string &_interfaceName, const std::string &_propertyName)
    	: interfaceName_(_interfaceName),
		  propertyName_(_propertyName),
		  isSubcriptionSet_(false),
		  internalEvent_(_proxy,
				  	  	 "PropertiesChanged",
						 "sa{sv}as",
						 _proxy.getDBusAddress().getObjectPath(),
						 "org.freedesktop.DBus.Properties",
						 std::make_tuple("", getDeployedMap(), InvalidArray())) {
    }

protected:
    void onFirstListenerAdded(const Listener &) {
    	if (isSubcriptionSet_) {
			subscription_ = internalEvent_.subscribe(
								[this](const std::string &_interfaceName,
									   const PropertyMap &_properties,
									   const std::vector<std::string> &_invalid) {
									if (interfaceName_ == _interfaceName) {
										auto iter = _properties.find(propertyName_);
										if (iter != _properties.end()) {
											this->notifyListeners(iter->second.template get<ValueType>());
										}
									}
								});
			isSubcriptionSet_ = true;
    	}
    }

    void onLastListenerRemoved(const Listener &) {
        if (isSubcriptionSet_) {
            internalEvent_.unsubscribe(subscription_);
            isSubcriptionSet_ = false;
        }
    }

    DBusEvent<SignalEvent, ValueType> internalEvent_;
    std::string interfaceName_;
    std::string propertyName_;

    typename DBusEvent<SignalEvent>::Subscription subscription_;
    bool isSubcriptionSet_;

private:
    static DeployedPropertyMap &getDeployedMap() {
    	static PropertyMapDeployment itsDeployment(nullptr, &freedesktopVariant);
    	static DeployedPropertyMap itsDeployedMap(&itsDeployment);
    	return itsDeployedMap;
    }
};

template <typename _AttributeType, typename _Variant>
class DBusFreedesktopUnionObservableAttribute: public _AttributeType {
 public:
    typedef typename _AttributeType::ValueType ValueType;
    typedef typename _AttributeType::AttributeAsyncCallback AttributeAsyncCallback;
    typedef typename _AttributeType::ChangedEvent ChangedEvent;

    template <typename... _AttributeTypeArguments>
    DBusFreedesktopUnionObservableAttribute(DBusProxy &_proxy,
    								        const std::string &_interfaceName,
											const std::string &_propertyName,
											_AttributeTypeArguments... _arguments)
	    : _AttributeType(_proxy, _interfaceName, _propertyName, _arguments...),
          externalChangedEvent_(_proxy, _interfaceName, _propertyName) {
    }

    ChangedEvent &getChangedEvent() {
        return externalChangedEvent_;
    }

 protected:
    LegacyUnionEvent<ChangedEvent, _Variant> externalChangedEvent_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_FREEDESKTOPATTRIBUTE_HPP_
