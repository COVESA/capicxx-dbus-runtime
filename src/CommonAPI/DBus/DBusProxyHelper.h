/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_DBUS_DBUS_PROXY_HELPER_H_
#define COMMONAPI_DBUS_DBUS_PROXY_HELPER_H_

#include "DBusMessage.h"
#include "DBusSerializableArguments.h"
#include "DBusProxyAsyncCallbackHandler.h"

#include <functional>
#include <future>
#include <memory>
#include <string>

namespace CommonAPI {
namespace DBus {


class DBusProxy;


template< class, class >
struct DBusProxyHelper;

template <
	template <class...> class _In, class... _InArgs,
	template <class...> class _Out, class... _OutArgs>
struct DBusProxyHelper<_In<_InArgs...>, _Out<_OutArgs...>> {
	template <typename _DBusProxy = DBusProxy>
	static void callMethod(const _DBusProxy& dbusProxy,
			const char* methodName,
			const char* methodSignature,
			const _InArgs&... inArgs,
			CommonAPI::CallStatus& callStatus) {
	    if (dbusProxy.isAvailableBlocking()) {

	        DBusMessage dbusMessage = dbusProxy.createMethodCall(methodName, methodSignature);

	        if (sizeof...(_InArgs) > 0) {
	            DBusOutputStream outputStream(dbusMessage);
	            const bool success = DBusSerializableArguments<_InArgs...>::serialize(outputStream, inArgs...);
	            if (!success) {
	                callStatus = CallStatus::OUT_OF_MEMORY;
	                return;
	            }
	            outputStream.flush();
	        }

	        const bool dbusMessageSent = dbusProxy.getDBusConnection()->sendDBusMessage(dbusMessage);
	        callStatus = dbusMessageSent ? CallStatus::SUCCESS : CallStatus::OUT_OF_MEMORY;
	    } else {
	        callStatus = CallStatus::NOT_AVAILABLE;
	    }
	}

	template <typename _DBusProxy = DBusProxy>
	static void callMethodWithReply(
			const _DBusProxy& dbusProxy,
			const char* methodName,
			const char* methodSignature,
			const _InArgs&... inArgs,
			CommonAPI::CallStatus& callStatus,
			_OutArgs&... outArgs) {

	    if (dbusProxy.isAvailableBlocking()) {

	        DBusMessage dbusMethodCall = dbusProxy.createMethodCall(methodName, methodSignature);

	        if (sizeof...(_InArgs) > 0) {
	            DBusOutputStream outputStream(dbusMethodCall);
	            const bool success = DBusSerializableArguments<_InArgs...>::serialize(outputStream, inArgs...);
	            if (!success) {
	                callStatus = CallStatus::OUT_OF_MEMORY;
	                return;
	            }
	            outputStream.flush();
	        }

	        DBusError dbusError;
	        DBusMessage dbusMessageReply = dbusProxy.getDBusConnection()->sendDBusMessageWithReplyAndBlock(dbusMethodCall, dbusError);
	        if (dbusError || !dbusMessageReply.isMethodReturnType()) {
	            callStatus = CallStatus::REMOTE_ERROR;
	            return;
	        }

	        if (sizeof...(_OutArgs) > 0) {
	            DBusInputStream inputStream(dbusMessageReply);
	            const bool success = DBusSerializableArguments<_OutArgs...>::deserialize(inputStream, outArgs...);
	            if (!success) {
	                callStatus = CallStatus::REMOTE_ERROR;
	                return;
	            }
	        }

	        callStatus = CallStatus::SUCCESS;
	    } else {
	        callStatus = CallStatus::NOT_AVAILABLE;
	    }
	}

	template <typename _DBusProxy = DBusProxy, typename _AsyncCallback>
	static std::future<CallStatus> callMethodAsync(
			const _DBusProxy& dbusProxy,
			const char* methodName,
			const char* methodSignature,
			const _InArgs&... inArgs,
			_AsyncCallback asyncCallback) {

	    if (dbusProxy.isAvailable()) {

	        DBusMessage dbusMessage = dbusProxy.createMethodCall(methodName, methodSignature);

	        if (sizeof...(_InArgs) > 0) {
	            DBusOutputStream outputStream(dbusMessage);
			    const bool success = DBusSerializableArguments<_InArgs...>::serialize(outputStream, inArgs...);
			    if (!success) {
			        std::promise<CallStatus> promise;
			        promise.set_value(CallStatus::OUT_OF_MEMORY);
			        return promise.get_future();
			    }
			    outputStream.flush();
	        }

	        return dbusProxy.getDBusConnection()->sendDBusMessageWithReplyAsync(
	                        dbusMessage,
	                        DBusProxyAsyncCallbackHandler<_OutArgs...>::create(std::move(asyncCallback)));
	        } else {
	            std::promise<CallStatus> promise;
	            promise.set_value(CallStatus::NOT_AVAILABLE);
	            return promise.get_future();
	        }
	   }
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_PROXY_HELPER_H_
