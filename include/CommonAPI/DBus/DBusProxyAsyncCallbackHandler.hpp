// Copyright (C) 2014-2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.hpp> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUSPROXYASYNCCALLBACKHANDLER_HPP_
#define COMMONAPI_DBUS_DBUSPROXYASYNCCALLBACKHANDLER_HPP_

#include <functional>
#include <future>
#include <memory>

#include <CommonAPI/DBus/DBusHelper.hpp>
#include <CommonAPI/DBus/DBusMessage.hpp>
#include <CommonAPI/DBus/DBusProxyConnection.hpp>
#include <CommonAPI/DBus/DBusSerializableArguments.hpp>

namespace CommonAPI {
namespace DBus {

template<typename ... _ArgTypes>
class DBusProxyAsyncCallbackHandler: public DBusProxyConnection::DBusMessageReplyAsyncHandler {
 public:
    typedef std::function<void(CallStatus, _ArgTypes...)> FunctionType;

    static std::unique_ptr<DBusProxyConnection::DBusMessageReplyAsyncHandler> create(
            FunctionType&& callback, std::tuple<_ArgTypes...> args) {
        return std::unique_ptr<DBusProxyConnection::DBusMessageReplyAsyncHandler>(
                new DBusProxyAsyncCallbackHandler(std::move(callback), args));
    }

    DBusProxyAsyncCallbackHandler() = delete;
    DBusProxyAsyncCallbackHandler(FunctionType&& callback, std::tuple<_ArgTypes...> args):
        callback_(std::move(callback)), args_(args) {
    }
    virtual ~DBusProxyAsyncCallbackHandler() {}

    virtual std::future<CallStatus> getFuture() {
        return promise_.get_future();
    }

    virtual void onDBusMessageReply(const CallStatus& dbusMessageCallStatus, const DBusMessage& dbusMessage) {
        promise_.set_value(handleDBusMessageReply(dbusMessageCallStatus, dbusMessage, typename make_sequence<sizeof...(_ArgTypes)>::type(), args_));
    }

 private:
    template <int... _ArgIndices>
    inline CallStatus handleDBusMessageReply(const CallStatus dbusMessageCallStatus, const DBusMessage& dbusMessage, index_sequence<_ArgIndices...>, std::tuple<_ArgTypes...> argTuple) const {
        CallStatus callStatus = dbusMessageCallStatus;

        if (dbusMessageCallStatus == CallStatus::SUCCESS) {
            if (!dbusMessage.isErrorType()) {
                DBusInputStream dbusInputStream(dbusMessage);
                const bool success = DBusSerializableArguments<_ArgTypes...>::deserialize(dbusInputStream, std::get<_ArgIndices>(argTuple)...);
                if (!success)
                    callStatus = CallStatus::REMOTE_ERROR;
            } else {
                callStatus = CallStatus::REMOTE_ERROR;
            }
        }

        callback_(callStatus, std::move(std::get<_ArgIndices>(argTuple))...);
        return callStatus;
    }

    std::promise<CallStatus> promise_;
    const FunctionType callback_;
    std::tuple<_ArgTypes...> args_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUSPROXYASYNCCALLBACKHANDLER_HPP_
