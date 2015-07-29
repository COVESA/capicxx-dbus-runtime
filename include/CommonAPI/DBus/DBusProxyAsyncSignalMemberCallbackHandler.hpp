// Copyright (C) 2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.hpp> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUSPROXYASYNCSIGNALMEMBERCALLBACKHANDLER_HPP_
#define COMMONAPI_DBUS_DBUSPROXYASYNCSIGNALMEMBERCALLBACKHANDLER_HPP_

#include <functional>
#include <future>
#include <memory>

//#include <CommonAPI/DBus/DBusHelper.hpp>
#include <CommonAPI/DBus/DBusMessage.hpp>
#include <CommonAPI/DBus/DBusProxyConnection.hpp>

namespace CommonAPI {
namespace DBus {

class DBusProxyAsyncSignalMemberCallbackHandler: public DBusProxyConnection::DBusMessageReplyAsyncHandler {
 public:
	typedef std::function<void(CallStatus, DBusMessage, DBusProxyConnection::DBusSignalHandler*, int)> FunctionType;

    static std::unique_ptr<DBusProxyConnection::DBusMessageReplyAsyncHandler> create(
			FunctionType& callback, DBusProxyConnection::DBusSignalHandler* dbusSignalHandler,
			const int tag) {
		return std::unique_ptr<DBusProxyConnection::DBusMessageReplyAsyncHandler>(
				new DBusProxyAsyncSignalMemberCallbackHandler(std::move(callback), dbusSignalHandler, tag));
	}

    DBusProxyAsyncSignalMemberCallbackHandler() = delete;
    DBusProxyAsyncSignalMemberCallbackHandler(FunctionType&& callback,
    		DBusProxyConnection::DBusSignalHandler* dbusSignalHandler,
			const int tag):
		callback_(std::move(callback)), dbusSignalHandler_(dbusSignalHandler), tag_(tag) {
	}
    virtual ~DBusProxyAsyncSignalMemberCallbackHandler() {}

    virtual std::future<CallStatus> getFuture() {
        return promise_.get_future();
    }

    virtual void onDBusMessageReply(const CallStatus& dbusMessageCallStatus, const DBusMessage& dbusMessage) {
    	promise_.set_value(handleDBusMessageReply(dbusMessageCallStatus, dbusMessage));
    }

 private:
    inline CallStatus handleDBusMessageReply(const CallStatus dbusMessageCallStatus, const DBusMessage& dbusMessage) const {
        CallStatus callStatus = dbusMessageCallStatus;

        callback_(callStatus, dbusMessage, dbusSignalHandler_, tag_);
        return callStatus;
    }

    std::promise<CallStatus> promise_;
    const FunctionType callback_;
    DBusProxyConnection::DBusSignalHandler* dbusSignalHandler_;
    const int tag_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUSPROXYASYNCSIGNALMEMBERCALLBACKHANDLER_HPP_
