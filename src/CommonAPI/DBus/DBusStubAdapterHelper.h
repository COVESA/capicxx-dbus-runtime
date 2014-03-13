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

#include <CommonAPI/SerializableVariant.h>

#include "DBusStubAdapter.h"
#include "DBusInputStream.h"
#include "DBusOutputStream.h"
#include "DBusHelper.h"
#include "DBusSerializableArguments.h"
#include "DBusClientId.h"
#include "DBusLegacyVariant.h"

#include <memory>
#include <initializer_list>
#include <tuple>
#include <unordered_map>

namespace CommonAPI {
namespace DBus {

class StubDispatcherBase {
public:
   virtual ~StubDispatcherBase() { }
};

template <typename _StubClass>
class DBusGetFreedesktopAttributeStubDispatcherBase;

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
        virtual bool dispatchDBusMessage(const DBusMessage& dbusMessage,
                                         const std::shared_ptr<_StubClass>& stub,
                                         DBusStubAdapterHelper<_StubClass>& dbusStubAdapterHelper) = 0;
    };
    // interfaceMemberName, interfaceMemberSignature
    typedef std::pair<const char*, const char*> DBusInterfaceMemberPath;
    typedef std::unordered_map<DBusInterfaceMemberPath, StubDispatcherBase*> StubDispatcherTable;

    DBusStubAdapterHelper(const std::shared_ptr<DBusFactory>& factory,
                          const std::string& commonApiAddress,
                          const std::string& dbusInterfaceName,
                          const std::string& dbusBusName,
                          const std::string& dbusObjectPath,
                          const std::shared_ptr<DBusProxyConnection>& dbusConnection,
                          const std::shared_ptr<_StubClass>& stub,
                          const bool isManagingInterface):
                    DBusStubAdapter(factory, commonApiAddress, dbusInterfaceName, dbusBusName, dbusObjectPath, dbusConnection, isManagingInterface),
                    stub_(stub),
                    remoteEventHandler_(NULL) {
    }

    virtual ~DBusStubAdapterHelper() {
        DBusStubAdapter::deinit();
        stub_.reset();
    }

    virtual void init(std::shared_ptr<DBusStubAdapter> instance) {
        DBusStubAdapter::init(instance);
        std::shared_ptr<StubAdapterType> stubAdapter = std::dynamic_pointer_cast<StubAdapterType>(instance);
        remoteEventHandler_ = stub_.lock()->initStubAdapter(stubAdapter);
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
        const char* interfaceMemberName = dbusMessage.getMemberName();
        const char* interfaceMemberSignature = dbusMessage.getSignatureString();

        assert(interfaceMemberName);
        assert(interfaceMemberSignature);

        DBusInterfaceMemberPath dbusInterfaceMemberPath(interfaceMemberName, interfaceMemberSignature);
        auto findIterator = getStubDispatcherTable().find(dbusInterfaceMemberPath);
        const bool foundInterfaceMemberHandler = (findIterator != getStubDispatcherTable().end());
        bool dbusMessageHandled = false;

        //To prevent the destruction of the stub whilst still handling a message
        auto stubSafety = stub_.lock();
        if (stubSafety && foundInterfaceMemberHandler) {
            StubDispatcher* stubDispatcher = static_cast<StubDispatcher*>(findIterator->second);
            dbusMessageHandled = stubDispatcher->dispatchDBusMessage(dbusMessage, stubSafety, *this);
        }

        return dbusMessageHandled;
    }

    virtual bool onInterfaceDBusFreedesktopPropertiesMessage(const DBusMessage& dbusMessage) {
        DBusInputStream dbusInputStream(dbusMessage);

        if(dbusMessage.hasMemberName("Get")) {
            return handleFreedesktopGet(dbusMessage, dbusInputStream);
        } else if(dbusMessage.hasMemberName("Set")) {
            return handleFreedesktopSet(dbusMessage, dbusInputStream);
        } else if(dbusMessage.hasMemberName("GetAll")) {
            return handleFreedesktopGetAll(dbusMessage, dbusInputStream);
        }

        return false;
    }

    virtual const StubDispatcherTable& getStubDispatcherTable() = 0;
    virtual const StubAttributeTable& getStubAttributeTable() = 0;

    std::weak_ptr<_StubClass> stub_;
    RemoteEventHandlerType* remoteEventHandler_;
 private:
    bool handleFreedesktopGet(const DBusMessage& dbusMessage, DBusInputStream& dbusInputStream) {
        std::string interfaceName;
        std::string attributeName;
        dbusInputStream >> interfaceName;
        dbusInputStream >> attributeName;

        if (dbusInputStream.hasError()) {
            return false;
        }

        auto attributeDispatcherIterator = getStubAttributeTable().find(attributeName);
        // check, if we want to access with a valid attribute name
        if (attributeDispatcherIterator == getStubAttributeTable().end()) {
            return false;
        }

        //To prevent the destruction of the stub whilst still handling a message
        auto stubSafety = stub_.lock();
        if (stubSafety) {
            StubDispatcher* getterDispatcher = static_cast<StubDispatcher*>(attributeDispatcherIterator->second.getter);
            assert(getterDispatcher != NULL); // all attributes have at least a getter
            return (getterDispatcher->dispatchDBusMessage(dbusMessage, stubSafety, *this));
        }

        return false;
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
        // check, if we want to access with a valid attribute name
        if(attributeDispatcherIterator == getStubAttributeTable().end()) {
            return false;
        }

        //To prevent the destruction of the stub whilst still handling a message
        auto stubSafety = stub_.lock();
        if (stubSafety) {
            StubDispatcher* setterDispatcher = static_cast<StubDispatcher*>(attributeDispatcherIterator->second.setter);

            if(setterDispatcher == NULL) { // readonly attributes do not have a setter
                return false;
            }

            return(setterDispatcher->dispatchDBusMessage(dbusMessage, stubSafety, *this));
        }

        return false;
    }

    bool handleFreedesktopGetAll(const DBusMessage& dbusMessage, DBusInputStream& dbusInputStream) {
        std::string interfaceName;
        dbusInputStream >> interfaceName;

        if(dbusInputStream.hasError()) {
            return false;
        }

        DBusMessage dbusMessageReply = dbusMessage.createMethodReturn("a{sv}");
        DBusOutputStream dbusOutputStream(dbusMessageReply);
        dbusOutputStream.beginWriteVectorOfSerializableStructs(getStubAttributeTable().size());

        std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSenderName()));

        auto stubSafety = stub_.lock();

        for(auto attributeDispatcherIterator = getStubAttributeTable().begin(); attributeDispatcherIterator != getStubAttributeTable().end(); attributeDispatcherIterator++) {
            //To prevent the destruction of the stub whilst still handling a message
            if (stubSafety) {
                DBusGetFreedesktopAttributeStubDispatcherBase<_StubClass>* const getterDispatcher = dynamic_cast<DBusGetFreedesktopAttributeStubDispatcherBase<_StubClass>*>(attributeDispatcherIterator->second.getter);

                if(getterDispatcher == NULL) { // readonly attributes do not have a setter
                    return false;
                }

                dbusOutputStream << attributeDispatcherIterator->first;
                getterDispatcher->dispatchDBusMessageAndAppendReply(dbusMessage, stubSafety, dbusOutputStream, clientId);
            }
        }

        dbusOutputStream.endWriteVector();
        dbusOutputStream.flush();
        return getDBusConnection()->sendDBusMessage(dbusMessageReply);
    }
};

template< class >
struct DBusStubSignalHelper;

template<template<class ...> class _In, class... _InArgs>
struct DBusStubSignalHelper<_In<_InArgs...>> {

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
    static bool sendSignal(const _DBusStub& dbusStub,
                    const char* signalName,
                    const char* signalSignature,
                    const _InArgs&... inArgs) {
        return(sendSignal(dbusStub.getObjectPath().c_str(),
                          dbusStub.getInterfaceName().c_str(),
                          signalName,
                          signalSignature,
                          dbusStub.getDBusConnection(),
                          inArgs...));
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

template< class >
struct DBusStubFreedesktopPropertiesSignalHelper;

template<template<class ...> class _In, class _InArg>
struct DBusStubFreedesktopPropertiesSignalHelper<_In<_InArg>> {
    template <typename _ValueType>
    struct DBusPropertiesEntry: public CommonAPI::SerializableStruct {
        std::string propertyName_;
        DBusLegacyVariantWrapper<CommonAPI::Variant<_ValueType>> propertyValue_;

        DBusPropertiesEntry() = default;
        DBusPropertiesEntry(const std::string& propertyName,
                            const DBusLegacyVariantWrapper<CommonAPI::Variant<_ValueType>>& propertyValue):
                                propertyName_(propertyName),
                                propertyValue_(propertyValue) {
        };

        virtual void readFromInputStream(CommonAPI::InputStream& inputStream) {
            inputStream >> propertyName_;
            inputStream >> propertyValue_;
        }

        virtual void writeToOutputStream(CommonAPI::OutputStream& outputStream) const {
            outputStream << propertyName_;
            outputStream << propertyValue_;
        }

        static inline void writeToTypeOutputStream(CommonAPI::TypeOutputStream& typeOutputStream) {
            typeOutputStream.writeStringType();
            typeOutputStream.writeVariantType();
        }

    };

    template <typename _DBusStub = DBusStubAdapter>
    static bool sendPropertiesChangedSignal(const _DBusStub& dbusStub, const std::string& propertyName, const _InArg& inArg) {
        const std::vector<std::string> invalidatedProperties;
        const std::vector<DBusPropertiesEntry<_InArg>> changedProperties = {wrapValue(propertyName, inArg)};

        return DBusStubSignalHelper<_In<const std::string, std::vector<DBusPropertiesEntry<_InArg>>, std::vector<std::string>>>::
                        sendSignal(dbusStub.getObjectPath().c_str(),
                                   "org.freedesktop.DBus.Properties",
                                   "PropertiesChanged",
                                   "sa{sv}as",
                                   dbusStub.getDBusConnection(),
                                   dbusStub.getInterfaceName(),
                                   changedProperties,
                                   invalidatedProperties);
    }
private:
    template <typename _ValueType>
    static DBusPropertiesEntry<_ValueType> wrapValue(const std::string& propertyName, _ValueType value) {
        CommonAPI::Variant<_ValueType> returnVariant(value);

        DBusLegacyVariantWrapper<CommonAPI::Variant<_ValueType>> wrappedReturnVariant;
        wrappedReturnVariant.contained_ = returnVariant;

        DBusPropertiesEntry<_ValueType> returnEntry(propertyName, wrappedReturnVariant);

        return returnEntry;
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
    template <int... _InArgIndices>
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

        std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSenderName()));

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


template <typename _StubClass, typename _AttributeType>
class DBusGetAttributeStubDispatcher: public virtual DBusStubAdapterHelper<_StubClass>::StubDispatcher {
 public:
    typedef DBusStubAdapterHelper<_StubClass> DBusStubAdapterHelperType;
    typedef const _AttributeType& (_StubClass::*GetStubFunctor)(std::shared_ptr<CommonAPI::ClientId>);

    DBusGetAttributeStubDispatcher(GetStubFunctor getStubFunctor, const char* dbusSignature):
        getStubFunctor_(getStubFunctor),
        dbusSignature_(dbusSignature) {
    }

    virtual ~DBusGetAttributeStubDispatcher() {};

    bool dispatchDBusMessage(const DBusMessage& dbusMessage, const std::shared_ptr<_StubClass>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
        return sendAttributeValueReply(dbusMessage, stub, dbusStubAdapterHelper);
    }
 protected:
    virtual bool sendAttributeValueReply(const DBusMessage& dbusMessage, const std::shared_ptr<_StubClass>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
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

template <typename _StubClass>
class DBusGetFreedesktopAttributeStubDispatcherBase {
public:
    virtual ~DBusGetFreedesktopAttributeStubDispatcherBase() {}
    virtual void dispatchDBusMessageAndAppendReply(const DBusMessage& dbusMessage, const std::shared_ptr<_StubClass>& stub, DBusOutputStream& dbusOutputStream, const std::shared_ptr<DBusClientId>& clientId) = 0;
};

template <typename _StubClass, typename _AttributeType>
class DBusGetFreedesktopAttributeStubDispatcher: public virtual DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>, public virtual DBusGetFreedesktopAttributeStubDispatcherBase<_StubClass> {
public:
    typedef DBusStubAdapterHelper<_StubClass> DBusStubAdapterHelperType;
    typedef typename DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>::GetStubFunctor GetStubFunctor;
    DBusGetFreedesktopAttributeStubDispatcher(GetStubFunctor getStubFunctor) :
                    DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>(getStubFunctor, "v") {
    }

    virtual ~DBusGetFreedesktopAttributeStubDispatcher() {};

    void dispatchDBusMessageAndAppendReply(const DBusMessage& dbusMessage, const std::shared_ptr<_StubClass>& stub, DBusOutputStream& dbusOutputStream, const std::shared_ptr<DBusClientId>& clientId) {
        CommonAPI::Variant<_AttributeType> returnVariant((stub.get()->*(DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>::getStubFunctor_))(clientId));

        DBusLegacyVariantWrapper<CommonAPI::Variant<_AttributeType>> wrappedReturnVariant;
        wrappedReturnVariant.contained_ = returnVariant;

        dbusOutputStream << wrappedReturnVariant;
    }
protected:
   virtual bool sendAttributeValueReply(const DBusMessage& dbusMessage, const std::shared_ptr<_StubClass>& stub, DBusStubAdapterHelperType& dbusStubAdapterHelper) {
       DBusMessage dbusMessageReply = dbusMessage.createMethodReturn(DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>::dbusSignature_);
       DBusOutputStream dbusOutputStream(dbusMessageReply);

       std::shared_ptr<DBusClientId> clientId = std::make_shared<DBusClientId>(std::string(dbusMessage.getSenderName()));

       CommonAPI::Variant<_AttributeType> returnVariant((stub.get()->*(DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>::getStubFunctor_))(clientId));

       DBusLegacyVariantWrapper<CommonAPI::Variant<_AttributeType>> wrappedReturnVariant;
       wrappedReturnVariant.contained_ = returnVariant;
       dbusOutputStream << wrappedReturnVariant;
       dbusOutputStream.flush();

       return dbusStubAdapterHelper.getDBusConnection()->sendDBusMessage(dbusMessageReply);
   }
};


template <typename _StubClass, typename _AttributeType>
class DBusSetAttributeStubDispatcher: public virtual DBusGetAttributeStubDispatcher<_StubClass, _AttributeType> {
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
    virtual _AttributeType retreiveAttributeValue(const DBusMessage& dbusMessage, bool& errorOccured) {
        errorOccured = false;

        DBusInputStream dbusInputStream(dbusMessage);
        _AttributeType attributeValue;
        dbusInputStream >> attributeValue;

        if (dbusInputStream.hasError()) {
            errorOccured = true;
        }

        return attributeValue;
    }

    inline bool setAttributeValue(const DBusMessage& dbusMessage,
                                  const std::shared_ptr<_StubClass>& stub,
                                  DBusStubAdapterHelperType& dbusStubAdapterHelper,
                                  bool& attributeValueChanged) {
        bool errorOccured;
        _AttributeType attributeValue = retreiveAttributeValue(dbusMessage, errorOccured);

        if(errorOccured) {
            return false;
        }

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
class DBusSetFreedesktopAttributeStubDispatcher: public virtual DBusGetFreedesktopAttributeStubDispatcher<_StubClass, _AttributeType>, public virtual DBusSetAttributeStubDispatcher<_StubClass, _AttributeType> {
public:
    typedef typename DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>::GetStubFunctor GetStubFunctor;
    typedef typename DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>::DBusStubAdapterHelperType DBusStubAdapterHelperType;
    typedef typename DBusStubAdapterHelperType::RemoteEventHandlerType RemoteEventHandlerType;
    typedef bool (RemoteEventHandlerType::*OnRemoteSetFunctor)(std::shared_ptr<CommonAPI::ClientId>, _AttributeType);
    typedef void (RemoteEventHandlerType::*OnRemoteChangedFunctor)();

    DBusSetFreedesktopAttributeStubDispatcher(GetStubFunctor getStubFunctor,
                                              OnRemoteSetFunctor onRemoteSetFunctor,
                                              OnRemoteChangedFunctor onRemoteChangedFunctor) :
                    DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>(
                                    getStubFunctor,
                                    "v"),
                    DBusGetFreedesktopAttributeStubDispatcher<_StubClass, _AttributeType>(
                                    getStubFunctor),
                    DBusSetAttributeStubDispatcher<_StubClass, _AttributeType>(
                                    getStubFunctor,
                                    onRemoteSetFunctor,
                                    onRemoteChangedFunctor,
                                    "v") {
    }

    virtual ~DBusSetFreedesktopAttributeStubDispatcher() {};
protected:
    virtual _AttributeType retreiveAttributeValue(const DBusMessage& dbusMessage, bool& errorOccured) {
        errorOccured = false;
        std::string interfaceName;
        std::string attributeName;

        DBusInputStream dbusInputStream(dbusMessage);
        DBusLegacyVariantWrapper<CommonAPI::Variant<_AttributeType>> variantValue;
        dbusInputStream >> interfaceName; // skip over interface and attribute name
        dbusInputStream >> attributeName;
        dbusInputStream >> variantValue;
        _AttributeType attributeValue = variantValue.contained_.template get<_AttributeType>() ;

        if (dbusInputStream.hasError()) {
            errorOccured = true;
        }

        return attributeValue;
    }
};

template <typename _StubClass, typename _AttributeType>
class DBusSetObservableAttributeStubDispatcher: public virtual DBusSetAttributeStubDispatcher<_StubClass, _AttributeType> {
 public:
    typedef typename DBusSetAttributeStubDispatcher<_StubClass, _AttributeType>::DBusStubAdapterHelperType DBusStubAdapterHelperType;
    typedef typename DBusStubAdapterHelperType::StubAdapterType StubAdapterType;
    typedef typename DBusSetAttributeStubDispatcher<_StubClass, _AttributeType>::GetStubFunctor GetStubFunctor;
    typedef typename DBusSetAttributeStubDispatcher<_StubClass, _AttributeType>::OnRemoteSetFunctor OnRemoteSetFunctor;
    typedef typename DBusSetAttributeStubDispatcher<_StubClass, _AttributeType>::OnRemoteChangedFunctor OnRemoteChangedFunctor;
    typedef typename CommonAPI::Stub<StubAdapterType, typename _StubClass::RemoteEventType> StubType;
    typedef void (StubAdapterType::*FireChangedFunctor)(const _AttributeType&);

    DBusSetObservableAttributeStubDispatcher(GetStubFunctor getStubFunctor,
                                             OnRemoteSetFunctor onRemoteSetFunctor,
                                             OnRemoteChangedFunctor onRemoteChangedFunctor,
                                             FireChangedFunctor fireChangedFunctor,
                                             const char* dbusSignature) :
                    DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>(getStubFunctor, dbusSignature),
                    DBusSetAttributeStubDispatcher<_StubClass, _AttributeType>(getStubFunctor,
                                    onRemoteSetFunctor,
                                    onRemoteChangedFunctor,
                                    dbusSignature),
                    fireChangedFunctor_(fireChangedFunctor) {
    }

    virtual ~DBusSetObservableAttributeStubDispatcher() {};

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
protected:
    virtual void fireAttributeValueChanged(std::shared_ptr<CommonAPI::ClientId> clientId,
                                           DBusStubAdapterHelperType& dbusStubAdapterHelper,
                                           const std::shared_ptr<_StubClass> stub) {
        (stub->StubType::getStubAdapter().get()->*fireChangedFunctor_)(this->getAttributeValue(clientId, stub));
    }

    const FireChangedFunctor fireChangedFunctor_;
};

template <typename _StubClass, typename _AttributeType>
class DBusSetFreedesktopObservableAttributeStubDispatcher: public virtual DBusSetFreedesktopAttributeStubDispatcher<_StubClass, _AttributeType>, public virtual DBusSetObservableAttributeStubDispatcher<_StubClass, _AttributeType> {
public:
    typedef typename DBusSetFreedesktopAttributeStubDispatcher<_StubClass, _AttributeType>::DBusStubAdapterHelperType DBusStubAdapterHelperType;
    typedef typename DBusStubAdapterHelperType::StubAdapterType StubAdapterType;
    typedef typename DBusSetFreedesktopAttributeStubDispatcher<_StubClass, _AttributeType>::GetStubFunctor GetStubFunctor;
    typedef typename DBusSetFreedesktopAttributeStubDispatcher<_StubClass, _AttributeType>::OnRemoteSetFunctor OnRemoteSetFunctor;
    typedef typename DBusSetFreedesktopAttributeStubDispatcher<_StubClass, _AttributeType>::OnRemoteChangedFunctor OnRemoteChangedFunctor;
    typedef void (StubAdapterType::*FireChangedFunctor)(const _AttributeType&);

    DBusSetFreedesktopObservableAttributeStubDispatcher(GetStubFunctor getStubFunctor,
                                             OnRemoteSetFunctor onRemoteSetFunctor,
                                             OnRemoteChangedFunctor onRemoteChangedFunctor,
                                             FireChangedFunctor fireChangedFunctor) :
            DBusGetAttributeStubDispatcher<_StubClass, _AttributeType>(getStubFunctor, "v"),
            DBusGetFreedesktopAttributeStubDispatcher<_StubClass, _AttributeType>(getStubFunctor),
            DBusSetAttributeStubDispatcher<_StubClass, _AttributeType>(getStubFunctor,
                                                                       onRemoteSetFunctor,
                                                                       onRemoteChangedFunctor,
                                                                       "v"),
            DBusSetFreedesktopAttributeStubDispatcher<_StubClass, _AttributeType>(getStubFunctor,
                                                                                  onRemoteSetFunctor,
                                                                                  onRemoteChangedFunctor),
            DBusSetObservableAttributeStubDispatcher<_StubClass, _AttributeType>(getStubFunctor,
                                                                                 onRemoteSetFunctor,
                                                                                 onRemoteChangedFunctor,
                                                                                 fireChangedFunctor,
                                                                                 "v") {
    }
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_STUB_ADAPTER_HELPER_H_
