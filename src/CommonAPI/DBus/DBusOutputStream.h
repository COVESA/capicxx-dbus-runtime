/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_OUTPUT_MESSAGE_STREAM_H_
#define COMMONAPI_DBUS_DBUS_OUTPUT_MESSAGE_STREAM_H_

#include "DBusMessage.h"
#include "DBusError.h"

#include <CommonAPI/OutputStream.h>

#include <string>
#include <cstring>
#include <vector>
#include <cassert>
#include <stack>
#include <memory>

namespace CommonAPI {
namespace DBus {

class DBusTypeOutputStream: public TypeOutputStream {
public:
    DBusTypeOutputStream() :
                    signature_("") {

    }
    virtual ~DBusTypeOutputStream() {}

    virtual void writeBoolType() {
        signature_.append("b");
    }

    virtual void writeInt8Type() {
        signature_.append("y");
    }
    virtual void writeInt16Type() {
        signature_.append("n");
    }
    virtual void writeInt32Type() {
        signature_.append("i");
    }
    virtual void writeInt64Type() {
        signature_.append("x");
    }

    virtual void writeUInt8Type() {
        signature_.append("y");
    }
    virtual void writeUInt16Type() {
        signature_.append("q");
    }
    virtual void writeUInt32Type() {
        signature_.append("u");
    }
    virtual void writeUInt64Type() {
        signature_.append("t");
    }

    virtual void writeInt8EnumType() {
        signature_.append("y");
    }
    virtual void writeInt16EnumType() {
        signature_.append("n");
    }
    virtual void writeInt32EnumType() {
        signature_.append("i");
    }
    virtual void writeInt64EnumType() {
        signature_.append("x");
    }

    virtual void writeUInt8EnumType() {
        signature_.append("y");
    }
    virtual void writeUInt16EnumType() {
        signature_.append("n");
    }
    virtual void writeUInt32EnumType() {
        signature_.append("u");
    }
    virtual void writeUInt64EnumType() {
        signature_.append("t");
    }

    virtual void writeFloatType() {
        signature_.append("d");
    }
    virtual void writeDoubleType() {
        signature_.append("d");
    }

    virtual void writeStringType() {
        signature_.append("s");
    }
    virtual void writeByteBufferType() {
        signature_.append("ay");
    }
    virtual void writeVersionType() {
        signature_.append("(uu)");
    }

    virtual void beginWriteStructType() {
        signature_.append("(");
    }
    virtual void endWriteStructType() {
        signature_.append(")");
    }

    virtual void beginWriteMapType() {
        signature_.append("a{");
    }
    virtual void endWriteMapType() {
        signature_.append("}");
    }

    virtual void beginWriteVectorType() {
        signature_.append("a");
    }

    virtual void endWriteVectorType() {
    }

    virtual void writeVariantType() {
        signature_.append("(yv)");
    }

    void writeLegacyVariantType() {
        signature_.append("v");
    }

    virtual std::string retrieveSignature() {
        return std::move(signature_);
    }

private:
    std::string signature_;
};

/**
 * Used to mark the position of a pointer within an array of bytes.
 */
typedef uint32_t position_t;

/**
 * @class DBusOutputMessageStream
 *
 * Used to serialize and write data into a #DBusMessage. For all data types that may be written to a #DBusMessage, a "<<"-operator should be defined to handle the writing
 * (this operator is predefined for all basic data types and for vectors). The signature that has to be written to the #DBusMessage separately is assumed
 * to match the actual data that is inserted via the #DBusOutputMessageStream.
 */
class DBusOutputStream: public OutputStream {
public:

    /**
     * Creates a #DBusOutputMessageStream which can be used to serialize and write data into the given #DBusMessage. Any data written is buffered within the stream.
     * Remember to call flush() when you are done with writing: Only then the data actually is written to the #DBusMessage.
     *
     * @param dbusMessage The #DBusMessage any data pushed into this stream should be written to.
     */
    DBusOutputStream(DBusMessage dbusMessage);

    /**
     * Destructor; does not call the destructor of the referred #DBusMessage. Make sure to maintain a reference to the
     * #DBusMessage outside of the stream if you intend to make further use of the message, e.g. in order to send it,
     * now that you have written some payload into it.
     */
    virtual ~DBusOutputStream();

    virtual OutputStream& writeValue(const bool& boolValue);

    virtual OutputStream& writeValue(const int8_t& int8Value);
    virtual OutputStream& writeValue(const int16_t& int16Value);
    virtual OutputStream& writeValue(const int32_t& int32Value);
    virtual OutputStream& writeValue(const int64_t& int64Value);

    virtual OutputStream& writeValue(const uint8_t& uint8Value);
    virtual OutputStream& writeValue(const uint16_t& uint16Value);
    virtual OutputStream& writeValue(const uint32_t& uint32Value);
    virtual OutputStream& writeValue(const uint64_t& uint64Value);

    virtual OutputStream& writeValue(const float& floatValue);
    virtual OutputStream& writeValue(const double& doubleValue);

    virtual OutputStream& writeValue(const std::string& stringValue);

    virtual OutputStream& writeValue(const ByteBuffer& byteBufferValue);

    virtual OutputStream& writeEnumValue(const int8_t& int8BackingTypeValue);
    virtual OutputStream& writeEnumValue(const int16_t& int16BackingTypeValue);
    virtual OutputStream& writeEnumValue(const int32_t& int32BackingTypeValue);
    virtual OutputStream& writeEnumValue(const int64_t& int64BackingTypeValue);
    virtual OutputStream& writeEnumValue(const uint8_t& uint8BackingTypeValue);
    virtual OutputStream& writeEnumValue(const uint16_t& uint16BackingTypeValue);
    virtual OutputStream& writeEnumValue(const uint32_t& uint32BackingTypeValue);
    virtual OutputStream& writeEnumValue(const uint64_t& uint64BackingTypeValue);

    virtual void beginWriteBoolVector(uint32_t sizeOfVector);
    virtual void beginWriteInt8Vector(uint32_t sizeOfVector);
    virtual void beginWriteInt16Vector(uint32_t sizeOfVector);
    virtual void beginWriteInt32Vector(uint32_t sizeOfVector);
    virtual void beginWriteInt64Vector(uint32_t sizeOfVector);
    virtual void beginWriteUInt8Vector(uint32_t sizeOfVector);
    virtual void beginWriteUInt16Vector(uint32_t sizeOfVector);
    virtual void beginWriteUInt32Vector(uint32_t sizeOfVector);
    virtual void beginWriteUInt64Vector(uint32_t sizeOfVector);
    virtual void beginWriteFloatVector(uint32_t sizeOfVector);
    virtual void beginWriteDoubleVector(uint32_t sizeOfVector);
    virtual void beginWriteStringVector(uint32_t sizeOfVector);
    virtual void beginWriteByteBufferVector(uint32_t sizeOfVector);
    virtual void beginWriteVersionVector(uint32_t sizeOfVector);

    virtual void beginWriteInt8EnumVector(uint32_t sizeOfVector);
    virtual void beginWriteInt16EnumVector(uint32_t sizeOfVector);
    virtual void beginWriteInt32EnumVector(uint32_t sizeOfVector);
    virtual void beginWriteInt64EnumVector(uint32_t sizeOfVector);
    virtual void beginWriteUInt8EnumVector(uint32_t sizeOfVector);
    virtual void beginWriteUInt16EnumVector(uint32_t sizeOfVector);
    virtual void beginWriteUInt32EnumVector(uint32_t sizeOfVector);
    virtual void beginWriteUInt64EnumVector(uint32_t sizeOfVector);

    virtual void beginWriteVectorOfSerializableStructs(uint32_t sizeOfVector);
    virtual void beginWriteVectorOfSerializableVariants(uint32_t sizeOfVector);
    virtual void beginWriteVectorOfVectors(uint32_t sizeOfVector);
    virtual void beginWriteVectorOfMaps(uint32_t sizeOfVector);

    virtual void beginWriteVectorOfSerializablePolymorphicStructs(uint32_t sizeOfVector);

    virtual void endWriteVector();

    virtual OutputStream& writeVersionValue(const Version& versionValue);

    virtual void beginWriteSerializableStruct(const SerializableStruct& serializableStruct);
    virtual void endWriteSerializableStruct(const SerializableStruct& serializableStruct);

    virtual void beginWriteSerializablePolymorphicStruct(const std::shared_ptr<SerializablePolymorphicStruct>& serializableStruct);
    virtual void endWriteSerializablePolymorphicStruct(const std::shared_ptr<SerializablePolymorphicStruct>& serializableStruct);

    virtual void beginWriteMap(size_t elementCount);
    virtual void endWriteMap();
    virtual void beginWriteMapElement();
    virtual void endWriteMapElement();

    virtual void beginWriteSerializableVariant(const SerializableVariant& serializableVariant);

    virtual void endWriteSerializableVariant(const SerializableVariant& serializableVariant);

    virtual bool hasError() const;

    /**
     * Writes the data that was buffered within this #DBusOutputMessageStream to the #DBusMessage that was given to the constructor. Each call to flush()
     * will completely override the data that currently is contained in the #DBusMessage. The data that is buffered in this #DBusOutputMessageStream is
     * not deleted by calling flush().
     */
    void flush();

    void setError();

    /**
     * Reserves the given number of bytes for writing, thereby negating the need to dynamically allocate memory while writing.
     * Use this method for optimization: If possible, reserve as many bytes as you need for your data before doing any writing.
     *
     * @param numOfBytes The number of bytes that should be reserved for writing.
     */
    void reserveMemory(size_t numOfBytes);

    template<typename _BasicType>
    DBusOutputStream& writeBasicTypeValue(const _BasicType& basicValue) {
        if (sizeof(_BasicType) > 1)
            alignToBoundary(sizeof(_BasicType));

        writeRawData(reinterpret_cast<const char*>(&basicValue), sizeof(_BasicType));

        return *this;
    }

    template<typename _BasicType>
    bool writeBasicTypeValueAtPosition(size_t position, const _BasicType& basicValue) {
        assert(position + sizeof(_BasicType) <= payload_.size());

        return writeRawDataAtPosition(position, reinterpret_cast<const char*>(&basicValue), sizeof(_BasicType));
    }

    DBusOutputStream& writeString(const char* cString, const uint32_t& length);

    /**
     * Fills the stream with 0-bytes to make the next value be aligned to the boundary given.
     * This means that as many 0-bytes are written to the buffer as are necessary
     * to make the next value start with the given alignment.
     *
     * @param alignBoundary The byte-boundary to which the next value should be aligned.
     */
    virtual void alignToBoundary(const size_t alignBoundary);

    /**
     * Takes sizeInByte characters, starting from the character which val points to, and stores them for later writing.
     * When calling flush(), all values that were written to this stream are copied into the payload of the #DBusMessage.
     *
     * The array of characters might be created from a pointer to a given value by using a reinterpret_cast. Example:
     * @code
     * ...
     * int32_t val = 15;
     * outputMessageStream.alignForBasicType(sizeof(int32_t));
     * const char* const reinterpreted = reinterpret_cast<const char*>(&val);
     * outputMessageStream.writeValue(reinterpreted, sizeof(int32_t));
     * ...
     * @endcode
     *
     * @param val The array of chars that should serve as input
     * @param sizeInByte The number of bytes that should be written
     * @return true if writing was successful, false otherwise.
     *
     * @see DBusOutputMessageStream()
     * @see flush()
     */
    bool writeRawData(const char* rawDataPtr, const size_t sizeInByte);

    bool writeRawDataAtPosition(size_t position, const char* rawDataPtr, const size_t sizeInByte);

protected:
    std::string payload_;

private:
    void beginWriteGenericVector();

    void writeSignature(const std::string& signature);

    void rememberCurrentStreamPosition();

    size_t popRememberedStreamPosition();

    size_t getCurrentStreamPosition();

    DBusError dbusError_;
    DBusMessage dbusMessage_;

    std::stack<position_t> savedStreamPositions_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_OUTPUT_MESSAGE_STREAM_H_
