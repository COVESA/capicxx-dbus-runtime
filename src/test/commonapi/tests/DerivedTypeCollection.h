/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_TESTS_DERIVED_TYPE_COLLECTION_H_
#define COMMONAPI_TESTS_DERIVED_TYPE_COLLECTION_H_

#include <CommonAPI/InputStream.h>
#include <CommonAPI/OutputStream.h>
#include <CommonAPI/SerializableStruct.h>
#include <CommonAPI/types.h>
#include "PredefinedTypeCollection.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace commonapi {
namespace tests {

namespace DerivedTypeCollection {

enum class TestEnum: int32_t {
    E_UNKNOWN = 0,
    E_OK = 1,
    E_OUT_OF_RANGE = 2,
    E_NOT_USED = 3
};

// XXX Definition of a comparator still is necessary for GCC 4.4.1, topic is fixed since 4.5.1
struct TestEnumComparator;

enum class TestEnumMissingValue: int32_t {
    E1 = 10,
    E2,
    E3 = 2
};

// XXX Definition of a comparator still is necessary for GCC 4.4.1, topic is fixed since 4.5.1
struct TestEnumMissingValueComparator;

enum class TestEnumExtended: int32_t {
    E_UNKNOWN = TestEnum::E_UNKNOWN,
    E_OK = TestEnum::E_OK,
    E_OUT_OF_RANGE = TestEnum::E_OUT_OF_RANGE,
    E_NOT_USED = TestEnum::E_NOT_USED
    ,
    E_NEW = 4
};

// XXX Definition of a comparator still is necessary for GCC 4.4.1, topic is fixed since 4.5.1
struct TestEnumExtendedComparator;

enum class TestEnumExtended2: int32_t {
    E_UNKNOWN = TestEnum::E_UNKNOWN,
    E_OK = TestEnum::E_OK,
    E_OUT_OF_RANGE = TestEnum::E_OUT_OF_RANGE,
    E_NOT_USED = TestEnum::E_NOT_USED,
    
    E_NEW = TestEnumExtended::E_NEW
    ,
    E_NEW2 = 5
};

// XXX Definition of a comparator still is necessary for GCC 4.4.1, topic is fixed since 4.5.1
struct TestEnumExtended2Comparator;

struct TestStruct: CommonAPI::SerializableStruct {
    PredefinedTypeCollection::TestString testString;
    uint16_t uintValue;
    
    TestStruct() = default;
    TestStruct(const PredefinedTypeCollection::TestString& testString, const uint16_t& uintValue);

    virtual void readFromInputStream(CommonAPI::InputStream& inputStream);
    virtual void writeToOutputStream(CommonAPI::OutputStream& outputStream) const;
};

struct TestStructExtended: TestStruct {
    TestEnumExtended2 testEnumExtended2;
    
    TestStructExtended() = default;
    TestStructExtended(const PredefinedTypeCollection::TestString& testString, const uint16_t& uintValue, const TestEnumExtended2& testEnumExtended2);

    virtual void readFromInputStream(CommonAPI::InputStream& inputStream);
    virtual void writeToOutputStream(CommonAPI::OutputStream& outputStream) const;
};

typedef std::vector<uint64_t> TestArrayUInt64;

typedef std::vector<TestStruct> TestArrayTestStruct;

typedef std::unordered_map<uint32_t, TestArrayTestStruct> TestMap;

inline CommonAPI::InputStream& operator>>(CommonAPI::InputStream& inputStream, TestEnum& enumValue) {
    return inputStream.readEnumValue<int32_t>(enumValue);
}

inline CommonAPI::OutputStream& operator<<(CommonAPI::OutputStream& outputStream, const TestEnum& enumValue) {
    return outputStream.writeEnumValue(static_cast<int32_t>(enumValue));
}

struct TestEnumComparator {
    inline bool operator()(const TestEnum& lhs, const TestEnum& rhs) const {
        return static_cast<int32_t>(lhs) < static_cast<int32_t>(rhs);
    }
};

inline CommonAPI::InputStream& operator>>(CommonAPI::InputStream& inputStream, TestEnumMissingValue& enumValue) {
    return inputStream.readEnumValue<int32_t>(enumValue);
}

inline CommonAPI::OutputStream& operator<<(CommonAPI::OutputStream& outputStream, const TestEnumMissingValue& enumValue) {
    return outputStream.writeEnumValue(static_cast<int32_t>(enumValue));
}

struct TestEnumMissingValueComparator {
    inline bool operator()(const TestEnumMissingValue& lhs, const TestEnumMissingValue& rhs) const {
        return static_cast<int32_t>(lhs) < static_cast<int32_t>(rhs);
    }
};

inline CommonAPI::InputStream& operator>>(CommonAPI::InputStream& inputStream, TestEnumExtended& enumValue) {
    return inputStream.readEnumValue<int32_t>(enumValue);
}

inline CommonAPI::OutputStream& operator<<(CommonAPI::OutputStream& outputStream, const TestEnumExtended& enumValue) {
    return outputStream.writeEnumValue(static_cast<int32_t>(enumValue));
}

struct TestEnumExtendedComparator {
    inline bool operator()(const TestEnumExtended& lhs, const TestEnumExtended& rhs) const {
        return static_cast<int32_t>(lhs) < static_cast<int32_t>(rhs);
    }
};


inline bool operator==(const TestEnumExtended& lhs, const TestEnum& rhs) {
    return static_cast<int32_t>(lhs) == static_cast<int32_t>(rhs);
}
inline bool operator==(const TestEnum& lhs, const TestEnumExtended& rhs) {
    return static_cast<int32_t>(lhs) == static_cast<int32_t>(rhs);
}
inline bool operator!=(const TestEnumExtended& lhs, const TestEnum& rhs) {
    return static_cast<int32_t>(lhs) != static_cast<int32_t>(rhs);
}
inline bool operator!=(const TestEnum& lhs, const TestEnumExtended& rhs) {
    return static_cast<int32_t>(lhs) != static_cast<int32_t>(rhs);
}
inline CommonAPI::InputStream& operator>>(CommonAPI::InputStream& inputStream, TestEnumExtended2& enumValue) {
    return inputStream.readEnumValue<int32_t>(enumValue);
}

inline CommonAPI::OutputStream& operator<<(CommonAPI::OutputStream& outputStream, const TestEnumExtended2& enumValue) {
    return outputStream.writeEnumValue(static_cast<int32_t>(enumValue));
}

struct TestEnumExtended2Comparator {
    inline bool operator()(const TestEnumExtended2& lhs, const TestEnumExtended2& rhs) const {
        return static_cast<int32_t>(lhs) < static_cast<int32_t>(rhs);
    }
};


inline bool operator==(const TestEnumExtended2& lhs, const TestEnum& rhs) {
    return static_cast<int32_t>(lhs) == static_cast<int32_t>(rhs);
}
inline bool operator==(const TestEnum& lhs, const TestEnumExtended2& rhs) {
    return static_cast<int32_t>(lhs) == static_cast<int32_t>(rhs);
}
inline bool operator!=(const TestEnumExtended2& lhs, const TestEnum& rhs) {
    return static_cast<int32_t>(lhs) != static_cast<int32_t>(rhs);
}
inline bool operator!=(const TestEnum& lhs, const TestEnumExtended2& rhs) {
    return static_cast<int32_t>(lhs) != static_cast<int32_t>(rhs);
}

inline bool operator==(const TestEnumExtended2& lhs, const TestEnumExtended& rhs) {
    return static_cast<int32_t>(lhs) == static_cast<int32_t>(rhs);
}
inline bool operator==(const TestEnumExtended& lhs, const TestEnumExtended2& rhs) {
    return static_cast<int32_t>(lhs) == static_cast<int32_t>(rhs);
}
inline bool operator!=(const TestEnumExtended2& lhs, const TestEnumExtended& rhs) {
    return static_cast<int32_t>(lhs) != static_cast<int32_t>(rhs);
}
inline bool operator!=(const TestEnumExtended& lhs, const TestEnumExtended2& rhs) {
    return static_cast<int32_t>(lhs) != static_cast<int32_t>(rhs);
}
bool operator==(const TestStruct& lhs, const TestStruct& rhs);
inline bool operator!=(const TestStruct& lhs, const TestStruct& rhs) {
    return !(lhs == rhs);
}
bool operator==(const TestStructExtended& lhs, const TestStructExtended& rhs);
inline bool operator!=(const TestStructExtended& lhs, const TestStructExtended& rhs) {
    return !(lhs == rhs);
}


static inline const char* getTypeCollectionName() {
    return "commonapi.tests.DerivedTypeCollection";
}


} // namespace DerivedTypeCollection

} // namespace tests
} // namespace commonapi

#endif // COMMONAPI_TESTS_DERIVED_TYPE_COLLECTION_H_
