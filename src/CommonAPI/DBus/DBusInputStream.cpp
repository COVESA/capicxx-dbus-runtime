/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusInputStream.h"

namespace CommonAPI {
namespace DBus {

DBusInputStream::DBusInputStream(const CommonAPI::DBus::DBusMessage& message) :
                dataBegin_(message.getBodyData()),
                currentDataPosition_(0),
                dataLength_(message.getBodyLength()),
                exception_(nullptr),
                message_(message) {
}

DBusInputStream::~DBusInputStream() {}

const CommonAPI::DBus::DBusError& DBusInputStream::getError() const {
    return *exception_;
}

bool DBusInputStream::isErrorSet() const {
    return exception_ != nullptr;
}

void DBusInputStream::clearError() {
    exception_ = nullptr;
}

void DBusInputStream::alignToBoundary(const size_t alignBoundary) {
    const unsigned int alignMask = alignBoundary - 1;
    currentDataPosition_ = (currentDataPosition_ + alignMask) & (~alignMask);
}

char* DBusInputStream::readRawData(const size_t numBytesToRead) {
    assert((currentDataPosition_ + numBytesToRead) <= dataLength_);

    char* dataPtr = (char*) (dataBegin_ + currentDataPosition_);
    currentDataPosition_ += numBytesToRead;
    return dataPtr;
}

template<>
DBusInputStream& DBusInputStream::readBasicTypeValue<float>(float& val) {
    if (sizeof(val) > 1)
        alignToBoundary(sizeof(double));

    val = (float) (*(reinterpret_cast<double*>(readRawData(sizeof(double)))));
    return *this;
}

InputStream& DBusInputStream::readValue(bool& boolValue) {
    alignToBoundary(4);
    readBasicTypeValue(boolValue);
    alignToBoundary(4);
    return *this;
}

InputStream& DBusInputStream::readValue(int8_t& int8Value) {
    return readBasicTypeValue(int8Value);
}
InputStream& DBusInputStream::readValue(int16_t& int16Value) {
    return readBasicTypeValue(int16Value);
}
InputStream& DBusInputStream::readValue(int32_t& int32Value) {
    return readBasicTypeValue(int32Value);
}
InputStream& DBusInputStream::readValue(int64_t& int64Value) {
    return readBasicTypeValue(int64Value);
}

InputStream& DBusInputStream::readValue(uint8_t& uint8Value) {
    return readBasicTypeValue(uint8Value);
}
InputStream& DBusInputStream::readValue(uint16_t& uint16Value) {
    return readBasicTypeValue(uint16Value);
}
InputStream& DBusInputStream::readValue(uint32_t& uint32Value) {
    return readBasicTypeValue(uint32Value);
}
InputStream& DBusInputStream::readValue(uint64_t& uint64Value) {
    return readBasicTypeValue(uint64Value);
}

InputStream& DBusInputStream::readValue(float& floatValue) {
    return readBasicTypeValue(floatValue);
}
InputStream& DBusInputStream::readValue(double& doubleValue) {
    return readBasicTypeValue(doubleValue);
}

InputStream& DBusInputStream::readValue(std::string& stringValue) {
    uint32_t lengthOfString;
    readValue(lengthOfString);

    // length field does not include terminating 0-byte, therefore length of data to read is +1
    char* dataPtr = readRawData(lengthOfString + 1);

    // The string contained in a DBus-message is required to be 0-terminated, therefore the following line works
    stringValue = dataPtr;

    return *this;
}

InputStream& DBusInputStream::readValue(ByteBuffer& byteBufferValue) {
    *this >> byteBufferValue;
    return *this;
}

InputStream& DBusInputStream::readEnumValue(int8_t& int8BackingTypeValue) {
    return readValue(int8BackingTypeValue);
}
InputStream& DBusInputStream::readEnumValue(int16_t& int16BackingTypeValue) {
    return readValue(int16BackingTypeValue);
}
InputStream& DBusInputStream::readEnumValue(int32_t& int32BackingTypeValue) {
    return readValue(int32BackingTypeValue);
}
InputStream& DBusInputStream::readEnumValue(int64_t& int64BackingTypeValue) {
    return readValue(int64BackingTypeValue);
}
InputStream& DBusInputStream::readEnumValue(uint8_t& uint8BackingTypeValue) {
    return readValue(uint8BackingTypeValue);
}
InputStream& DBusInputStream::readEnumValue(uint16_t& uint16BackingTypeValue) {
    return readValue(uint16BackingTypeValue);
}
InputStream& DBusInputStream::readEnumValue(uint32_t& uint32BackingTypeValue) {
    return readValue(uint32BackingTypeValue);
}
InputStream& DBusInputStream::readEnumValue(uint64_t& uint64BackingTypeValue) {
    return readValue(uint64BackingTypeValue);
}

InputStream& DBusInputStream::readVersionValue(Version& versionValue) {
    alignToBoundary(8);
    readValue(versionValue.Major);
    readValue(versionValue.Minor);
    return *this;
}

void DBusInputStream::beginReadSerializableStruct(const SerializableStruct& serializableStruct) {
    alignToBoundary(8);
}

void DBusInputStream::endReadSerializableStruct(const SerializableStruct& serializableStruct) {
}

void DBusInputStream::beginReadSerializablePolymorphicStruct(uint32_t& serialId) {
    alignToBoundary(8);
    readValue(serialId);
    skipOverSignature();
    alignToBoundary(8);
}

void DBusInputStream::endReadSerializablePolymorphicStruct(const uint32_t& serialId) {
}

void DBusInputStream::readSerializableVariant(SerializableVariant& serializableVariant) {
    alignToBoundary(8);
    uint8_t containedTypeIndex;
    readValue(containedTypeIndex);
    skipOverSignature();

    serializableVariant.readFromInputStream(containedTypeIndex, *this);
}

void DBusInputStream::beginReadBoolVector() {
    beginReadGenericVector();
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadInt8Vector() {
    beginReadGenericVector();
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadInt16Vector() {
    beginReadGenericVector();
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadInt32Vector() {
    beginReadGenericVector();
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadInt64Vector() {
    beginReadGenericVector();
    alignToBoundary(8);
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadUInt8Vector() {
    beginReadGenericVector();
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadUInt16Vector() {
    beginReadGenericVector();
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadUInt32Vector() {
    beginReadGenericVector();
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadUInt64Vector() {
    beginReadGenericVector();
    alignToBoundary(8);
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadFloatVector() {
    beginReadGenericVector();
    alignToBoundary(8);
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadDoubleVector() {
    beginReadGenericVector();
    alignToBoundary(8);
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadStringVector() {
    beginReadGenericVector();
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadByteBufferVector() {
    beginReadGenericVector();
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadVersionVector() {
    beginReadGenericVector();
    alignToBoundary(8);
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadInt8EnumVector() {
    beginReadGenericVector();
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadInt16EnumVector() {
    beginReadGenericVector();
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadInt32EnumVector() {
    beginReadGenericVector();
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadInt64EnumVector() {
    beginReadGenericVector();
    alignToBoundary(8);
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadUInt8EnumVector() {
    beginReadGenericVector();
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadUInt16EnumVector() {
    beginReadGenericVector();
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadUInt32EnumVector() {
    beginReadGenericVector();
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadUInt64EnumVector() {
    beginReadGenericVector();
    alignToBoundary(8);
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadVectorOfSerializableStructs() {
    beginReadGenericVector();
    alignToBoundary(8);
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadVectorOfSerializableVariants() {
    beginReadGenericVector();
    alignToBoundary(8);
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadVectorOfVectors() {
    beginReadGenericVector();
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadVectorOfMaps() {
    beginReadGenericVector();
    alignToBoundary(4);
    savedStreamPositions_.push(currentDataPosition_);
}

void DBusInputStream::beginReadVectorOfSerializablePolymorphicStructs() {
    beginReadGenericVector();
    alignToBoundary(8);
    savedStreamPositions_.push(currentDataPosition_);
}

bool DBusInputStream::hasMoreVectorElements() {
    return bytesToRead_.top() > currentDataPosition_ - savedStreamPositions_.top();
}

void DBusInputStream::endReadVector() {
    bytesToRead_.pop();
    savedStreamPositions_.pop();
}

void DBusInputStream::beginReadMap() {
    uint32_t vectorByteSize;
    readBasicTypeValue(vectorByteSize);
    bytesToRead_.push(vectorByteSize);
    alignToBoundary(8);
    savedStreamPositions_.push(currentDataPosition_);
}

bool DBusInputStream::hasMoreMapElements() {
    return bytesToRead_.top() > currentDataPosition_ - savedStreamPositions_.top();
}

void DBusInputStream::endReadMap() {
    bytesToRead_.pop();
    savedStreamPositions_.pop();
}

void DBusInputStream::beginReadMapElement() {
    alignToBoundary(8);
}
void DBusInputStream::endReadMapElement() {
}

void DBusInputStream::setError() {
    exception_ = new CommonAPI::DBus::DBusError();
}

} // namespace DBus
} // namespace CommonAPI
