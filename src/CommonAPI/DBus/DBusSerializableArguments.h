/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_SERIALIZABLE_ARGUMENTS_H_
#define COMMONAPI_DBUS_SERIALIZABLE_ARGUMENTS_H_

#include "DBusInputStream.h"
#include "DBusOutputStream.h"

namespace CommonAPI {
namespace DBus {

template <typename... _Arguments>
struct DBusSerializableArguments;

template <>
struct DBusSerializableArguments<> {
	static inline bool serialize(OutputStream& outputStream) {
		return true;
	}

	static inline bool deserialize(DBusInputStream& inputStream) {
		return true;
	}
};

template <typename _ArgumentType>
struct DBusSerializableArguments<_ArgumentType> {
	static inline bool serialize(OutputStream& outputStream, const _ArgumentType& argument) {
		outputStream << argument;
		return !outputStream.hasError();
	}

	static inline bool deserialize(DBusInputStream& inputStream, _ArgumentType& argument) {
		inputStream >> argument;
		return !inputStream.hasError();
	}
};

template <typename _ArgumentType, typename ... _Rest>
struct DBusSerializableArguments<_ArgumentType, _Rest...> {
	static inline bool serialize(OutputStream& outputStream, const _ArgumentType& argument, const _Rest&... rest) {
		outputStream << argument;
		const bool success = !outputStream.hasError();
		return success ? DBusSerializableArguments<_Rest...>::serialize(outputStream, rest...) : false;
	}

	static inline bool deserialize(DBusInputStream& inputStream, _ArgumentType& argument, _Rest&... rest) {
		inputStream >> argument;
		const bool success = !inputStream.hasError();
		return success ? DBusSerializableArguments<_Rest...>::deserialize(inputStream, rest...) : false;
	}
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_SERIALIZABLE_ARGUMENTS_H_
