/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_STUB_ADAPTER_HELPER_H_
#define COMMONAPI_DBUS_DBUS_STUB_ADAPTER_HELPER_H_

#include "DBusStubAdapter.h"
#include "DBusInputStream.h"
#include "DBusOutputStream.h"
#include "DBusHelper.h"
#include "DBusSerializableArguments.h"
#include "DBusClientId.h"

#include <memory>
#include <initializer_list>
#include <tuple>
#include <unordered_map>

namespace CommonAPI {
namespace DBus {

template <typename _StubClass>
class DBusStubAdapterHelper: public DBusStubAdapter, public std::enable_shared_from_this<typename _StubClass::StubAdapterType> {
 public:
    typedef typename _StubClass::StubAdapterType StubAdapterType;
    typedef typename _StubClass::RemoteEventHandlerType RemoteEventHandlerType;

    class StubDispatcher {
     public:
        virtual ~StubDispatcher() { }
        virtual bool dispatchDBusMessage(const DBusMessage& dbusMessage, const std::shared_ptr<_StubClass>& stub, DBusStubAdapterHelper<_StubClass>& dbusStubAdapterHelper) = 0;
    };

 public:
    DBusStubAdapterHelper(const std::shared_ptr<DBusFactory>& factory,
                          const std::string& commonApiAddress,
                          const std::string& dbusInterfaceName,
                          const std::string& dbusBusName,
                          const std::string& dbusObjectPath,
                          const std::shared_ptr<DBusProxyConnection>& dbusConnection,
                          const std::shared_ptr<_StubClass>& stub,
                          const bool isManagingInterface):
                    DBusStubAdapter(factory, commonApiAddress, dbusInterfaceName, dbusBusName, dbusObjectPath, dbusConnection, isManagingInterface),
                    stub_(stub) {
    }

    virtual ~DBusStubAdapterHelper() {
        DBusStubAdapter::deinit();
        stub_.reset();
    }

    virtual void init() {
        DBusStubAdapter::init();
        remoteEventHandler_ = stub_->initStubAdapter(getStubAdapter());
    }

    virtual void deinit() {
        DBusStubAdapter::deinit();
        stub_.reset();
    }

    inline std::shared_ptr<StubAdapterType> getStubAdapter() {
        return this->shared_from_this();
    }

    inline RemoteEventHandlerType* getRemoteEventHandler() {
        return remoteEventHandler_;
    }

 protected:
    // interfaceMemberName, interfaceMemberSignature
    typedef std::pair<const char*, const char*> DBusInterfaceMemberPath;
    typedef std::unordered_map<DBusInterfaceMemberPath, StubDispatcher*> StubDispatcherTable;

    virtual bool onInterfaceDBusMessage(const DBusMessage& dbusMessage) {
        const char* interfaceMemberName = dbusMessage.getMemberName();
        const char* interfaceMemberSignature = dbusMessage.getSignatureString();

        assert(interfaceMemberName);
        assert(interfaceMemberSignature);

        DBusInterfaceMemberPath dbusInterfaceMemberPath(interfaceMemberName, interfaceMemberSignature);
        auto findIterator = getStubDispatcherTable().find(dbusInterfaceMemberPath);
        const bool foundInterfaceMemberHandler = (findIterator != getStubDispatcherTable().end());
        bool dbusMessageHandled = false;

        //To prevent the destruction of the stub whilst still handling a message
        auto stubSafety = stub_;
        if (stubSafety && foundInterfaceMemberHandler) {
            StubDispatcher* stubDispatcher = findIterator->second;
            dbusMessageHandled = stubDispatcher->dispatchDBusMessage(dbusMessage, stubSafety, *this);
        }

        return dbusMessageHandled;
    }

    virtual const StubDispatcherTable& getStubDispatcherTable() = 0;

    std::shared_ptr<_StubClass> stub_;
    RemoteEventHandlerType* remoteEventHandler_;
};

template< class >
struct DBusStubSignalHelper;

template<template<class ...> class _In, class... _InArgs>
struct DBusStubSignalHelper<_In<_InArgs...>> {
    template <typename _DBusStub = DBusStubAdapter>
    static bool sendSignal(const _DBusStub& dbusStub,
                    const char* signalName,
                    const char* signalSignature,
                    const _InArgs&... inArgs) {
        DBusMessage dbusMessage = DBusMessage::createSignal(
                        dbusStub.getObjectPath().c_str(),
                        dbusStub.getInterfaceName(),
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

        const bool dbusMessageSent = dbusStub.getDBusConnection()->sendDBusMessage(dbusMessage);
        return dbusMessageSent;
    }

    template <typename _DBusStub = DBusStubAdapter>
       static bool sendSignal( const char* target,
                       const _DBusStub& dbusStub,
                       const char* signalName,
                       const char* signalSignature,
                       const _InArgs&... inArgs) {
           DBusMessage dbusMessage = DBusMessage::createSignal(
                           dbusStub.getObjectPath().c_str(),
                           dbusStub.getInterfaceName(),
                           signalName,
                           signalSignature);

           dbusMessage.setDestination(target);

           if (sizeof...(_InArgs) > 0) {
               DBusOutputStream outputStream(dbusMessage);
               const bool success = DBusSerializableArguments<_InArgs...>::serialize(outputStream, inArgs...);
               if (!success) {
                   return false;
               }
               outputStream.flush();
           }

           const bool dbusMessageSent = dbusStub.getDBusConnection()->sendDBusMessage(dbusMessage);
           return dbusMessageSent;
       }
};



template< class, class >
class DBusMethodStubDispatcher;

template <
    typename _StubClass,
    template <class...> class _In, class... _InArgs>
class DBusMethodStubDispatcher<_StubClass, _In<_InArgs...> >: public DBusStubAdapterHelper<_StubClass>::StubDispatcher {
 public:
    typedef DBusStubAdapterHelper<_StubClass> DBusStubAdapterHelperType;
    typedef void (_StubClass::*_StubFunctor)(std::shared_ptr<CommonAPI::ClientId>, _InArgs...);

    DBusMethodStubDispatcher(_StubFunctor stubFunctor):
            stubFunctor_(stubFunctor) {
    }

    bool dispatchDBusMessage(const DBusMessage& dbusMessage, const std::shared_ptr<_StubClass>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
        return handleDBusMessage(dbusMessage, stub, dbusStubAdapterHelper, typename make_sequence<sizeof...(_InArgs)>::type());
    }

 private:
    template <int... _InArgIndices, int... _OutArgIndices>
    inline bool handleDBusMessage(const DBusMessage& dbusMessage,
                                  const std::shared_ptr<_StubClass>& stub,
                                  DBusStubAdapterHelperType& dbusStubAdapterHelper,
                                  index_sequence<_InArgIndices...>) const {
        std::tuple<_InArgs...> argTuple;

        if (sizeof...(_InArgs) > 0) {
            DBusInputStream dbusInputStream(dbusMessage);
            const bool success = DBusSerializableArguments<_InArgs...>::deserialize(dbusInputStream, std::get<_InArgIndices>(argTuple)...);
            if (!success)
                return false;
        }

        std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSenderName()));

        (stub.get()->*stubFunctor_)(clientId, std::move(std::get<_InArgIndices>(argTuple))...);

        return true;
    }

    _StubFunctor stubFunctor_;
};


template< class, class, class >
class DBusMethodWithReplyStubDispatcher;

template <
    typename _StubClass,
    template <class...> class _In, class... _InArgs,
    template <class...> class _Out, class... _OutArgs>
class DBusMethodWithReplyStubDispatcher<_StubClass, _In<_InArgs...>, _Out<_OutArgs...> >:
            public DBusStubAdapterHelper<_StubClass>::StubDispatcher {
 public:
    typedef DBusStubAdapterHelper<_StubClass> DBusStubAdapterHelperType;
    typedef void (_StubClass::*_StubFunctor)(std::shared_ptr<CommonAPI::ClientId>, _InArgs..., _OutArgs&...);

    DBusMethodWithReplyStubDispatcher(_StubFunctor stubFunctor, const char* dbusReplySignature):
            stubFunctor_(stubFunctor),
            dbusReplySignature_(dbusReplySignature) {
    }

    bool dispatchDBusMessage(const DBusMessage& dbusMessage, const std::shared_ptr<_StubClass>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
        return handleDBusMessage(
                        dbusMessage,
                        stub,
                        dbusStubAdapterHelper,
                        typename make_sequence_range<sizeof...(_InArgs), 0>::type(),
                        typename make_sequence_range<sizeof...(_OutArgs), sizeof...(_InArgs)>::type());
    }

 private:
    template <int... _InArgIndices, int... _OutArgIndices>
    inline bool handleDBusMessage(const DBusMessage& dbusMessage,
                                  const std::shared_ptr<_StubClass>& stub,
                                  DBusStubAdapterHelperType& dbusStubAdapterHelper,
                                  index_sequence<_InArgIndices...>,
                                  index_sequence<_OutArgIndices...>) const {
        std::tuple<_InArgs..., _OutArgs...> argTuple;

        if (sizeof...(_InArgs) > 0) {
            DBusInputStream dbusInputStream(dbusMessage);
            const bool success = DBusSerializableArguments<_InArgs...>::deserialize(dbusInputStream, std::get<_InArgIndices>(argTuple)...);
            if (!success)
                return false;
        }

        std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSenderName()));

        (stub.get()->*stubFunctor_)(clientId, std::move(std::get<_InArgIndices>(argTuple))..., std::get<_OutArgIndices>(argTuple)...);

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

    DBusMethodWithReplyAdapterDispatcher(_StubFunctor stubFunctor, const char* dbusReplySignature):
            stubFunctor_(stubFunctor),
            dbusReplySignature_(dbusReplySignature) {
    }

    bool dispatchDBusMessage(const DBusMessage& dbusMessage, const std::shared_ptr<_StubClass>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
        return handleDBusMessage(
                        dbusMessage,
                        stub,
                        dbusStubAdapterHelper,
                        typename make_sequence_range<sizeof...(_InArgs), 0>::type(),
                        typename make_sequence_range<sizeof...(_OutArgs), sizeof...(_InArgs)>::type());
    }

 private:
    template <int... _InArgIndices, int... _OutArgIndices>
    inline bool handleDBusMessage(const DBusMessage& dbusMessage,
                                  const std::shared_ptr<_StubClass>& stub,
                                  DBusStubAdapterHelperType& dbusStubAdapterHelper,
                                  index_sequence<_InArgIndices...>,
                                  index_sequence<_OutArgIndices...>) const {
        std::tuple<_InArgs..., _OutArgs...> argTuple;

        if (sizeof...(_InArgs) > 0) {
            DBusInputStream dbusInputStream(dbusMessage);
            const bool success = DBusSerializableArguments<_InArgs...>::deserialize(dbusInputStream, std::get<_InArgIndices>(argTuple)...);
            if (!success)
                return false;
        }

        std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSenderName()));

        (dbusStubAdapterHelper.getStubAdapter().get()->*stubFunctor_)(clientId, std::move(std::get<_InArgIndices>(argTuple))..., std::get<_OutArgIndices>(argTuple)...);
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


template <typename _StubClass, typename _AttributeType>
class DBusGetAttributeStubDispatcher: public DBusStubAdapterHelper<_StubClass>::StubDispatcher {
 public:
    typedef DBusStubAdapterHelper<_StubClass> DBusStubAdapterHelperType;
    typedef const _AttributeType& (_StubClass::*GetStubFunctor)(std::shared_ptr<CommonAPI::ClientId>);

    DBusGetAttributeStubDispatcher(GetStubFunctor getStubFunctor, const char* dbusSignature):
        getStubFunctor_(getStubFunctor),
        dbusSignature_(dbusSignature) {
    }

    bool dispatchDBusMessage(const DBusMessage& dbusMessage, const std::shared_ptr<_StubClass>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
        return sendAttributeValueReply(dbusMessage, stub, dbusStubAdapterHelper);
    }

 protected:
    inline bool sendAttributeValueReply(const DBusMessage& dbusMessage, const std::shared_ptr<_StubClass>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
        DBusMessage dbusMessageReply = dbusMessage.createMethodReturn(dbusSignature_);
        DBusOutputStream dbusOutputStream(dbusMessageReply);

        std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSenderName()));

        dbusOutputStream << (stub.get()->*getStubFunctor_)(clientId);
        dbusOutputStream.flush();

        return dbusStubAdapterHelper.getDBusConnection()->sendDBusMessage(dbusMessageReply);
    }

    GetStubFunctor getStubFunctor_;
    const char* dbusSignature_;
};


template <typename _StubClass, typename _AttributeType>
class DBusSetAttributeStubDispatcher: public DBusGetAttributeStubDispatcher<_StubClass, _AttributeType> {
 public:
    typedef typename DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>::DBusStubAdapterHelperType DBusStubAdapterHelperType;
    typedef typename DBusStubAdapterHelperType::RemoteEventHandlerType RemoteEventHandlerType;

    typedef typename DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>::GetStubFunctor GetStubFunctor;
    typedef bool (RemoteEventHandlerType::*OnRemoteSetFunctor)(std::shared_ptr<CommonAPI::ClientId>, _AttributeType);
    typedef void (RemoteEventHandlerType::*OnRemoteChangedFunctor)();

    DBusSetAttributeStubDispatcher(GetStubFunctor getStubFunctor,
                                   OnRemoteSetFunctor onRemoteSetFunctor,
                                   OnRemoteChangedFunctor onRemoteChangedFunctor,
                                   const char* dbusSignature) :
                    DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>(getStubFunctor, dbusSignature),
                    onRemoteSetFunctor_(onRemoteSetFunctor),
                    onRemoteChangedFunctor_(onRemoteChangedFunctor) {
    }

    bool dispatchDBusMessage(const DBusMessage& dbusMessage, const std::shared_ptr<_StubClass>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
        bool attributeValueChanged;

        if (!setAttributeValue(dbusMessage, stub, dbusStubAdapterHelper, attributeValueChanged))
            return false;

        if (attributeValueChanged)
            notifyOnRemoteChanged(dbusStubAdapterHelper);

        return true;
    }

 protected:
    inline bool setAttributeValue(const DBusMessage& dbusMessage,
                                  const std::shared_ptr<_StubClass>& stub,
                                  DBusStubAdapterHelperType& dbusStubAdapterHelper,
                                  bool& attributeValueChanged) {
        DBusInputStream dbusInputStream(dbusMessage);
        _AttributeType attributeValue;
        dbusInputStream >> attributeValue;
        if (dbusInputStream.hasError())
            return false;

        std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSenderName()));

        attributeValueChanged = (dbusStubAdapterHelper.getRemoteEventHandler()->*onRemoteSetFunctor_)(clientId, std::move(attributeValue));

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


template <typename _StubClass, typename _AttributeType>
class DBusSetObservableAttributeStubDispatcher: public DBusSetAttributeStubDispatcher<_StubClass, _AttributeType> {
 public:
    typedef typename DBusSetAttributeStubDispatcher<_StubClass, _AttributeType>::DBusStubAdapterHelperType DBusStubAdapterHelperType;
    typedef typename DBusStubAdapterHelperType::StubAdapterType StubAdapterType;

    typedef typename DBusSetAttributeStubDispatcher<_StubClass, _AttributeType>::GetStubFunctor GetStubFunctor;
    typedef typename DBusSetAttributeStubDispatcher<_StubClass, _AttributeType>::OnRemoteSetFunctor OnRemoteSetFunctor;
    typedef typename DBusSetAttributeStubDispatcher<_StubClass, _AttributeType>::OnRemoteChangedFunctor OnRemoteChangedFunctor;
    typedef void (StubAdapterType::*FireChangedFunctor)(const _AttributeType&);

    DBusSetObservableAttributeStubDispatcher(GetStubFunctor getStubFunctor,
                                             OnRemoteSetFunctor onRemoteSetFunctor,
                                             OnRemoteChangedFunctor onRemoteChangedFunctor,
                                             FireChangedFunctor fireChangedFunctor,
                                             const char* dbusSignature) :
                    DBusSetAttributeStubDispatcher<_StubClass, _AttributeType>(getStubFunctor,
                                                                               onRemoteSetFunctor,
                                                                               onRemoteChangedFunctor,
                                                                               dbusSignature),
                    fireChangedFunctor_(fireChangedFunctor) {
    }

    bool dispatchDBusMessage(const DBusMessage& dbusMessage, const std::shared_ptr<_StubClass>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
        bool attributeValueChanged;
        if (!this->setAttributeValue(dbusMessage, stub, dbusStubAdapterHelper, attributeValueChanged))
            return false;

        if (attributeValueChanged) {
            std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSenderName()));
            fireAttributeValueChanged(clientId, dbusStubAdapterHelper, stub);
            this->notifyOnRemoteChanged(dbusStubAdapterHelper);
        }
        return true;
    }

 private:
    inline void fireAttributeValueChanged(std::shared_ptr<CommonAPI::ClientId> clientId, DBusStubAdapterHelperType& dbusStubAdapterHelper, const std::shared_ptr<_StubClass> stub) {
        (dbusStubAdapterHelper.getStubAdapter().get()->*fireChangedFunctor_)(this->getAttributeValue(clientId, stub));
    }

    const FireChangedFunctor fireChangedFunctor_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_STUB_ADAPTER_HELPER_H_
