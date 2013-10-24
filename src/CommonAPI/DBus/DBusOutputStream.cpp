/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "DBusOutputStream.h"

namespace CommonAPI {
namespace DBus {

DBusOutputStream::DBusOutputStream(DBusMessage dbusMessage) :
                dbusMessage_(dbusMessage) {
}

DBusOutputStream::~DBusOutputStream() {}

OutputStream& DBusOutputStream::writeValue(const bool& boolValue) {
    alignToBoundary(4);
    writeBasicTypeValue(boolValue);
    alignToBoundary(4);
    return *this;
}

OutputStream& DBusOutputStream::writeValue(const int8_t& int8Value) {
    return writeBasicTypeValue(int8Value);
}
OutputStream& DBusOutputStream::writeValue(const int16_t& int16Value) {
    return writeBasicTypeValue(int16Value);
}
OutputStream& DBusOutputStream::writeValue(const int32_t& int32Value) {
    return writeBasicTypeValue(int32Value);
}
OutputStream& DBusOutputStream::writeValue(const int64_t& int64Value) {
    return writeBasicTypeValue(int64Value);
}

OutputStream& DBusOutputStream::writeValue(const uint8_t& uint8Value) {
    return writeBasicTypeValue(uint8Value);
}
OutputStream& DBusOutputStream::writeValue(const uint16_t& uint16Value) {
    return writeBasicTypeValue(uint16Value);
}
OutputStream& DBusOutputStream::writeValue(const uint32_t& uint32Value) {
    return writeBasicTypeValue(uint32Value);
}
OutputStream& DBusOutputStream::writeValue(const uint64_t& uint64Value) {
    return writeBasicTypeValue(uint64Value);
}

OutputStream& DBusOutputStream::writeValue(const float& floatValue) {
    return writeBasicTypeValue((double) floatValue);
}
OutputStream& DBusOutputStream::writeValue(const double& doubleValue) {
    return writeBasicTypeValue(doubleValue);
}

OutputStream& DBusOutputStream::writeValue(const std::string& stringValue) {
    return writeString(stringValue.c_str(), stringValue.length());
}

OutputStream& DBusOutputStream::writeValue(const ByteBuffer& byteBufferValue) {
    return *this;
}

OutputStream& DBusOutputStream::writeEnumValue(const int8_t& int8BackingTypeValue) {
    return writeValue(int8BackingTypeValue);
}
OutputStream& DBusOutputStream::writeEnumValue(const int16_t& int16BackingTypeValue) {
    return writeValue(int16BackingTypeValue);
}
OutputStream& DBusOutputStream::writeEnumValue(const int32_t& int32BackingTypeValue) {
    return writeValue(int32BackingTypeValue);
}
OutputStream& DBusOutputStream::writeEnumValue(const int64_t& int64BackingTypeValue) {
    return writeValue(int64BackingTypeValue);
}
OutputStream& DBusOutputStream::writeEnumValue(const uint8_t& uint8BackingTypeValue) {
    return writeValue(uint8BackingTypeValue);
}
OutputStream& DBusOutputStream::writeEnumValue(const uint16_t& uint16BackingTypeValue) {
    return writeValue(uint16BackingTypeValue);
}
OutputStream& DBusOutputStream::writeEnumValue(const uint32_t& uint32BackingTypeValue) {
    return writeValue(uint32BackingTypeValue);
}
OutputStream& DBusOutputStream::writeEnumValue(const uint64_t& uint64BackingTypeValue) {
    return writeValue(uint64BackingTypeValue);
}

void DBusOutputStream::beginWriteBoolVector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteInt8Vector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteInt16Vector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteInt32Vector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteInt64Vector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    alignToBoundary(8);
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteUInt8Vector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteUInt16Vector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteUInt32Vector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteUInt64Vector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    alignToBoundary(8);
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteFloatVector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    alignToBoundary(8);
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteDoubleVector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    alignToBoundary(8);
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteStringVector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteByteBufferVector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteVersionVector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    alignToBoundary(8);
    rememberCurrentStreamPosition();
}

void DBusOutputStream::beginWriteInt8EnumVector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteInt16EnumVector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteInt32EnumVector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteInt64EnumVector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    alignToBoundary(8);
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteUInt8EnumVector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteUInt16EnumVector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteUInt32EnumVector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteUInt64EnumVector(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    alignToBoundary(8);
    rememberCurrentStreamPosition();
}

void DBusOutputStream::beginWriteVectorOfSerializableStructs(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    alignToBoundary(8);
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteVectorOfSerializableVariants(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    alignToBoundary(8);
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteVectorOfVectors(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    rememberCurrentStreamPosition();
}
void DBusOutputStream::beginWriteVectorOfMaps(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    alignToBoundary(4);
    rememberCurrentStreamPosition();
}

void DBusOutputStream::beginWriteVectorOfSerializablePolymorphicStructs(uint32_t sizeOfVector) {
    beginWriteGenericVector();
    alignToBoundary(8);
    rememberCurrentStreamPosition();
}

void DBusOutputStream::endWriteVector() {
    uint32_t numOfWrittenBytes = getCurrentStreamPosition() - popRememberedStreamPosition();
    writeBasicTypeValueAtPosition(popRememberedStreamPosition(), numOfWrittenBytes);
}

OutputStream& DBusOutputStream::writeVersionValue(const Version& versionValue) {
    alignToBoundary(8);
    writeValue(versionValue.Major);
    writeValue(versionValue.Minor);
    return *this;
}

void DBusOutputStream::beginWriteSerializableStruct(const SerializableStruct& serializableStruct) {
    alignToBoundary(8);
}
void DBusOutputStream::endWriteSerializableStruct(const SerializableStruct& serializableStruct) {
}

void DBusOutputStream::beginWriteSerializablePolymorphicStruct(const std::shared_ptr<SerializablePolymorphicStruct>& serializableStruct) {
    alignToBoundary(8);
    writeValue(serializableStruct->getSerialId());

    DBusTypeOutputStream typeOutputStream;
    typeOutputStream.beginWriteStructType();
    serializableStruct->createTypeSignature(typeOutputStream);
    typeOutputStream.endWriteStructType();

    writeSignature(std::move(typeOutputStream.retrieveSignature()));

    beginWriteSerializableStruct(*serializableStruct);
}

void DBusOutputStream::endWriteSerializablePolymorphicStruct(const std::shared_ptr<SerializablePolymorphicStruct>& serializableStruct) {
    endWriteSerializableStruct(*serializableStruct);
}

void DBusOutputStream::beginWriteMap(size_t elementCount) {
    alignToBoundary(sizeof(uint32_t));
    rememberCurrentStreamPosition();
    writeBasicTypeValue((uint32_t) 0);
    alignToBoundary(8);
    rememberCurrentStreamPosition();
}

void DBusOutputStream::endWriteMap() {
    uint32_t numOfWrittenBytes = getCurrentStreamPosition() - popRememberedStreamPosition();
    writeBasicTypeValueAtPosition(popRememberedStreamPosition(), numOfWrittenBytes);
}

void DBusOutputStream::beginWriteMapElement() {
    alignToBoundary(8);
}
void DBusOutputStream::endWriteMapElement() {
}

void DBusOutputStream::beginWriteSerializableVariant(const SerializableVariant& serializableVariant) {
    alignToBoundary(8);
    writeValue(serializableVariant.getValueType());

    DBusTypeOutputStream typeOutputStream;
    serializableVariant.writeToTypeOutputStream(typeOutputStream);
    writeSignature(std::move(typeOutputStream.retrieveSignature()));
}

void DBusOutputStream::endWriteSerializableVariant(const SerializableVariant& serializableVariant) {
}

bool DBusOutputStream::hasError() const {
    return dbusError_;
}

/**
 * Writes the data that was buffered within this #DBusOutputMessageStream to the #DBusMessage that was given to the constructor. Each call to flush()
 * will completely override the data that currently is contained in the #DBusMessage. The data that is buffered in this #DBusOutputMessageStream is
 * not deleted by calling flush().
 */
void DBusOutputStream::flush() {
    const int toWrite = payload_.size();
    const bool success = dbusMessage_.setBodyLength(toWrite);
    char* destinationDataPtr = dbusMessage_.getBodyData();

    memcpy(destinationDataPtr, payload_.c_str(), toWrite);
}

void DBusOutputStream::setError() {
}

/**
 * Reserves the given number of bytes for writing, thereby negating the need to dynamically allocate memory while writing.
 * Use this method for optimization: If possible, reserve as many bytes as you need for your data before doing any writing.
 *
 * @param numOfBytes The number of bytes that should be reserved for writing.
 */
void DBusOutputStream::reserveMemory(size_t numOfBytes) {
    assert(numOfBytes >= 0);
    payload_.reserve(numOfBytes);
}

DBusOutputStream& DBusOutputStream::writeString(const char* cString, const uint32_t& length) {
    assert(cString != NULL);
    assert(cString[length] == '\0');

    *this << length;

    writeRawData(cString, length + 1);

    return *this;
}

//Additional 0-termination, so this is 8 byte of \0
static const char eightByteZeroString[] = "\0\0\0\0\0\0\0";

void DBusOutputStream::alignToBoundary(const size_t alignBoundary) {
    assert(alignBoundary > 0 && alignBoundary <= 8 && (alignBoundary % 2 == 0 || alignBoundary == 1));

    size_t alignMask = alignBoundary - 1;
    size_t necessaryAlignment = ((alignMask - (payload_.size() & alignMask)) + 1) & alignMask;

    writeRawData(eightByteZeroString, necessaryAlignment);
}

bool DBusOutputStream::writeRawData(const char* rawDataPtr, const size_t sizeInByte) {
    assert(sizeInByte >= 0);

    payload_.append(rawDataPtr, sizeInByte);

    return true;
}

bool DBusOutputStream::writeRawDataAtPosition(size_t position, const char* rawDataPtr, const size_t sizeInByte) {
    assert(sizeInByte >= 0);

    payload_ = payload_.replace(position, sizeInByte, rawDataPtr, sizeInByte);

    return true;
}

void DBusOutputStream::beginWriteGenericVector() {
    alignToBoundary(sizeof(uint32_t));
    rememberCurrentStreamPosition();
    writeBasicTypeValue((uint32_t) 0);
}

void DBusOutputStream::writeSignature(const std::string& signature) {
    const auto& signatureLength = signature.length();
    assert(signatureLength > 0 && signatureLength < 256);

    const uint8_t wireLength = (uint8_t) signatureLength;
    *this << wireLength;
    writeRawData(signature.c_str(), wireLength + 1);
}

void DBusOutputStream::rememberCurrentStreamPosition() {
    savedStreamPositions_.push(payload_.size());
}

size_t DBusOutputStream::popRememberedStreamPosition() {
    size_t val = savedStreamPositions_.top();
    savedStreamPositions_.pop();
    return val;
}

size_t DBusOutputStream::getCurrentStreamPosition() {
    return payload_.size();
}

} // namespace DBus
} // namespace CommonAPI
