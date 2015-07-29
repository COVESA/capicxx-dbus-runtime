// Copyright (C) 2013-2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.hpp> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUSSTUBADAPTERHELPER_HPP_
#define COMMONAPI_DBUS_DBUSSTUBADAPTERHELPER_HPP_

#include <initializer_list>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <map>

#include <CommonAPI/Variant.hpp>
#include <CommonAPI/DBus/DBusStubAdapter.hpp>
#include <CommonAPI/DBus/DBusInputStream.hpp>
#include <CommonAPI/DBus/DBusOutputStream.hpp>
#include <CommonAPI/DBus/DBusHelper.hpp>
#include <CommonAPI/DBus/DBusSerializableArguments.hpp>
#include <CommonAPI/DBus/DBusClientId.hpp>

namespace CommonAPI {
namespace DBus {

class StubDispatcherBase {
public:
   virtual ~StubDispatcherBase() { }
};




struct DBusAttributeDispatcherStruct {
    StubDispatcherBase* getter;
    StubDispatcherBase* setter;

    DBusAttributeDispatcherStruct(StubDispatcherBase* g, StubDispatcherBase* s) {
        getter = g;
        setter = s;
    }
};

typedef std::unordered_map<std::string, DBusAttributeDispatcherStruct> StubAttributeTable;

template <typename _StubClass>
class DBusStubAdapterHelper: public virtual DBusStubAdapter {
 public:
    typedef typename _StubClass::StubAdapterType StubAdapterType;
    typedef typename _StubClass::RemoteEventHandlerType RemoteEventHandlerType;

    class StubDispatcher: public StubDispatcherBase {
    public:
    	virtual ~StubDispatcher() {}
        virtual bool dispatchDBusMessage(const DBusMessage& dbusMessage,
                                         const std::shared_ptr<_StubClass>& stub,
                                         DBusStubAdapterHelper<_StubClass>& dbusStubAdapterHelper) = 0;
        virtual void appendGetAllReply(const DBusMessage& dbusMessage,
                                       const std::shared_ptr<_StubClass>& stub,
                                       DBusStubAdapterHelper<_StubClass>& dbusStubAdapterHelper,
                                       DBusOutputStream &_output) {}
    };
    // interfaceMemberName, interfaceMemberSignature
    typedef std::pair<const char*, const char*> DBusInterfaceMemberPath;
    typedef std::unordered_map<DBusInterfaceMemberPath, StubDispatcherBase*> StubDispatcherTable;

    DBusStubAdapterHelper(const DBusAddress &_address,
                          const std::shared_ptr<DBusProxyConnection> &_connection,
                          const std::shared_ptr<_StubClass> &_stub,
                          const bool _isManaging):
                    DBusStubAdapter(_address, _connection, _isManaging),
                    stub_(_stub),
                    remoteEventHandler_(nullptr) {
    }

    virtual ~DBusStubAdapterHelper() {
        DBusStubAdapter::deinit();
        stub_.reset();
    }

    virtual void init(std::shared_ptr<DBusStubAdapter> instance) {
        DBusStubAdapter::init(instance);
        std::shared_ptr<StubAdapterType> stubAdapter = std::dynamic_pointer_cast<StubAdapterType>(instance);
        remoteEventHandler_ = stub_->initStubAdapter(stubAdapter);
    }

    virtual void deinit() {
        DBusStubAdapter::deinit();
        stub_.reset();
    }

    inline RemoteEventHandlerType* getRemoteEventHandler() {
        return remoteEventHandler_;
    }

 protected:

    virtual bool onInterfaceDBusMessage(const DBusMessage& dbusMessage) {
        const char* interfaceMemberName = dbusMessage.getMember();
        const char* interfaceMemberSignature = dbusMessage.getSignature();

        assert(interfaceMemberName);
        assert(interfaceMemberSignature);

        DBusInterfaceMemberPath dbusInterfaceMemberPath(interfaceMemberName, interfaceMemberSignature);
        auto findIterator = getStubDispatcherTable().find(dbusInterfaceMemberPath);
        const bool foundInterfaceMemberHandler = (findIterator != getStubDispatcherTable().end());
        bool dbusMessageHandled = false;
        if (foundInterfaceMemberHandler) {
            StubDispatcher* stubDispatcher = static_cast<StubDispatcher*>(findIterator->second);
            dbusMessageHandled = stubDispatcher->dispatchDBusMessage(dbusMessage, stub_, *this);
        }

        return dbusMessageHandled;
    }

    virtual bool onInterfaceDBusFreedesktopPropertiesMessage(const DBusMessage &_message) {
        DBusInputStream input(_message);

        if (_message.hasMemberName("Get")) {
            return handleFreedesktopGet(_message, input);
        } else if (_message.hasMemberName("Set")) {
            return handleFreedesktopSet(_message, input);
        } else if (_message.hasMemberName("GetAll")) {
            return handleFreedesktopGetAll(_message, input);
        }

        return false;
    }

    virtual const StubDispatcherTable& getStubDispatcherTable() = 0;
    virtual const StubAttributeTable& getStubAttributeTable() = 0;

    std::shared_ptr<_StubClass> stub_;
    RemoteEventHandlerType* remoteEventHandler_;

 private:
    bool handleFreedesktopGet(const DBusMessage &_message, DBusInputStream &_input) {
        std::string interfaceName;
        std::string attributeName;
        _input >> interfaceName;
        _input >> attributeName;

        if (_input.hasError()) {
            return false;
        }

        auto attributeDispatcherIterator = getStubAttributeTable().find(attributeName);
        if (attributeDispatcherIterator == getStubAttributeTable().end()) {
            return false;
        }

		StubDispatcher* getterDispatcher = static_cast<StubDispatcher*>(attributeDispatcherIterator->second.getter);
		assert(getterDispatcher != NULL); // all attributes have at least a getter
		return (getterDispatcher->dispatchDBusMessage(_message, stub_, *this));
    }

    bool handleFreedesktopSet(const DBusMessage& dbusMessage, DBusInputStream& dbusInputStream) {
        std::string interfaceName;
        std::string attributeName;
        dbusInputStream >> interfaceName;
        dbusInputStream >> attributeName;

        if(dbusInputStream.hasError()) {
            return false;
        }

        auto attributeDispatcherIterator = getStubAttributeTable().find(attributeName);
        if(attributeDispatcherIterator == getStubAttributeTable().end()) {
            return false;
        }

		StubDispatcher *setterDispatcher = static_cast<StubDispatcher*>(attributeDispatcherIterator->second.setter);
		if (setterDispatcher == NULL) { // readonly attributes do not have a setter
			return false;
		}

		return setterDispatcher->dispatchDBusMessage(dbusMessage, stub_, *this);
    }

    bool handleFreedesktopGetAll(const DBusMessage& dbusMessage, DBusInputStream& dbusInputStream) {
        std::string interfaceName;
        dbusInputStream >> interfaceName;

        if(dbusInputStream.hasError()) {
            return false;
        }

        DBusMessage dbusMessageReply = dbusMessage.createMethodReturn("a{sv}");
        DBusOutputStream dbusOutputStream(dbusMessageReply);

        dbusOutputStream.beginWriteMap();

        std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSender()));
        for(auto attributeDispatcherIterator = getStubAttributeTable().begin(); attributeDispatcherIterator != getStubAttributeTable().end(); attributeDispatcherIterator++) {

            //To prevent the destruction of the stub whilst still handling a message
            if (stub_) {
                StubDispatcher* getterDispatcher = static_cast<StubDispatcher*>(attributeDispatcherIterator->second.getter);
                assert(getterDispatcher != NULL); // all attributes have at least a getter
                dbusOutputStream.align(8);
                dbusOutputStream << attributeDispatcherIterator->first;
                getterDispatcher->appendGetAllReply(dbusMessage, stub_, *this, dbusOutputStream);
            }
        }

        dbusOutputStream.endWriteMap();
        dbusOutputStream.flush();

        return getDBusConnection()->sendDBusMessage(dbusMessageReply);
    }
};

template< class >
struct DBusStubSignalHelper;

template<template<class ...> class _In, class... _InArgs>
struct DBusStubSignalHelper<_In<DBusInputStream, DBusOutputStream, _InArgs...>> {

    static inline bool sendSignal(const char* objectPath,
                           const char* interfaceName,
                    const char* signalName,
                    const char* signalSignature,
                    const std::shared_ptr<DBusProxyConnection>& dbusConnection,
                    const _InArgs&... inArgs) {
        DBusMessage dbusMessage = DBusMessage::createSignal(
                        objectPath,
                        interfaceName,
                        signalName,
                        signalSignature);

        if (sizeof...(_InArgs) > 0) {
            DBusOutputStream outputStream(dbusMessage);
            const bool success = DBusSerializableArguments<_InArgs...>::serialize(outputStream, inArgs...);
            if (!success) {
                return false;
            }
            outputStream.flush();
        }

        const bool dbusMessageSent = dbusConnection->sendDBusMessage(dbusMessage);
        return dbusMessageSent;
    }

    template <typename _DBusStub = DBusStubAdapter>
    static bool sendSignal(const _DBusStub &_stub,
                    const char *_name,
                    const char *_signature,
                    const _InArgs&... inArgs) {
        return(sendSignal(_stub.getDBusAddress().getObjectPath().c_str(),
                          _stub.getDBusAddress().getInterface().c_str(),
                          _name,
                          _signature,
                          _stub.getDBusConnection(),
                          inArgs...));
    }


    template <typename _DBusStub = DBusStubAdapter>
       static bool sendSignal(const char *_target,
                       	   	  const _DBusStub &_stub,
                       	   	  const char *_name,
                       	   	  const char *_signature,
                       	   	  const _InArgs&... inArgs) {
           DBusMessage dbusMessage
           	   = DBusMessage::createSignal(
        		   _stub.getDBusAddress().getObjectPath().c_str(),
        		   _stub.getDBusAddress().getInterface().c_str(),
        		   _name,
        		   _signature);

           dbusMessage.setDestination(_target);

           if (sizeof...(_InArgs) > 0) {
               DBusOutputStream outputStream(dbusMessage);
               const bool success = DBusSerializableArguments<_InArgs...>::serialize(outputStream, inArgs...);
               if (!success) {
                   return false;
               }
               outputStream.flush();
           }

           return _stub.getDBusConnection()->sendDBusMessage(dbusMessage);
       }
};

template< class, class, class >
class DBusMethodStubDispatcher;

template <
    typename _StubClass,
    template <class...> class _In, class... _InArgs,
    template <class...> class _DeplIn, class... _DeplInArgs>

class DBusMethodStubDispatcher<_StubClass, _In<_InArgs...>, _DeplIn<_DeplInArgs...> >: public DBusStubAdapterHelper<_StubClass>::StubDispatcher {
 public:
    typedef DBusStubAdapterHelper<_StubClass> DBusStubAdapterHelperType;
    typedef void (_StubClass::*_StubFunctor)(std::shared_ptr<CommonAPI::ClientId>, _InArgs...);

    DBusMethodStubDispatcher(_StubFunctor stubFunctor, std::tuple<_DeplInArgs*...> _in):
            stubFunctor_(stubFunctor) {
            initialize(typename make_sequence_range<sizeof...(_DeplInArgs), 0>::type(), _in);
    }

    bool dispatchDBusMessage(const DBusMessage& dbusMessage, const std::shared_ptr<_StubClass>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
		return handleDBusMessage(dbusMessage, stub, dbusStubAdapterHelper, typename make_sequence_range<sizeof...(_InArgs), 0>::type());
    }

 private:
    template <int... _DeplInArgIndices>
    inline void initialize(index_sequence<_DeplInArgIndices...>, std::tuple<_DeplInArgs*...> &_in) {
	in_ = std::make_tuple(std::get<_DeplInArgIndices>(_in)...);
    }

    template <int... _InArgIndices>
    inline bool handleDBusMessage(const DBusMessage& dbusMessage,
                                  const std::shared_ptr<_StubClass>& stub,
                                  DBusStubAdapterHelperType& dbusStubAdapterHelper,
                                  index_sequence<_InArgIndices...>) {

        if (sizeof...(_InArgs) > 0) {
            DBusInputStream dbusInputStream(dbusMessage);
            const bool success = DBusSerializableArguments<CommonAPI::Deployable<_InArgs, _DeplInArgs>...>::deserialize(dbusInputStream, std::get<_InArgIndices>(in_)...);
            if (!success)
                return false;
        }

        std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSender()));

        (stub.get()->*stubFunctor_)(clientId, std::move(std::get<_InArgIndices>(in_).getValue())...);

        return true;
    }

    _StubFunctor stubFunctor_;
    std::tuple<CommonAPI::Deployable<_InArgs, _DeplInArgs>...> in_;
};


template< class, class, class, class, class>
class DBusMethodWithReplyStubDispatcher;

template <
    typename _StubClass,
    template <class...> class _In, class... _InArgs,
    template <class...> class _Out, class... _OutArgs,
    template <class...> class _DeplIn, class... _DeplInArgs,
    template <class...> class _DeplOut, class... _DeplOutArgs>

class DBusMethodWithReplyStubDispatcher<
       _StubClass, 
       _In<_InArgs...>,
       _Out<_OutArgs...>,
       _DeplIn<_DeplInArgs...>,
       _DeplOut<_DeplOutArgs...> >:
            public DBusStubAdapterHelper<_StubClass>::StubDispatcher {
 public:
    typedef DBusStubAdapterHelper<_StubClass> DBusStubAdapterHelperType;
    typedef std::function<void (_OutArgs...)> ReplyType_t;
    typedef void (_StubClass::*_StubFunctor)(
        		std::shared_ptr<CommonAPI::ClientId>, _InArgs..., ReplyType_t);

    DBusMethodWithReplyStubDispatcher(_StubFunctor stubFunctor,
        const char* dbusReplySignature, 
        std::tuple<_DeplInArgs*...> _inDepArgs,
        std::tuple<_DeplOutArgs*...> _outDepArgs):
            stubFunctor_(stubFunctor),
            dbusReplySignature_(dbusReplySignature),
            out_(_outDepArgs),
            currentCall_(0) {

    	initialize(typename make_sequence_range<sizeof...(_DeplInArgs), 0>::type(), _inDepArgs);

    }

    bool dispatchDBusMessage(const DBusMessage& dbusMessage, 
                             const std::shared_ptr<_StubClass>& stub, 
                             DBusStubAdapterHelperType& dbusStubAdapterHelper) {
    	connection_ = dbusStubAdapterHelper.getDBusConnection();
        return handleDBusMessage(
                        dbusMessage,
                        stub,
                        dbusStubAdapterHelper,
                        typename make_sequence_range<sizeof...(_InArgs), 0>::type(),
                        typename make_sequence_range<sizeof...(_OutArgs), 0>::type());
    }

	bool sendReply(CommonAPI::CallId_t _call, 
                       std::tuple<CommonAPI::Deployable<_OutArgs, _DeplOutArgs>...> args = std::make_tuple()) {
		return sendReplyInternal(_call, typename make_sequence_range<sizeof...(_OutArgs), 0>::type(), args);
	}

private:

	template <int... _DeplInArgIndices>
	inline void initialize(index_sequence<_DeplInArgIndices...>, std::tuple<_DeplInArgs*...> &_in) {
		in_ = std::make_tuple(std::get<_DeplInArgIndices>(_in)...);
	}


    template <int... _InArgIndices, int... _OutArgIndices>
    inline bool handleDBusMessage(const DBusMessage& dbusMessage,
                                  const std::shared_ptr<_StubClass>& stub,
                                  DBusStubAdapterHelperType& dbusStubAdapterHelper,
                                  index_sequence<_InArgIndices...>,
                                  index_sequence<_OutArgIndices...>) {
        if (sizeof...(_DeplInArgs) > 0) {
            DBusInputStream dbusInputStream(dbusMessage);
            const bool success = DBusSerializableArguments<CommonAPI::Deployable<_InArgs, _DeplInArgs>...>::deserialize(dbusInputStream, std::get<_InArgIndices>(in_)...);
            if (!success)
                return false;
        }

        std::shared_ptr<DBusClientId> clientId
        	= std::make_shared<DBusClientId>(std::string(dbusMessage.getSender()));
        DBusMessage reply = dbusMessage.createMethodReturn(dbusReplySignature_);

        CommonAPI::CallId_t call;
        {
        	std::lock_guard<std::mutex> lock(mutex_);
        	call = currentCall_++;
        	pending_[call] = reply;
        }

        (stub.get()->*stubFunctor_)(
        	clientId,
        	std::move(std::get<_InArgIndices>(in_).getValue())...,
        	[call, this](_OutArgs... _args){
        		this->sendReply(call, std::make_tuple(CommonAPI::Deployable<_OutArgs, _DeplOutArgs>(
        					_args, std::get<_OutArgIndices>(out_)
						)...));
        	}
        );

       	return true;
	}

    template<int... _OutArgIndices>
    bool sendReplyInternal(CommonAPI::CallId_t _call,
    					   index_sequence<_OutArgIndices...>,
    					   std::tuple<CommonAPI::Deployable<_OutArgs, _DeplOutArgs>...> args) {
		std::lock_guard<std::mutex> lock(mutex_);
		auto reply = pending_.find(_call);
		if (reply != pending_.end()) {
			if (sizeof...(_DeplOutArgs) > 0) {
				DBusOutputStream output(reply->second);
				if (!DBusSerializableArguments<CommonAPI::Deployable<_OutArgs, _DeplOutArgs>...>::serialize(
						output, std::get<_OutArgIndices>(args)...)) {
					pending_.erase(_call);
					return false;
				}
				output.flush();
			}
			bool isSuccessful = connection_->sendDBusMessage(reply->second);
			pending_.erase(_call);
			return isSuccessful;
		}
		return false;
	}

    _StubFunctor stubFunctor_;
    const char* dbusReplySignature_;

    std::tuple<CommonAPI::Deployable<_InArgs, _DeplInArgs>...> in_;
    std::tuple<_DeplOutArgs*...> out_;
	CommonAPI::CallId_t currentCall_;
	std::map<CommonAPI::CallId_t, DBusMessage> pending_;
	std::mutex mutex_; // protects pending_

	std::shared_ptr<DBusProxyConnection> connection_;
};

template< class, class, class, class >
class DBusMethodWithReplyAdapterDispatcher;

template <
    typename _StubClass,
    typename _StubAdapterClass,
    template <class...> class _In, class... _InArgs,
    template <class...> class _Out, class... _OutArgs>
class DBusMethodWithReplyAdapterDispatcher<_StubClass, _StubAdapterClass, _In<_InArgs...>, _Out<_OutArgs...> >:
            public DBusStubAdapterHelper<_StubClass>::StubDispatcher {
 public:
    typedef DBusStubAdapterHelper<_StubClass> DBusStubAdapterHelperType;
    typedef void (_StubAdapterClass::*_StubFunctor)(std::shared_ptr<CommonAPI::ClientId>, _InArgs..., _OutArgs&...);
    typedef typename CommonAPI::Stub<typename DBusStubAdapterHelperType::StubAdapterType, typename _StubClass::RemoteEventType> StubType;

    DBusMethodWithReplyAdapterDispatcher(_StubFunctor stubFunctor, const char* dbusReplySignature):
            stubFunctor_(stubFunctor),
            dbusReplySignature_(dbusReplySignature) {
    }

    bool dispatchDBusMessage(const DBusMessage& dbusMessage, const std::shared_ptr<_StubClass>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
        std::tuple<_InArgs..., _OutArgs...> argTuple;
        return handleDBusMessage(
                        dbusMessage,
                        stub,
                        dbusStubAdapterHelper,
                        typename make_sequence_range<sizeof...(_InArgs), 0>::type(),
                        typename make_sequence_range<sizeof...(_OutArgs), sizeof...(_InArgs)>::type(),argTuple);
    }

 private:
    template <int... _InArgIndices, int... _OutArgIndices>
    inline bool handleDBusMessage(const DBusMessage& dbusMessage,
                                  const std::shared_ptr<_StubClass>& stub,
                                  DBusStubAdapterHelperType& dbusStubAdapterHelper,
                                  index_sequence<_InArgIndices...>,
                                  index_sequence<_OutArgIndices...>,
                                  std::tuple<_InArgs..., _OutArgs...> argTuple) const {

        if (sizeof...(_InArgs) > 0) {
            DBusInputStream dbusInputStream(dbusMessage);
            const bool success = DBusSerializableArguments<_InArgs...>::deserialize(dbusInputStream, std::get<_InArgIndices>(argTuple)...);
            if (!success)
                return false;
        }

        std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSender()));

        (stub->StubType::getStubAdapter().get()->*stubFunctor_)(clientId, std::move(std::get<_InArgIndices>(argTuple))..., std::get<_OutArgIndices>(argTuple)...);
        DBusMessage dbusMessageReply = dbusMessage.createMethodReturn(dbusReplySignature_);

        if (sizeof...(_OutArgs) > 0) {
            DBusOutputStream dbusOutputStream(dbusMessageReply);
            const bool success = DBusSerializableArguments<_OutArgs...>::serialize(dbusOutputStream, std::get<_OutArgIndices>(argTuple)...);
            if (!success)
                return false;

            dbusOutputStream.flush();
        }

        return dbusStubAdapterHelper.getDBusConnection()->sendDBusMessage(dbusMessageReply);
    }

    _StubFunctor stubFunctor_;
    const char* dbusReplySignature_;
};


template <typename _StubClass, typename _AttributeType, typename _AttributeDepl = EmptyDeployment>
class DBusGetAttributeStubDispatcher: public virtual DBusStubAdapterHelper<_StubClass>::StubDispatcher {
 public:
    typedef DBusStubAdapterHelper<_StubClass> DBusStubAdapterHelperType;
    typedef const _AttributeType& (_StubClass::*GetStubFunctor)(std::shared_ptr<CommonAPI::ClientId>);

    DBusGetAttributeStubDispatcher(GetStubFunctor _getStubFunctor, const char *_signature, _AttributeDepl *_depl = nullptr):
        getStubFunctor_(_getStubFunctor),
        signature_(_signature),
        depl_(_depl) {
    }

    virtual ~DBusGetAttributeStubDispatcher() {};

    bool dispatchDBusMessage(const DBusMessage& dbusMessage, const std::shared_ptr<_StubClass>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
        return sendAttributeValueReply(dbusMessage, stub, dbusStubAdapterHelper);
    }

    void appendGetAllReply(const DBusMessage& dbusMessage, const std::shared_ptr<_StubClass>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper, DBusOutputStream &_output) {
        std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSender()));
        auto varDepl = CommonAPI::DBus::VariantDeployment<_AttributeDepl>(true, depl_); // presuming FreeDesktop variant deployment, as support for "legacy" service only
        _output << CommonAPI::Deployable<CommonAPI::Variant<_AttributeType>, CommonAPI::DBus::VariantDeployment<_AttributeDepl>>((stub.get()->*getStubFunctor_)(clientId), &varDepl);
        _output.flush();
    }

 protected:
    virtual bool sendAttributeValueReply(const DBusMessage& dbusMessage, const std::shared_ptr<_StubClass>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
        DBusMessage dbusMessageReply = dbusMessage.createMethodReturn(signature_);
        DBusOutputStream dbusOutputStream(dbusMessageReply);

        std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSender()));

        dbusOutputStream << CommonAPI::Deployable<_AttributeType, _AttributeDepl>((stub.get()->*getStubFunctor_)(clientId), depl_);
        dbusOutputStream.flush();

        return dbusStubAdapterHelper.getDBusConnection()->sendDBusMessage(dbusMessageReply);
    }


    GetStubFunctor getStubFunctor_;
    const char* signature_;
    _AttributeDepl *depl_;
};

template <typename _StubClass, typename _AttributeType, typename _AttributeDepl = EmptyDeployment>
class DBusSetAttributeStubDispatcher: public virtual DBusGetAttributeStubDispatcher<_StubClass, _AttributeType, _AttributeDepl> {
 public:
    typedef typename DBusGetAttributeStubDispatcher<_StubClass, _AttributeType, _AttributeDepl>::DBusStubAdapterHelperType DBusStubAdapterHelperType;
    typedef typename DBusStubAdapterHelperType::RemoteEventHandlerType RemoteEventHandlerType;

    typedef typename DBusGetAttributeStubDispatcher<_StubClass, _AttributeType, _AttributeDepl>::GetStubFunctor GetStubFunctor;
    typedef bool (RemoteEventHandlerType::*OnRemoteSetFunctor)(std::shared_ptr<CommonAPI::ClientId>, _AttributeType);
    typedef void (RemoteEventHandlerType::*OnRemoteChangedFunctor)();

    DBusSetAttributeStubDispatcher(GetStubFunctor getStubFunctor,
                                   OnRemoteSetFunctor onRemoteSetFunctor,
                                   OnRemoteChangedFunctor onRemoteChangedFunctor,
                                   const char* dbusSignature,
                                   _AttributeDepl *_depl = nullptr) :
                    DBusGetAttributeStubDispatcher<_StubClass, _AttributeType, _AttributeDepl>(getStubFunctor, dbusSignature, _depl),
                    onRemoteSetFunctor_(onRemoteSetFunctor),
                    onRemoteChangedFunctor_(onRemoteChangedFunctor) {
    }

    virtual ~DBusSetAttributeStubDispatcher() {};

    bool dispatchDBusMessage(const DBusMessage& dbusMessage, const std::shared_ptr<_StubClass>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
        bool attributeValueChanged;

        if (!setAttributeValue(dbusMessage, stub, dbusStubAdapterHelper, attributeValueChanged))
            return false;

        if (attributeValueChanged)
            notifyOnRemoteChanged(dbusStubAdapterHelper);

        return true;
    }

 protected:
    virtual _AttributeType retrieveAttributeValue(const DBusMessage& dbusMessage, bool& errorOccured) {
        errorOccured = false;

        DBusInputStream dbusInputStream(dbusMessage);
        CommonAPI::Deployable<_AttributeType, _AttributeDepl> attributeValue(this->depl_);
        dbusInputStream >> attributeValue;

        if (dbusInputStream.hasError()) {
            errorOccured = true;
        }

        return attributeValue.getValue();
    }

    inline bool setAttributeValue(const DBusMessage& dbusMessage,
                                  const std::shared_ptr<_StubClass>& stub,
                                  DBusStubAdapterHelperType& dbusStubAdapterHelper,
                                  bool& attributeValueChanged) {
        bool errorOccured;
        CommonAPI::Deployable<_AttributeType, _AttributeDepl> attributeValue(
        	 retrieveAttributeValue(dbusMessage, errorOccured), this->depl_);

        if(errorOccured) {
            return false;
        }

        std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSender()));

        attributeValueChanged = (dbusStubAdapterHelper.getRemoteEventHandler()->*onRemoteSetFunctor_)(clientId, std::move(attributeValue.getValue()));

        return this->sendAttributeValueReply(dbusMessage, stub, dbusStubAdapterHelper);
    }

    inline void notifyOnRemoteChanged(DBusStubAdapterHelperType& dbusStubAdapterHelper) {
        (dbusStubAdapterHelper.getRemoteEventHandler()->*onRemoteChangedFunctor_)();
    }

    inline const _AttributeType& getAttributeValue(std::shared_ptr<CommonAPI::ClientId> clientId, const std::shared_ptr<_StubClass>& stub) {
        return (stub.get()->*(this->getStubFunctor_))(clientId);
    }

    const OnRemoteSetFunctor onRemoteSetFunctor_;
    const OnRemoteChangedFunctor onRemoteChangedFunctor_;
};

template <typename _StubClass, typename _AttributeType, typename _AttributeDepl = EmptyDeployment>
class DBusSetObservableAttributeStubDispatcher: public virtual DBusSetAttributeStubDispatcher<_StubClass, _AttributeType, _AttributeDepl> {
 public:
    typedef typename DBusSetAttributeStubDispatcher<_StubClass, _AttributeType, _AttributeDepl>::DBusStubAdapterHelperType DBusStubAdapterHelperType;
    typedef typename DBusStubAdapterHelperType::StubAdapterType StubAdapterType;
    typedef typename DBusSetAttributeStubDispatcher<_StubClass, _AttributeType, _AttributeDepl>::GetStubFunctor GetStubFunctor;
    typedef typename DBusSetAttributeStubDispatcher<_StubClass, _AttributeType, _AttributeDepl>::OnRemoteSetFunctor OnRemoteSetFunctor;
    typedef typename DBusSetAttributeStubDispatcher<_StubClass, _AttributeType, _AttributeDepl>::OnRemoteChangedFunctor OnRemoteChangedFunctor;
    typedef typename CommonAPI::Stub<StubAdapterType, typename _StubClass::RemoteEventType> StubType;
    typedef void (StubAdapterType::*FireChangedFunctor)(const _AttributeType&);

    DBusSetObservableAttributeStubDispatcher(GetStubFunctor getStubFunctor,
                                             OnRemoteSetFunctor onRemoteSetFunctor,
                                             OnRemoteChangedFunctor onRemoteChangedFunctor,
                                             FireChangedFunctor fireChangedFunctor,
                                             const char* dbusSignature,
                                             _AttributeDepl *_depl = nullptr)
    	: DBusGetAttributeStubDispatcher<_StubClass, _AttributeType, _AttributeDepl>(
    			getStubFunctor, dbusSignature, _depl),
          DBusSetAttributeStubDispatcher<_StubClass, _AttributeType, _AttributeDepl>(
        		getStubFunctor, onRemoteSetFunctor, onRemoteChangedFunctor, dbusSignature, _depl),
          fireChangedFunctor_(fireChangedFunctor) {
    }

    virtual ~DBusSetObservableAttributeStubDispatcher() {};

    bool dispatchDBusMessage(const DBusMessage& dbusMessage, const std::shared_ptr<_StubClass>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
        bool attributeValueChanged;
        if (!this->setAttributeValue(dbusMessage, stub, dbusStubAdapterHelper, attributeValueChanged))
            return false;

        if (attributeValueChanged) {
            std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSender()));
            fireAttributeValueChanged(clientId, dbusStubAdapterHelper, stub);
            this->notifyOnRemoteChanged(dbusStubAdapterHelper);
        }
        return true;
    }
protected:
    virtual void fireAttributeValueChanged(std::shared_ptr<CommonAPI::ClientId> clientId,
                                           DBusStubAdapterHelperType& dbusStubAdapterHelper,
                                           const std::shared_ptr<_StubClass> stub) {
        (stub->StubType::getStubAdapter().get()->*fireChangedFunctor_)(this->getAttributeValue(clientId, stub));
    }

    const FireChangedFunctor fireChangedFunctor_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUSSTUBADAPTERHELPER_HPP_

