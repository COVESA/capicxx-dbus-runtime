// Copyright (C) 2013-2015 Bayerische Motoren Werke Aktiengesellschaft (BMW AG)
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.hpp> can be included directly, this file may disappear or change contents."
#endif

#ifndef COMMONAPI_DBUS_DBUSINPUTSTREAM_HPP_
#define COMMONAPI_DBUS_DBUSINPUTSTREAM_HPP_

#include <iostream>
#include <iomanip>
#include <sstream>

#include <cassert>
#include <cstdint>
#include <stack>
#include <string>
#include <vector>

#include <CommonAPI/Export.hpp>
#include <CommonAPI/InputStream.hpp>
#include <CommonAPI/Struct.hpp>
#include <CommonAPI/DBus/DBusDeployment.hpp>
#include <CommonAPI/DBus/DBusError.hpp>
#include <CommonAPI/DBus/DBusFreedesktopVariant.hpp>
#include <CommonAPI/DBus/DBusHelper.hpp>
#include <CommonAPI/DBus/DBusMessage.hpp>

namespace CommonAPI {
namespace DBus {

// Used to mark the position of a pointer within an array of bytes.
typedef uint32_t position_t;

/**
 * @class DBusInputMessageStream
 *
 * Used to deserialize and read data from a #DBusMessage. For all data types that can be read from a #DBusMessage, a ">>"-operator should be defined to handle the reading
 * (this operator is predefined for all basic data types and for vectors).
 */
class DBusInputStream
	: public InputStream<DBusInputStream> {
public:
	COMMONAPI_EXPORT bool hasError() const {
        return isErrorSet();
    }

	COMMONAPI_EXPORT InputStream &readValue(bool &_value, const EmptyDeployment *_depl);

	COMMONAPI_EXPORT InputStream &readValue(int8_t &_value, const EmptyDeployment *_depl);
	COMMONAPI_EXPORT InputStream &readValue(int16_t &_value, const EmptyDeployment *_depl);
	COMMONAPI_EXPORT InputStream &readValue(int32_t &_value, const EmptyDeployment *_depl);
	COMMONAPI_EXPORT InputStream &readValue(int64_t &_value, const EmptyDeployment *_depl);

	COMMONAPI_EXPORT InputStream &readValue(uint8_t &_value, const EmptyDeployment *_depl);
	COMMONAPI_EXPORT InputStream &readValue(uint16_t &_value, const EmptyDeployment *_depl);
	COMMONAPI_EXPORT InputStream &readValue(uint32_t &_value, const EmptyDeployment *_depl);
	COMMONAPI_EXPORT InputStream &readValue(uint64_t &_value, const EmptyDeployment *_depl);

	COMMONAPI_EXPORT InputStream &readValue(float &_value, const EmptyDeployment *_depl);
	COMMONAPI_EXPORT InputStream &readValue(double &_value, const EmptyDeployment *_depl);

	COMMONAPI_EXPORT InputStream &readValue(std::string &_value, const EmptyDeployment *_depl);

	COMMONAPI_EXPORT InputStream &readValue(Version &_value, const EmptyDeployment *_depl);

    template<class _Deployment, typename _Base>
	COMMONAPI_EXPORT InputStream &readValue(Enumeration<_Base> &_value, const _Deployment *_depl) {
    	_Base tmpValue;
    	readValue(tmpValue, _depl);
    	_value = tmpValue;
    	return (*this);
    }

    template<class _Deployment, typename... _Types>
	COMMONAPI_EXPORT InputStream &readValue(Struct<_Types...> &_value, const _Deployment *_depl) {
    	align(8);
    	const auto itsSize(std::tuple_size<std::tuple<_Types...>>::value);
    	StructReader<itsSize-1, DBusInputStream, Struct<_Types...>, _Deployment>{}(
    		(*this), _value, _depl);
    	return (*this);
    }

    template<class _Deployment, class _PolymorphicStruct>
	COMMONAPI_EXPORT InputStream &readValue(std::shared_ptr<_PolymorphicStruct> &_value,
    					   const _Deployment *_depl) {
    	uint32_t serial;
    	align(8);
    	_readValue(serial);
    	skipSignature();
    	align(8);
    	if (!hasError()) {
    		_value = _PolymorphicStruct::create(serial);
    		_value->template readValue<>(*this, _depl);
    	}

    	return (*this);
    }

    template<typename... _Types>
	COMMONAPI_EXPORT InputStream &readValue(Variant<_Types...> &_value, const CommonAPI::EmptyDeployment *_depl = nullptr) {
    	if(_value.hasValue()) {
    		DeleteVisitor<_value.maxSize> visitor(_value.valueStorage_);
    		ApplyVoidVisitor<DeleteVisitor<_value.maxSize>,
    			Variant<_Types...>, _Types... >::visit(visitor, _value);
    	}

		align(8);
		readValue(_value.valueType_, static_cast<EmptyDeployment *>(nullptr));
		skipSignature();

		InputStreamReadVisitor<DBusInputStream, _Types...> visitor((*this), _value);
		ApplyVoidVisitor<InputStreamReadVisitor<DBusInputStream, _Types... >,
			Variant<_Types...>, _Types...>::visit(visitor, _value);

		return (*this);
    }

    template<typename _Deployment, typename... _Types>
	COMMONAPI_EXPORT InputStream &readValue(Variant<_Types...> &_value, const _Deployment *_depl) {
    	if(_value.hasValue()) {
    		DeleteVisitor<_value.maxSize> visitor(_value.valueStorage_);
    		ApplyVoidVisitor<DeleteVisitor<_value.maxSize>,
    			Variant<_Types...>, _Types... >::visit(visitor, _value);
    	}

    	if (_depl != nullptr && _depl->isFreeDesktop_) {
			// Read signature
			uint8_t signatureLength;
			readValue(signatureLength, static_cast<EmptyDeployment *>(nullptr));
			std::string signature(_readRaw(signatureLength+1), signatureLength);

			// Determine index (value type) from signature
			TypeCompareVisitor<_Types...> visitor(signature);
			_value.valueType_ = ApplyTypeCompareVisitor<
									TypeCompareVisitor<_Types...>,
									Variant<_Types...>,
									_Types...
								>::visit(visitor, _value);
    	} else {
    		align(8);
    		readValue(_value.valueType_, static_cast<EmptyDeployment *>(nullptr));
    		skipSignature();
    	}


		InputStreamReadVisitor<DBusInputStream, _Types...> visitor((*this), _value);
		ApplyVoidVisitor<InputStreamReadVisitor<DBusInputStream, _Types... >,
			Variant<_Types...>, _Types...>::visit(visitor, _value);

		return (*this);
    }

    template<typename _ElementType>
	COMMONAPI_EXPORT InputStream &readValue(std::vector<_ElementType> &_value, const EmptyDeployment *_depl) {
    	uint32_t itsSize;
    	_readValue(itsSize);
    	pushSize(itsSize);

    	alignVector<_ElementType>();

    	pushPosition();

    	_value.clear();
    	while (sizes_.top() > current_ - positions_.top()) {
    		_ElementType itsElement;
    		readValue(itsElement, static_cast<EmptyDeployment *>(nullptr));

    		if (hasError()) {
    			break;
    		}

    		_value.push_back(std::move(itsElement));
    	}

    	popSize();
    	popPosition();

    	return (*this);
    }

    template<class _Deployment, typename _ElementType>
	COMMONAPI_EXPORT InputStream &readValue(std::vector<_ElementType> &_value, const _Deployment *_depl) {
    	uint32_t itsSize;
    	_readValue(itsSize);
    	pushSize(itsSize);

    	alignVector<_ElementType>();

    	pushPosition();

    	_value.clear();
    	while (sizes_.top() > current_ - positions_.top()) {
    		_ElementType itsElement;
    		readValue(itsElement, _depl->elementDepl_);

    		if (hasError()) {
    			break;
    		}

    		_value.push_back(std::move(itsElement));
    	}

    	popSize();
    	popPosition();

    	return (*this);
    }

    template<typename _KeyType, typename _ValueType, typename _HasherType>
	COMMONAPI_EXPORT InputStream &readValue(std::unordered_map<_KeyType, _ValueType, _HasherType> &_value,
        				   const EmptyDeployment *_depl) {

    	typedef typename std::unordered_map<_KeyType, _ValueType, _HasherType>::value_type MapElement;

    	uint32_t itsSize;
    	_readValue(itsSize);
    	pushSize(itsSize);

    	align(8);
    	pushPosition();

    	_value.clear();
    	while (sizes_.top() > current_ - positions_.top()) {
    		_KeyType itsKey;
    		_ValueType itsValue;

    		align(8);
    		readValue(itsKey, _depl);
    		readValue(itsValue, _depl);

    		if (hasError()) {
    			break;
    		}

    		_value.insert(MapElement(std::move(itsKey), std::move(itsValue)));
    	}

    	(void)popSize();
    	(void)popPosition();

    	return (*this);
    }

    template<class _Deployment, typename _KeyType, typename _ValueType, typename _HasherType>
	COMMONAPI_EXPORT InputStream &readValue(std::unordered_map<_KeyType, _ValueType, _HasherType> &_value,
        				   const _Deployment *_depl) {

    	typedef typename std::unordered_map<_KeyType, _ValueType, _HasherType>::value_type MapElement;

    	uint32_t itsSize;
    	_readValue(itsSize);
    	pushSize(itsSize);

    	align(8);
    	pushPosition();

    	_value.clear();
    	while (sizes_.top() > current_ - positions_.top()) {
    		_KeyType itsKey;
    		_ValueType itsValue;

    		align(8);
    		readValue(itsKey, _depl->key_);
    		readValue(itsValue, _depl->value_);

    		if (hasError()) {
    			break;
    		}

    		_value.insert(MapElement(std::move(itsKey), std::move(itsValue)));
    	}

    	(void)popSize();
    	(void)popPosition();

    	return (*this);
    }

    /**
     * Creates a #DBusInputMessageStream which can be used to deserialize and read data from the given #DBusMessage.
     * As no message-signature is checked, the user is responsible to ensure that the correct data types are read in the correct order.
     *
     * @param message the #DBusMessage from which data should be read.
     */
	COMMONAPI_EXPORT DBusInputStream(const CommonAPI::DBus::DBusMessage &_message);
	COMMONAPI_EXPORT DBusInputStream(const DBusInputStream &_stream) = delete;

    /**
     * Destructor; does not call the destructor of the referred #DBusMessage. Make sure to maintain a reference to the
     * #DBusMessage outside of the stream if you intend to make further use of the message.
     */
	COMMONAPI_EXPORT ~DBusInputStream();

    // Marks the stream as erroneous.
	COMMONAPI_EXPORT void setError();

    /**
     * @return An instance of #DBusError if this stream is in an erroneous state, NULL otherwise
     */
	COMMONAPI_EXPORT const DBusError &getError() const;

    /**
     * @return true if this stream is in an erroneous state, false otherwise.
     */
	COMMONAPI_EXPORT bool isErrorSet() const;

    // Marks the state of the stream as cleared from all errors. Further reading is possible afterwards.
    // The stream will have maintained the last valid position from before its state became erroneous.
	COMMONAPI_EXPORT void clearError();

    /**
     * Aligns the stream to the given byte boundary, i.e. the stream skips as many bytes as are necessary to execute the next read
     * starting from the given boundary.
     *
     * @param _boundary the byte boundary to which the stream needs to be aligned.
     */
	COMMONAPI_EXPORT void align(const size_t _boundary);

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
	COMMONAPI_EXPORT char *_readRaw(const size_t _size);

    /**
     * Handles all reading of basic types from a given #DBusInputMessageStream.
     * Basic types in this context are: uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t, float, double.
     * Any types not listed here (especially all complex types, e.g. structs, unions etc.) need to provide a
     * specialized implementation of this operator.
     *
     * @tparam _Type The type of the value that is to be read from the given stream.
     * @param _value The variable in which the retrieved value is to be stored
     * @return The given inputMessageStream to allow for successive reading
     */
    template<typename _Type>
	COMMONAPI_EXPORT DBusInputStream &_readValue(_Type &_value) {
        if (sizeof(_value) > 1)
        	align(sizeof(_Type));

        _value = *(reinterpret_cast<_Type *>(_readRaw(sizeof(_Type))));

        return (*this);
    }

	COMMONAPI_EXPORT DBusInputStream &_readValue(float &_value) {
    	align(sizeof(double));

        _value = (float) (*(reinterpret_cast<double*>(_readRaw(sizeof(double)))));
        return (*this);
    }

private:
	COMMONAPI_EXPORT void pushPosition();
	COMMONAPI_EXPORT size_t popPosition();

	COMMONAPI_EXPORT void pushSize(size_t _size);
	COMMONAPI_EXPORT size_t popSize();

    inline void skipSignature() {
        uint8_t length;
        _readValue(length);
        _readRaw(length + 1);
    }

    template<typename _Type>
	COMMONAPI_EXPORT void alignVector(typename std::enable_if<!std::is_class<_Type>::value>::type * = nullptr,
    				 typename std::enable_if<!is_std_vector<_Type>::value>::type * = nullptr,
    				 typename std::enable_if<!is_std_unordered_map<_Type>::value>::type * = nullptr) {
    	if (4 < sizeof(_Type)) align(8);
    }

    template<typename _Type>
	COMMONAPI_EXPORT void alignVector(typename std::enable_if<!std::is_same<_Type, std::string>::value>::type * = nullptr,
    				 typename std::enable_if<std::is_class<_Type>::value>::type * = nullptr,
    				 typename std::enable_if<!is_std_vector<_Type>::value>::type * = nullptr,
    				 typename std::enable_if<!is_std_unordered_map<_Type>::value>::type * = nullptr) {
    	align(8);
    }

	template<typename _Type>
	COMMONAPI_EXPORT void alignVector(typename std::enable_if<std::is_same<_Type, std::string>::value>::type * = nullptr) {
		// Intentionally do nothing
	}

    template<typename _Type>
	COMMONAPI_EXPORT void alignVector(typename std::enable_if<is_std_vector<_Type>::value>::type * = nullptr) {
    	// Intentionally do nothing
    }

    template<typename _Type>
	COMMONAPI_EXPORT void alignVector(typename std::enable_if<is_std_unordered_map<_Type>::value>::type * = nullptr) {
    	align(4);
    }

    char *begin_;
    size_t current_;
    size_t size_;
    CommonAPI::DBus::DBusError* exception_;
    CommonAPI::DBus::DBusMessage message_;

    std::stack<uint32_t> sizes_;
    std::stack<size_t> positions_;
};

} // namespace DBus
} // namespace CommonAPI

#endif // COMMONAPI_DBUS_DBUS_INPUTSTREAM_HPP_
