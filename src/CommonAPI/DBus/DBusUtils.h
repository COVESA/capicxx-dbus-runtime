/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef DBUSUTILS_H_
#define DBUSUTILS_H_

#include <algorithm>
#include <string>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <future>

namespace CommonAPI {
namespace DBus {

static const char* DBUS_CONFIG_SUFFIX = "_dbus.conf";

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


inline std::string getCurrentBinaryFileName() {
    char fqnOfBinary[FILENAME_MAX];
    char pathToProcessImage[FILENAME_MAX];

    sprintf(pathToProcessImage, "/proc/%ld/exe", getpid());
    const unsigned int lengthOfFqn = readlink(pathToProcessImage, fqnOfBinary, sizeof(fqnOfBinary) - 1);

    if (lengthOfFqn != -1) {
        fqnOfBinary[lengthOfFqn] = '\0';
        return std::string(std::move(fqnOfBinary));
    } else {
        //TODO fail of readlink, i.e. it returns -1, sets errno. See http://linux.die.net/man/3/readlink
        return std::string("");
    }
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
