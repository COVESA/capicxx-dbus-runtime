/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUS_INPUT_STREAM_H_
#define COMMONAPI_DBUS_DBUS_INPUT_STREAM_H_

#include "DBusError.h"
#include "DBusMessage.h"

#include <CommonAPI/InputStream.h>

#include <stdint.h>
#include <cassert>
#include <string>
#include <vector>
#include <stack>

namespace CommonAPI {
namespace DBus {

/**
 * Used to mark the position of a pointer within an array of bytes.
 */
typedef uint32_t position_t;

/**
 * @class DBusInputMessageStream
 *
 * Used to deserialize and read data from a #DBusMessage. For all data types that can be read from a #DBusMessage, a ">>"-operator should be defined to handle the reading
 * (this operator is predefined for all basic data types and for vectors).
 */
class DBusInputStream: public InputStream {
public:
    virtual bool hasError() const {
        return isErrorSet();
    }

    virtual InputStream& readValue(bool& boolValue);

    virtual InputStream& readValue(int8_t& int8Value);
    virtual InputStream& readValue(int16_t& int16Value);
    virtual InputStream& readValue(int32_t& int32Value);
    virtual InputStream& readValue(int64_t& int64Value);

    virtual InputStream& readValue(uint8_t& uint8Value);
    virtual InputStream& readValue(uint16_t& uint16Value);
    virtual InputStream& readValue(uint32_t& uint32Value);
    virtual InputStream& readValue(uint64_t& uint64Value);

    virtual InputStream& readValue(float& floatValue);
    virtual InputStream& readValue(double& doubleValue);

    virtual InputStream& readValue(std::string& stringValue);
    virtual InputStream& readValue(ByteBuffer& byteBufferValue);

    virtual InputStream& readEnumValue(int8_t& int8BackingTypeValue);
    virtual InputStream& readEnumValue(int16_t& int16BackingTypeValue);
    virtual InputStream& readEnumValue(int32_t& int32BackingTypeValue);
    virtual InputStream& readEnumValue(int64_t& int64BackingTypeValue);
    virtual InputStream& readEnumValue(uint8_t& uint8BackingTypeValue);
    virtual InputStream& readEnumValue(uint16_t& uint16BackingTypeValue);
    virtual InputStream& readEnumValue(uint32_t& uint32BackingTypeValue);
    virtual InputStream& readEnumValue(uint64_t& uint64BackingTypeValue);

    virtual InputStream& readVersionValue(Version& versionValue);

    virtual void beginReadSerializableStruct(const SerializableStruct& serializableStruct);
    virtual void endReadSerializableStruct(const SerializableStruct& serializableStruct);

    virtual void beginReadSerializablePolymorphicStruct(uint32_t& serialId);
    virtual void endReadSerializablePolymorphicStruct(const uint32_t& serialId);

    virtual void readSerializableVariant(SerializableVariant& serializableVariant);

    virtual void beginReadBoolVector();
    virtual void beginReadInt8Vector();
    virtual void beginReadInt16Vector();
    virtual void beginReadInt32Vector();
    virtual void beginReadInt64Vector();
    virtual void beginReadUInt8Vector();
    virtual void beginReadUInt16Vector();
    virtual void beginReadUInt32Vector();
    virtual void beginReadUInt64Vector();
    virtual void beginReadFloatVector();
    virtual void beginReadDoubleVector();
    virtual void beginReadStringVector();
    virtual void beginReadByteBufferVector();
    virtual void beginReadVersionVector();

    virtual void beginReadInt8EnumVector();
    virtual void beginReadInt16EnumVector();
    virtual void beginReadInt32EnumVector();
    virtual void beginReadInt64EnumVector();
    virtual void beginReadUInt8EnumVector();
    virtual void beginReadUInt16EnumVector();
    virtual void beginReadUInt32EnumVector();
    virtual void beginReadUInt64EnumVector();

    virtual void beginReadVectorOfSerializableStructs();
    virtual void beginReadVectorOfSerializableVariants();
    virtual void beginReadVectorOfVectors();
    virtual void beginReadVectorOfMaps();

    virtual void beginReadVectorOfSerializablePolymorphicStructs();

    virtual bool hasMoreVectorElements();
    virtual void endReadVector();

    virtual void beginReadMap();
    virtual bool hasMoreMapElements();
    virtual void endReadMap();
    virtual void beginReadMapElement();
    virtual void endReadMapElement();

    /**
     * Creates a #DBusInputMessageStream which can be used to deserialize and read data from the given #DBusMessage.
     * As no message-signature is checked, the user is responsible to ensure that the correct data types are read in the correct order.
     *
     * @param message the #DBusMessage from which data should be read.
     */
    DBusInputStream(const CommonAPI::DBus::DBusMessage& message);
    DBusInputStream(const DBusInputStream& imessagestream) = delete;

    /**
     * Destructor; does not call the destructor of the referred #DBusMessage. Make sure to maintain a reference to the
     * #DBusMessage outside of the stream if you intend to make further use of the message.
     */
    ~DBusInputStream();

    /**
     * Marks the stream as erroneous.
     */
    void setError();

    /**
     * @return An instance of #DBusError if this stream is in an erroneous state, NULL otherwise
     */
    const CommonAPI::DBus::DBusError& getError() const;

    /**
     * @return true if this stream is in an erroneous state, false otherwise.
     */
    bool isErrorSet() const;

    /**
     * Marks the state of the stream as cleared from all errors. Further reading is possible afterwards.
     * The stream will have maintained the last valid position from before its state became erroneous.
     */
    void clearError();

    /**
     * Aligns the stream to the given byte boundary, i.e. the stream skips as many bytes as are necessary to execute the next read
     * starting from the given boundary.
     *
     * @param alignBoundary the byte boundary to which the stream needs to be aligned.
     */
    void alignToBoundary(const size_t alignBoundary);

    /**
     * Reads the given number of bytes and returns them as an array of characters.
     *
     * Actually, for performance reasons this command only returns a pointer to the current position in the stream,
     * and then increases the position of this pointer by the number of bytes indicated by the given parameter.
     * It is the user's responsibility to actually use only the number of bytes he indicated he would use.
     * It is assumed the user knows what kind of value is stored next in the #DBusMessage the data is streamed from.
     * Using a reinterpret_cast on the returned pointer should then restore the original value.
     *
     * Example use case:
     * @code
     * ...
     * inputMessageStream.alignForBasicType(sizeof(int32_t));
     * char* const dataPtr = inputMessageStream.read(sizeof(int32_t));
     * int32_t val = *(reinterpret_cast<int32_t*>(dataPtr));
     * ...
     * @endcode
     */
    char* readRawData(const size_t numBytesToRead);

    /**
     * Handles all reading of basic types from a given #DBusInputMessageStream.
     * Basic types in this context are: uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t, float, double.
     * Any types not listed here (especially all complex types, e.g. structs, unions etc.) need to provide a
     * specialized implementation of this operator.
     *
     * @tparam _BasicType The type of the value that is to be read from the given stream.
     * @param val The variable in which the retrieved value is to be stored
     * @param inputMessageStream The stream which the value is to be read from
     * @return The given inputMessageStream to allow for successive reading
     */
    template<typename _BasicType>
    DBusInputStream& readBasicTypeValue(_BasicType& val) {
        if (sizeof(val) > 1)
            alignToBoundary(sizeof(_BasicType));

        val = *(reinterpret_cast<_BasicType*>(readRawData(sizeof(_BasicType))));
        return *this;
    }

private:
    inline void beginReadGenericVector() {
        uint32_t vectorByteSize;
        readBasicTypeValue(vectorByteSize);
        bytesToRead_.push(vectorByteSize);
    }

    inline void skipOverSignature() {
        uint8_t signatureLength;
        readValue(signatureLength);
        readRawData(signatureLength + 1);
    }

    char* dataBegin_;
    position_t currentDataPosition_;
    size_t dataLength_;
    CommonAPI::DBus::DBusError* exception_;
    CommonAPI::DBus::DBusMessage message_;

    std::stack<uint32_t> bytesToRead_;
    std::stack<position_t> savedStreamPositions_;
};

inline void DBusInputStream::setError() {
    exception_ = new CommonAPI::DBus::DBusError();
}

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_INPUT_STREAM_H_
