/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef DBUSUTILS_H_
#define DBUSUTILS_H_

#include <future>

namespace CommonAPI {
namespace DBus {

//In gcc 4.4.1, the enumeration "std::future_status" is defined, but the return values of some functions
//are bool where the same functions in gcc 4.6. return a value from this enum. This template is a way
//to ensure compatibility for this issue.
template<typename _FutureWaitType>
inline bool checkReady(_FutureWaitType&);

template<>
inline bool checkReady<bool>(bool& returnedValue) {
	return returnedValue;
}

template<>
inline bool checkReady<std::future_status>(std::future_status& returnedValue) {
	return returnedValue == std::future_status::ready;
}

} //namespace DBus
} //namespace CommonAPI

#endif /* DBUSUTILS_H_ */
