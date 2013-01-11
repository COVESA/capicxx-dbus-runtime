/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef DBUSUTILS_H_
#define DBUSUTILS_H_

#include <algorithm>
#include <string>
#include <sstream>
#include <vector>

namespace CommonAPI {
namespace DBus {

inline std::vector<std::string>& split(const std::string& s, char delim, std::vector<std::string>& elems) {
    std::istringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

inline std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}

inline void findFallbackInstanceIdMapping(const std::string& instanceId,
                                          std::string& connectionName,
                                          std::string& objectPath) {
    connectionName = instanceId;
    objectPath = '/' + instanceId;
    std::replace(objectPath.begin(), objectPath.end(), '.', '/');
}

inline std::string findCommonAPIAddressForDBusAddress(const std::string& conName,
                                               const std::string& objName,
                                               const std::string& intName) {

    return "local:" + intName + ":" + conName;
}

inline std::string findFallbackInstanceIdMapping(const std::string& connectionName, const std::string& objectPath) {
    return connectionName;
}

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

}
}

#endif /* DBUSUTILS_H_ */
