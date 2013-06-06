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

inline bool containsOnlyAlphanumericCharacters(const std::string& toCheck) {
    auto firstNonAlphanumericCharacterIt = std::find_if(toCheck.begin(),
                    toCheck.end(),
                    [](char c) {
                        return !std::isalnum(c);
                    });

    return firstNonAlphanumericCharacterIt == toCheck.end();
}

inline bool isValidDomainName(const std::string& domainName) {
    return containsOnlyAlphanumericCharacters(domainName);
}

inline bool isValidServiceName(const std::string& serviceName) {
    bool isValid = serviceName[0] != '.' && serviceName[serviceName.size() - 1] != '.';

    if (isValid) {
        std::vector<std::string> splittedServiceName = split(serviceName, '.');

        for (auto serviceNameElementIt = splittedServiceName.begin();
                        serviceNameElementIt != splittedServiceName.end() && isValid;
                        ++serviceNameElementIt) {
            isValid &= containsOnlyAlphanumericCharacters(*serviceNameElementIt);
        }
    }

    return isValid;
}

inline bool isValidInstanceId(const std::string& instanceId) {
    //Validation rules for ServiceName and InstanceID are equivalent
    return isValidServiceName(instanceId);
}

inline bool isValidCommonApiAddress(const std::string& commonApiAddress) {
    std::vector<std::string> splittedAddress = split(commonApiAddress, ':');
    if (splittedAddress.size() != 3) {
        return false;
    }
    return isValidDomainName(splittedAddress[0]) && isValidServiceName(splittedAddress[1]) && isValidInstanceId(splittedAddress[2]);
}


inline std::string getCurrentBinaryFileFQN() {
    char fqnOfBinary[FILENAME_MAX];
    char pathToProcessImage[FILENAME_MAX];

    sprintf(pathToProcessImage, "/proc/%d/exe", getpid());
    const ssize_t lengthOfFqn = readlink(pathToProcessImage, fqnOfBinary, sizeof(fqnOfBinary) - 1);

    if (lengthOfFqn != -1) {
        fqnOfBinary[lengthOfFqn] = '\0';
        return std::string(std::move(fqnOfBinary));
    } else {
        return std::string("");
    }
}

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

}
}

#endif /* DBUSUTILS_H_ */
