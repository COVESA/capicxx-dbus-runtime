// Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.hpp> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUSFREEDESKTOPSTUBADAPTERHELPER_HPP_
#define COMMONAPI_DBUS_DBUSFREEDESKTOPSTUBADAPTERHELPER_HPP_

#include <CommonAPI/Struct.hpp>
#include <CommonAPI/DBus/DBusStubAdapterHelper.hpp>

namespace CommonAPI {
namespace DBus {

template <typename _StubClass>
class DBusGetFreedesktopAttributeStubDispatcherBase {
public:
    virtual ~DBusGetFreedesktopAttributeStubDispatcherBase() {}
    virtual void dispatchDBusMessageAndAppendReply(const DBusMessage &_message,
    											   const std::shared_ptr<_StubClass> &_stub,
												   DBusOutputStream &_output,
												   const std::shared_ptr<DBusClientId> &_clientId) = 0;
};

template <typename _StubClass, typename _AttributeType>
class DBusGetFreedesktopAttributeStubDispatcher
		: public virtual DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>,
		  public virtual DBusGetFreedesktopAttributeStubDispatcherBase<_StubClass> {
public:
    typedef DBusStubAdapterHelper<_StubClass> DBusStubAdapterHelperType;
    typedef typename DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>::GetStubFunctor GetStubFunctor;

    DBusGetFreedesktopAttributeStubDispatcher(GetStubFunctor _getStubFunctor)
    	: DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>(_getStubFunctor, "v") {
    }

    virtual ~DBusGetFreedesktopAttributeStubDispatcher() {};

    void dispatchDBusMessageAndAppendReply(const DBusMessage &_message,
    									   const std::shared_ptr<_StubClass> &_stub,
										   DBusOutputStream &_output,
										   const std::shared_ptr<DBusClientId> &_clientId) {
    	CommonAPI::Deployable<CommonAPI::Variant<_AttributeType>, VariantDeployment<>> deployedVariant(
        		(_stub.get()->*(DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>::getStubFunctor_))(_clientId), &freedesktopVariant);

        _output << deployedVariant;
    }

protected:
   virtual bool sendAttributeValueReply(const DBusMessage &_message, const std::shared_ptr<_StubClass> &_stub, DBusStubAdapterHelperType &_helper) {
       DBusMessage reply = _message.createMethodReturn(DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>::signature_);

       std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(_message.getSender()));
       CommonAPI::Deployable<CommonAPI::Variant<_AttributeType>, VariantDeployment<>> deployedVariant(
    		   (_stub.get()->*(DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>::getStubFunctor_))(clientId), &freedesktopVariant);

       DBusOutputStream output(reply);
       output << deployedVariant;
       output.flush();

       return _helper.getDBusConnection()->sendDBusMessage(reply);
   }
};

template <typename _StubClass, typename _AttributeType>
class DBusSetFreedesktopAttributeStubDispatcher
		: public virtual DBusGetFreedesktopAttributeStubDispatcher<_StubClass, _AttributeType>,
		  public virtual DBusSetAttributeStubDispatcher<_StubClass, _AttributeType> {
public:
    typedef typename DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>::GetStubFunctor GetStubFunctor;
    typedef typename DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>::DBusStubAdapterHelperType DBusStubAdapterHelperType;
    typedef typename DBusStubAdapterHelperType::RemoteEventHandlerType RemoteEventHandlerType;
    typedef bool (RemoteEventHandlerType::*OnRemoteSetFunctor)(std::shared_ptr<CommonAPI::ClientId>, _AttributeType);
    typedef void (RemoteEventHandlerType::*OnRemoteChangedFunctor)();

    DBusSetFreedesktopAttributeStubDispatcher(
    		GetStubFunctor _getStubFunctor,
			OnRemoteSetFunctor _onRemoteSetFunctor,
            OnRemoteChangedFunctor _onRemoteChangedFunctor)
    	: DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>(_getStubFunctor, "v"),
          DBusGetFreedesktopAttributeStubDispatcher<_StubClass, _AttributeType>(_getStubFunctor),
          DBusSetAttributeStubDispatcher<_StubClass, _AttributeType>(_getStubFunctor, _onRemoteSetFunctor, _onRemoteChangedFunctor, "v") {
    }

    virtual ~DBusSetFreedesktopAttributeStubDispatcher() {};

protected:
    virtual _AttributeType retreiveAttributeValue(const DBusMessage &_message, bool &_error) {
        std::string interfaceName, attributeName;
        DBusInputStream input(_message);
        CommonAPI::Deployable<CommonAPI::Variant<_AttributeType>, VariantDeployment<>> deployedVariant(&freedesktopVariant);
        input >> interfaceName; // skip over interface and attribute name
        input >> attributeName;
        input >> deployedVariant;
        _error = input.hasError();
        _AttributeType attributeValue = deployedVariant.getValue().template get<_AttributeType>() ;
        return attributeValue;
    }
};

template <typename _StubClass, typename _AttributeType>
class DBusSetFreedesktopObservableAttributeStubDispatcher
		: public virtual DBusSetFreedesktopAttributeStubDispatcher<_StubClass, _AttributeType>,
		  public virtual DBusSetObservableAttributeStubDispatcher<_StubClass, _AttributeType> {
public:
    typedef typename DBusSetFreedesktopAttributeStubDispatcher<_StubClass, _AttributeType>::DBusStubAdapterHelperType DBusStubAdapterHelperType;
    typedef typename DBusStubAdapterHelperType::StubAdapterType StubAdapterType;
    typedef typename DBusSetFreedesktopAttributeStubDispatcher<_StubClass, _AttributeType>::GetStubFunctor GetStubFunctor;
    typedef typename DBusSetFreedesktopAttributeStubDispatcher<_StubClass, _AttributeType>::OnRemoteSetFunctor OnRemoteSetFunctor;
    typedef typename DBusSetFreedesktopAttributeStubDispatcher<_StubClass, _AttributeType>::OnRemoteChangedFunctor OnRemoteChangedFunctor;
    typedef void (StubAdapterType::*FireChangedFunctor)(const _AttributeType&);

    DBusSetFreedesktopObservableAttributeStubDispatcher(
    		GetStubFunctor _getStubFunctor,
            OnRemoteSetFunctor _onRemoteSetFunctor,
            OnRemoteChangedFunctor _onRemoteChangedFunctor,
            FireChangedFunctor _fireChangedFunctor)
    	: DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>(_getStubFunctor, "v"),
          DBusGetFreedesktopAttributeStubDispatcher<_StubClass, _AttributeType>(_getStubFunctor),
          DBusSetAttributeStubDispatcher<_StubClass, _AttributeType>(_getStubFunctor, _onRemoteSetFunctor, _onRemoteChangedFunctor, "v"),
          DBusSetFreedesktopAttributeStubDispatcher<_StubClass, _AttributeType>(_getStubFunctor, _onRemoteSetFunctor, _onRemoteChangedFunctor),
          DBusSetObservableAttributeStubDispatcher<_StubClass, _AttributeType>(_getStubFunctor, _onRemoteSetFunctor, _onRemoteChangedFunctor, _fireChangedFunctor, "v") {
    }
};

template<class>
struct DBusStubFreedesktopPropertiesSignalHelper;

template<template<class ...> class _In, class _InArg>
struct DBusStubFreedesktopPropertiesSignalHelper<_In<DBusInputStream, DBusOutputStream, _InArg>> {

	template <typename _ValueType>
    struct DBusPropertiesEntry
    		: public CommonAPI::Struct<std::string, CommonAPI::Variant<_ValueType>> {

		DBusPropertiesEntry() = default;
        DBusPropertiesEntry(const std::string &_propertyName,
        					const _ValueType &_propertyValue) {
        	std::get<0>(this->values_) = _propertyName;
        	std::get<1>(this->values_) = _propertyValue;
        };

        const std::string &getPropertyName() const { return std::get<0>(this->values_); }
        void setPropertyName(const std::string &_value) { std::get<0>(this->values_) = _value; }

        const _ValueType getPropertyValue() const { return std::get<1>(this->values_); }
        void setPropertyValue(const _ValueType &_value) { std::get<1>(this->values_) = _value; }
    };

	typedef std::vector<DBusPropertiesEntry<_InArg>> PropertiesArray;
	typedef CommonAPI::Deployment<CommonAPI::EmptyDeployment, VariantDeployment<>> PropertyDeployment;
	typedef CommonAPI::ArrayDeployment<PropertyDeployment> PropertiesDeployment;
	typedef CommonAPI::Deployable<PropertiesArray, PropertiesDeployment> DeployedPropertiesArray;

	template <typename _StubClass>
    static bool sendPropertiesChangedSignal(const _StubClass &_stub, const std::string &_propertyName, const _InArg &_inArg) {
        const std::vector<std::string> invalidatedProperties;
        PropertiesArray changedProperties;
        DBusPropertiesEntry<_InArg> entry(_propertyName, _inArg);
        changedProperties.push_back(entry);

        PropertyDeployment propertyDeployment(nullptr, &freedesktopVariant);
        PropertiesDeployment changedPropertiesDeployment(&propertyDeployment);

        DeployedPropertiesArray deployedChangedProperties(changedProperties, &changedPropertiesDeployment);

        return DBusStubSignalHelper<
        			_In<
						DBusInputStream,
						DBusOutputStream,
						const std::string,
						DeployedPropertiesArray,
						std::vector<std::string>
        			>
			   >::sendSignal(
					_stub.getDBusAddress().getObjectPath().c_str(),
                    "org.freedesktop.DBus.Properties",
                    "PropertiesChanged",
                    "sa{sv}as",
                    _stub.getDBusConnection(),
                    _stub.getInterface(),
                    deployedChangedProperties,
                    invalidatedProperties);
    }
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUSFREEDESKTOPSTUBADAPTERHELPER_HPP_
