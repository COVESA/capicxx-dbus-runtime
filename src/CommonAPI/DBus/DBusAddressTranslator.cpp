// Copyright (C) 2013-2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifdef WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <sys/stat.h>

#include <algorithm>

#include <CommonAPI/IniFileReader.hpp>
#include <CommonAPI/Logger.hpp>
#include <CommonAPI/Runtime.hpp>
#include <CommonAPI/DBus/DBusAddressTranslator.hpp>

namespace CommonAPI {
namespace DBus {

const char *COMMONAPI_DBUS_DEFAULT_CONFIG_FILE = "commonapi-dbus.ini";
const char *COMMONAPI_DBUS_DEFAULT_CONFIG_FOLDER = "/etc/";

const std::size_t DBUS_MAXIMUM_NAME_LENGTH = 255;

static std::shared_ptr<DBusAddressTranslator> theTranslator = std::make_shared<DBusAddressTranslator>();

std::shared_ptr<DBusAddressTranslator> DBusAddressTranslator::get() {
	return theTranslator;
}

DBusAddressTranslator::DBusAddressTranslator()
	: defaultDomain_("local") {
	init();

	isDefault_ = ("dbus" == Runtime::get()->getDefaultBinding());
}

void
DBusAddressTranslator::init() {
	// Determine default configuration file
	const char *config = getenv("COMMONAPI_DBUS_DEFAULT_CONFIG");
	if (config) {
		defaultConfig_ = config;
	} else {
		defaultConfig_ = COMMONAPI_DBUS_DEFAULT_CONFIG_FOLDER;
		defaultConfig_ += "/";
		defaultConfig_ += COMMONAPI_DBUS_DEFAULT_CONFIG_FILE;
	}

	(void)readConfiguration();
}

bool
DBusAddressTranslator::translate(const std::string &_key, DBusAddress &_value) {
	return translate(CommonAPI::Address(_key), _value);
}

bool
DBusAddressTranslator::translate(const CommonAPI::Address &_key, DBusAddress &_value) {
	bool result(true);
	std::lock_guard<std::mutex> itsLock(mutex_);

	const auto it = forwards_.find(_key);
	if (it != forwards_.end()) {
		_value = it->second;
	} else if (isDefault_) {
		std::string interfaceName(_key.getInterface());
		std::string objectPath("/" + _key.getInstance());
		std::replace(objectPath.begin(), objectPath.end(), '.', '/');
		std::string service(_key.getInterface() + "_" + _key.getInstance());

		if (isValid(service, '.', false, false, true)
		 && isValid(objectPath, '/', true)
		 && isValid(interfaceName, '.')) {
			_value.setInterface(interfaceName);
			_value.setObjectPath(objectPath);
			_value.setService(service);

			forwards_.insert({ _key, _value });
			backwards_.insert({ _value, _key });
		}
	} else {
		result = false;
	}

	return result;
}

bool
DBusAddressTranslator::translate(const DBusAddress &_key, std::string &_value) {
	CommonAPI::Address address;
	if (translate(_key, address)) {
		_value = address.getAddress();
		return true;
	}
	return false;
}

bool
DBusAddressTranslator::translate(const DBusAddress &_key, CommonAPI::Address &_value) {
	bool result(true);
	std::lock_guard<std::mutex> itsLock(mutex_);

	const auto it = backwards_.find(_key);
	if (it != backwards_.end()) {
		_value = it->second;
	} else if (isDefault_) {
		if (isValid(_key.getObjectPath(), '/', true) && isValid(_key.getInterface(), '.')) {
			std::string interfaceName(_key.getInterface());
			std::string instance(_key.getObjectPath().substr(1));
			std::replace(instance.begin(), instance.end(), '/', '.');

			_value.setDomain(defaultDomain_);
			_value.setInterface(interfaceName);
			_value.setInstance(instance);

			forwards_.insert({_value, _key});
			backwards_.insert({_key, _value});
		} else {
			result = false;
		}
	} else {
		result = false;
	}

	return result;
}


void
DBusAddressTranslator::insert(
		const std::string &_address,
		const std::string &_service, const std::string &_path, const std::string &_interface) {

	if (isValid(_service, '.',
			(_service.length() > 0 && _service[0] == ':'),
			(_service.length() > 0 && _service[0] == ':'),
			true)
	  && isValid(_path, '/', true)
	  && isValid(_interface, '.')) {
		CommonAPI::Address address(_address);
		DBusAddress dbusAddress(_service, _path, _interface);

		std::lock_guard<std::mutex> itsLock(mutex_);
		auto fw = forwards_.find(address);
		auto bw = backwards_.find(dbusAddress);
		if (fw == forwards_.end() && bw == backwards_.end()) {
			forwards_[address] = dbusAddress;
			backwards_[dbusAddress] = address;
			COMMONAPI_DEBUG(
				"Added address mapping: ", address, " <--> ", dbusAddress);
		} else if(bw != backwards_.end() && bw->second != address) {
			COMMONAPI_ERROR("Trying to overwrite existing DBus address "
					"which is already mapped to a CommonAPI address: ",
					dbusAddress, " <--> ", _address);
		} else if(fw != forwards_.end() && fw->second != dbusAddress) {
			COMMONAPI_ERROR("Trying to overwrite existing CommonAPI address "
					"which is already mapped to a DBus address: ",
					_address, " <--> ", dbusAddress);
		}
	}
}

bool
DBusAddressTranslator::readConfiguration() {
#define MAX_PATH_LEN 255
	std::string config;
	char currentDirectory[MAX_PATH_LEN];
#ifdef WIN32
	if (GetCurrentDirectory(MAX_PATH_LEN, currentDirectory)) {
#else
	if (getcwd(currentDirectory, MAX_PATH_LEN)) {
#endif
		config = currentDirectory;
		config += "/";
		config += COMMONAPI_DBUS_DEFAULT_CONFIG_FILE;

		struct stat s;
		if (stat(config.c_str(), &s) != 0) {
			config = defaultConfig_;
		}
	}

	IniFileReader reader;
	if (!reader.load(config))
		return false;

	for (auto itsMapping : reader.getSections()) {
		CommonAPI::Address itsAddress(itsMapping.first);

		std::string service = itsMapping.second->getValue("service");
		std::string path = itsMapping.second->getValue("path");
		std::string interfaceName = itsMapping.second->getValue("interface");

		insert(itsMapping.first, service, path, interfaceName);
	}

	return true;
}

bool
DBusAddressTranslator::isValid(
		const std::string &_name, const char _separator,
		bool _ignoreFirst, bool _isAllowedToStartWithDigit, bool _isBusName) const {
	// DBus addresses must contain at least one separator
	std::size_t separatorPos = _name.find(_separator);
	if (separatorPos == std::string::npos) {
		COMMONAPI_ERROR(
			"Invalid name \'", _name,
			"\'. Contains no \'", _separator, "\'");
		return false;
	}

	bool isInitial(true);
	std::size_t start(0);

	if (_ignoreFirst) {
		start = 1;
		if (separatorPos == 0)
			separatorPos = _name.find(_separator, separatorPos+1);
	}

	while (start != std::string::npos) {
		// DBus names parts must not be empty
		std::string part;

		if (isInitial) {
			isInitial = false;
		} else {
			start++;
		}

		if (separatorPos == std::string::npos) {
			part = _name.substr(start);
		} else {
			part = _name.substr(start, separatorPos-start);
		}

		if ("" == part) {
			COMMONAPI_ERROR(
				"Invalid interface name \'", _name,
				"\'. Must not contain empty parts.");
			return false;
		}

		// DBus name parts must not start with a digit (not valid for unique names)
		if (!_isAllowedToStartWithDigit) {
			if (part[0] >= '0' && part[0] <= '9') {
				COMMONAPI_ERROR(
					"Invalid interface name \'", _name,
					"\'. First character must not be a digit.");
				return false;
			}
		}

		// DBus name parts consist of the ASCII characters [0-9][A-Z][a-z]_,
		for (auto c : part) {
			// bus names may additionally contain [-]
			if (_isBusName && c == '-')
				continue;

			if (c < '0' ||
				(c > '9' && c < 'A') ||
				(c > 'Z' && c < '_') ||
				(c > '_' && c < 'a') ||
				c > 'z') {
				COMMONAPI_ERROR(
					"Invalid interface name \'", _name,
					"\'. Contains illegal character \'", c,
					"\'. Only \'[0-9][A-Z][a-z]_\' are allowed.");
				return false;
			}
		}

		start = separatorPos;
		separatorPos = _name.find(_separator, separatorPos+1);
	}

	// DBus names must not exceed the maximum length
	if (_name.length() > DBUS_MAXIMUM_NAME_LENGTH) {
		COMMONAPI_ERROR(
			"Invalid interface name \'", _name,
			"\'. Size exceeds maximum size.");
		return false;
	}

	return true;
}

} // namespace DBus
} // namespace CommonAPI
