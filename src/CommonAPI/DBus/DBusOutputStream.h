/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
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

namespace CommonAPI {
namespace DBus {

/**
 * Used to mark the position of a pointer within an array of bytes.
 */
typedef uint32_t position_t;

class DBusVariantOutputStream;


/**
 * @class DBusOutputMessageStream
 *
 * Used to serialize and write data into a #DBusMessage. For all data types that may be written to a #DBusMessage, a "<<"-operator should be defined to handle the writing
 * (this operator is predefined for all basic data types and for vectors). The signature that has to be written to the #DBusMessage separately is assumed
 * to match the actual data that is inserted via the #DBusOutputMessageStream.
 */
class DBusOutputStream: public OutputStream {
  public:
	virtual OutputStream& writeValue(const bool& boolValue) { return writeBasicTypeValue<uint32_t>(boolValue); }

	virtual OutputStream& writeValue(const int8_t& int8Value) { return writeBasicTypeValue(int8Value); }
	virtual OutputStream& writeValue(const int16_t& int16Value) { return writeBasicTypeValue(int16Value); }
	virtual OutputStream& writeValue(const int32_t& int32Value) { return writeBasicTypeValue(int32Value); }
	virtual OutputStream& writeValue(const int64_t& int64Value) { return writeBasicTypeValue(int64Value); }

	virtual OutputStream& writeValue(const uint8_t& uint8Value) { return writeBasicTypeValue(uint8Value); }
	virtual OutputStream& writeValue(const uint16_t& uint16Value) { return writeBasicTypeValue(uint16Value); }
	virtual OutputStream& writeValue(const uint32_t& uint32Value) { return writeBasicTypeValue(uint32Value); }
	virtual OutputStream& writeValue(const uint64_t& uint64Value) { return writeBasicTypeValue(uint64Value); }

	virtual OutputStream& writeValue(const float& floatValue) { return writeBasicTypeValue((double) floatValue); }
	virtual OutputStream& writeValue(const double& doubleValue) { return writeBasicTypeValue(doubleValue); }

	virtual OutputStream& writeValue(const std::string& stringValue) { return writeString(stringValue.c_str(), stringValue.length()); }

	inline virtual OutputStream& writeValue(const ByteBuffer& byteBufferValue);

 	virtual OutputStream& writeEnumValue(const int8_t& int8BackingTypeValue) { return writeValue(int8BackingTypeValue); }
 	virtual OutputStream& writeEnumValue(const int16_t& int16BackingTypeValue) { return writeValue(int16BackingTypeValue); }
 	virtual OutputStream& writeEnumValue(const int32_t& int32BackingTypeValue) { return writeValue(int32BackingTypeValue); }
 	virtual OutputStream& writeEnumValue(const int64_t& int64BackingTypeValue) { return writeValue(int64BackingTypeValue); }
 	virtual OutputStream& writeEnumValue(const uint8_t& uint8BackingTypeValue) { return writeValue(uint8BackingTypeValue); }
 	virtual OutputStream& writeEnumValue(const uint16_t& uint16BackingTypeValue) { return writeValue(uint16BackingTypeValue); }
 	virtual OutputStream& writeEnumValue(const uint32_t& uint32BackingTypeValue) { return writeValue(uint32BackingTypeValue); }
 	virtual OutputStream& writeEnumValue(const uint64_t& uint64BackingTypeValue) { return writeValue(uint64BackingTypeValue); }

    virtual void beginWriteBoolVector(uint32_t sizeOfVector) {
        beginWriteGenericVector();
        rememberCurrentStreamPosition();
    }
    virtual void beginWriteInt8Vector(uint32_t sizeOfVector) {
        beginWriteGenericVector();
        rememberCurrentStreamPosition();
    }
    virtual void beginWriteInt16Vector(uint32_t sizeOfVector) {
        beginWriteGenericVector();
        rememberCurrentStreamPosition();
    }
    virtual void beginWriteInt32Vector(uint32_t sizeOfVector) {
        beginWriteGenericVector();
        rememberCurrentStreamPosition();
    }
    virtual void beginWriteInt64Vector(uint32_t sizeOfVector) {
        beginWriteGenericVector();
        alignToBoundary(8);
        rememberCurrentStreamPosition();
    }
    virtual void beginWriteUInt8Vector(uint32_t sizeOfVector) {
        beginWriteGenericVector();
        rememberCurrentStreamPosition();
    }
    virtual void beginWriteUInt16Vector(uint32_t sizeOfVector) {
        beginWriteGenericVector();
        rememberCurrentStreamPosition();
    }
    virtual void beginWriteUInt32Vector(uint32_t sizeOfVector) {
        beginWriteGenericVector();
        rememberCurrentStreamPosition();
    }
    virtual void beginWriteUInt64Vector(uint32_t sizeOfVector) {
        beginWriteGenericVector();
        alignToBoundary(8);
        rememberCurrentStreamPosition();
    }
    virtual void beginWriteFloatVector(uint32_t sizeOfVector) {
        beginWriteGenericVector();
        alignToBoundary(8);
        rememberCurrentStreamPosition();
    }
    virtual void beginWriteDoubleVector(uint32_t sizeOfVector) {
        beginWriteGenericVector();
        alignToBoundary(8);
        rememberCurrentStreamPosition();
    }
    virtual void beginWriteStringVector(uint32_t sizeOfVector) {
        beginWriteGenericVector();
        rememberCurrentStreamPosition();
    }
    virtual void beginWriteByteBufferVector(uint32_t sizeOfVector) {
        beginWriteGenericVector();
        rememberCurrentStreamPosition();
    }
    virtual void beginWriteVersionVector(uint32_t sizeOfVector) {
        beginWriteGenericVector();
        alignToBoundary(8);
        rememberCurrentStreamPosition();
    }
    virtual void beginWriteVectorOfSerializableStructs(uint32_t sizeOfVector) {
        beginWriteGenericVector();
        alignToBoundary(8);
        rememberCurrentStreamPosition();
    }
    virtual void beginWriteVectorOfVectors(uint32_t sizeOfVector) {
        beginWriteGenericVector();
        rememberCurrentStreamPosition();
    }
    virtual void beginWriteVectorOfMaps(uint32_t sizeOfVector) {
        beginWriteGenericVector();
        alignToBoundary(8);
        rememberCurrentStreamPosition();
    }

    virtual void endWriteVector() {
        uint32_t numOfWrittenBytes = getCurrentStreamPosition() - popRememberedStreamPosition();
        writeBasicTypeValueAtPosition(popRememberedStreamPosition(), numOfWrittenBytes);
    }

 	virtual OutputStream& writeVersionValue(const Version& versionValue) {
 		alignToBoundary(8);
 		writeValue(versionValue.Major);
 		writeValue(versionValue.Minor);
 		return *this;
 	}

 	virtual void beginWriteSerializableStruct(const SerializableStruct& serializableStruct) { alignToBoundary(8); }
	virtual void endWriteSerializableStruct(const SerializableStruct& serializableStruct) { }

	virtual void beginWriteMap(size_t elementCount) {
        alignToBoundary(sizeof(uint32_t));
        rememberCurrentStreamPosition();
        writeBasicTypeValue((uint32_t) 0);
	    alignToBoundary(8);
        rememberCurrentStreamPosition();
	}

	virtual void endWriteMap() {
        uint32_t numOfWrittenBytes = getCurrentStreamPosition() - popRememberedStreamPosition();
        writeBasicTypeValueAtPosition(popRememberedStreamPosition(), numOfWrittenBytes);
	}

    virtual void beginWriteVariant(const SerializableVariant& serializableVariant) {
        writeValue(serializableVariant.getValueType());
    }

    virtual void endWriteVariant(const SerializableVariant& serializableVariant) {
        //TODO
    }

	virtual bool hasError() const {
		return dbusError_;
	}

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

    /**
     * Writes the data that was buffered within this #DBusOutputMessageStream to the #DBusMessage that was given to the constructor. Each call to flush()
     * will completely override the data that currently is contained in the #DBusMessage. The data that is buffered in this #DBusOutputMessageStream is
     * not deleted by calling flush().
     */
    void flush();

    /**
     * Marks the stream as erroneous.
     */
    inline void setError();

    /**
     * Reserves the given number of bytes for writing, thereby negating the need to dynamically allocate memory while writing.
     * Use this method for optimization: If possible, reserve as many bytes as you need for your data before doing any writing.
     *
     * @param numOfBytes The number of bytes that should be reserved for writing.
     */
    void reserveMemory(size_t numOfBytes);

    /**
     * @return current data position where later writing is possible
     */
    size_t getCurrentPosition();

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
    inline void beginWriteGenericVector() {
        alignToBoundary(sizeof(uint32_t));
        rememberCurrentStreamPosition();
        writeBasicTypeValue((uint32_t) 0);
    }

    inline void writeSignature(std::string& signature) {
        uint8_t length = (uint8_t) signature.length();
        assert(length < 256);
        *this << length;
        writeRawData(signature.c_str(), length + 1);
    }

    inline void rememberCurrentStreamPosition() {
        savedStreamPositions_.push(payload_.size());
    }

    inline size_t popRememberedStreamPosition() {
        size_t val = savedStreamPositions_.top();
        savedStreamPositions_.pop();
        return val;
    }

    inline size_t getCurrentStreamPosition() {
        return payload_.size();
    }

    DBusError dbusError_;
    DBusMessage dbusMessage_;

    std::stack<position_t> savedStreamPositions_;
};


//Additional 0-termination, so this is 8 byte of \0
static const char* eightByteZeroString = "\0\0\0\0\0\0\0";

inline void DBusOutputStream::alignToBoundary(const size_t alignBoundary) {
    assert(alignBoundary > 0 && alignBoundary <= 8 && (alignBoundary % 2 == 0 || alignBoundary == 1));

    size_t alignMask = alignBoundary - 1;
    size_t necessaryAlignment = ((alignMask - (payload_.size() & alignMask)) + 1) & alignMask;

    writeRawData(eightByteZeroString, necessaryAlignment);
}

inline bool DBusOutputStream::writeRawData(const char* rawDataPtr, const size_t sizeInByte) {
    assert(sizeInByte >= 0);

    payload_.append(rawDataPtr, sizeInByte);

    return true;
}

inline bool DBusOutputStream::writeRawDataAtPosition(size_t position, const char* rawDataPtr, const size_t sizeInByte) {
    assert(sizeInByte >= 0);

    payload_ = payload_.replace(position, sizeInByte, rawDataPtr, sizeInByte);

    return true;
}

inline size_t DBusOutputStream::getCurrentPosition() {
    return payload_.size();
}


OutputStream& DBusOutputStream::writeValue(const ByteBuffer& byteBufferValue) {
	*this << byteBufferValue;
	return *this;
}

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_OUTPUT_MESSAGE_STREAM_H_
