// Copyright (C) 2013-2020 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <iomanip>

#include <CommonAPI/DBus/DBusInputStream.hpp>

namespace CommonAPI {
namespace DBus {

DBusInputStream::DBusInputStream(const CommonAPI::DBus::DBusMessage &_message)
    : begin_(_message.getBodyData()),
      current_(0),
      size_((size_t)(_message.getBodyLength())),
      exception_(nullptr),
      message_(_message) {
}

DBusInputStream::~DBusInputStream() {}

const CommonAPI::DBus::DBusError& DBusInputStream::getError() const {
    return (*exception_);
}

bool DBusInputStream::isErrorSet() const {
    return (exception_ != nullptr);
}

void DBusInputStream::clearError() {
    exception_ = nullptr;
}

void DBusInputStream::align(const size_t _boundary) {
    const unsigned int mask = static_cast<unsigned int>(_boundary) - 1;
    current_ = (current_ + mask) & (~mask);
}

char *DBusInputStream::_readRaw(const size_t _size) {
    if ((current_ + _size) > size_) {
        setError();
        return NULL;
    }

    char *data = (char *) (begin_ + current_);
    current_ += _size;
    return data;
}

void DBusInputStream::setError() {
    exception_ = std::unique_ptr<CommonAPI::DBus::DBusError>(new CommonAPI::DBus::DBusError());
}

void DBusInputStream::pushPosition() {
    positions_.push_back(current_);
}

size_t DBusInputStream::popPosition() {
    size_t itsPosition = positions_.back();
    positions_.pop_back();
    return itsPosition;
}

void DBusInputStream::pushSize(size_t _size) {
    sizes_.push_back(static_cast<unsigned int>(_size));
}

size_t DBusInputStream::popSize() {
    size_t itsSize = sizes_.back();
    sizes_.pop_back();
    return itsSize;
}

InputStream<DBusInputStream> &DBusInputStream::readValue(bool &_value, const EmptyDeployment *_depl) {
    uint32_t tmp(2);
    readValue(tmp, _depl);
    if (tmp > 1)
        setError();
    _value = (tmp != 0);
    return (*this);
}

InputStream<DBusInputStream> &DBusInputStream::readValue(int8_t &_value, const EmptyDeployment *_depl) {
    (void)_depl;
    return _readValue(_value);
}

InputStream<DBusInputStream> &DBusInputStream::readValue(int16_t &_value, const EmptyDeployment *_depl) {
    (void)_depl;
    return _readValue(_value);
}

InputStream<DBusInputStream> &DBusInputStream::readValue(int32_t &_value, const EmptyDeployment *_depl) {
    (void)_depl;
    return _readValue(_value);
}

InputStream<DBusInputStream> &DBusInputStream::readValue(int64_t &_value, const EmptyDeployment *_depl) {
    (void)_depl;
    return _readValue(_value);
}

InputStream<DBusInputStream> &DBusInputStream::readValue(uint8_t &_value, const EmptyDeployment *_depl) {
    (void)_depl;
    return _readValue(_value);
}

InputStream<DBusInputStream> &DBusInputStream::readValue(uint16_t &_value, const EmptyDeployment *_depl) {
    (void)_depl;
    return _readValue(_value);
}

InputStream<DBusInputStream> &DBusInputStream::readValue(uint32_t &_value, const EmptyDeployment *_depl) {
    (void)_depl;
    return _readValue(_value);
}

InputStream<DBusInputStream> &DBusInputStream::readValue(uint64_t &_value, const EmptyDeployment *_depl) {
    (void)_depl;
    return _readValue(_value);
}

InputStream<DBusInputStream> &DBusInputStream::readValue(float &_value, const EmptyDeployment *_depl) {
    (void)_depl;
    return _readValue(_value);
}

InputStream<DBusInputStream> &DBusInputStream::readValue(double &_value, const EmptyDeployment *_depl) {
    (void)_depl;
    return _readValue(_value);
}

InputStream<DBusInputStream> &DBusInputStream::readValue(std::string &_value, const EmptyDeployment *_depl) {
    (void)_depl;
    uint32_t length(0);
    _readValue(length);

    // length field does not include terminating 0-byte, therefore length of data to read is +1
    char *data = _readRaw(length + 1);
    if (!hasError()) {
        // The string contained in a DBus-message is required to be 0-terminated, therefore the following line works
        _value = data;
    }

    return (*this);
}

InputStream<DBusInputStream> &DBusInputStream::readValue(Version &_value, const EmptyDeployment *_depl) {
    (void)_depl;
    align(8);
    _readValue(_value.Major);
    _readValue(_value.Minor);
    return *this;
}

} // namespace DBus
} // namespace CommonAPI
