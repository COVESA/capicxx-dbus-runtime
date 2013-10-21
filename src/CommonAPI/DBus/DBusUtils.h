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
#include <future>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN // this prevents windows.h from including winsock.h, which causes duplicate definitions with winsock2.h
#include <windows.h>
#else
#include <unistd.h>
#endif

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


inline std::string getCurrentBinaryFileFQN() {
#ifdef WIN32
	TCHAR result[MAX_PATH];
	std::basic_string<TCHAR> resultString(result, GetModuleFileName(NULL, result, MAX_PATH));
	return std::string(resultString.begin(), resultString.end());
#else
	char fqnOfBinary[FILENAME_MAX];
	char pathToProcessImage[FILENAME_MAX];

	sprintf(pathToProcessImage, "/proc/%d/exe", getpid());
	const ssize_t lengthOfFqn = readlink(pathToProcessImage, fqnOfBinary, sizeof(fqnOfBinary)-1);

	if (lengthOfFqn != -1) {
		fqnOfBinary[lengthOfFqn] = '\0';
		return std::string(std::move(fqnOfBinary));
	}
	else {
		return std::string("");
	}
#endif
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
