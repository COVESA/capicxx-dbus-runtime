/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef COMMONAPI_TESTS_PREDEFINED_TYPE_COLLECTION_H_
#define COMMONAPI_TESTS_PREDEFINED_TYPE_COLLECTION_H_

#include <CommonAPI/ByteBuffer.h>
#include <CommonAPI/InputStream.h>
#include <CommonAPI/OutputStream.h>
#include <CommonAPI/types.h>
#include <cstdint>
#include <string>

namespace commonapi {
namespace tests {

namespace PredefinedTypeCollection {
    typedef uint8_t TestUInt8;
    typedef uint16_t TestUInt16;
    typedef uint32_t TestUInt32;
    typedef uint64_t TestUInt64;
    typedef int8_t TestInt8;
    typedef int16_t TestInt16;
    typedef int32_t TestInt32;
    typedef int64_t TestInt64;
    typedef bool TestBoolean;
    typedef CommonAPI::ByteBuffer TestByteBuffer;
    typedef double TestDouble;
    typedef float TestFloat;
    typedef std::string TestString;
    enum class WeirdStrangeAlienEnum: int32_t {
        WEIRD,
        STRANGE,
        ALIEN
    };
    
    // XXX Definition of a comparator still is necessary for GCC 4.4.1, topic is fixed since 4.5.1
    struct WeirdStrangeAlienEnumComparator;

inline CommonAPI::InputStream& operator>>(CommonAPI::InputStream& inputStream, WeirdStrangeAlienEnum& enumValue) {
    return inputStream.readEnumValue<int32_t>(enumValue);
}

inline CommonAPI::OutputStream& operator<<(CommonAPI::OutputStream& outputStream, const WeirdStrangeAlienEnum& enumValue) {
    return outputStream.writeEnumValue(static_cast<int32_t>(enumValue));
}

struct WeirdStrangeAlienEnumComparator {
    inline bool operator()(const WeirdStrangeAlienEnum& lhs, const WeirdStrangeAlienEnum& rhs) const {
        return static_cast<int32_t>(lhs) < static_cast<int32_t>(rhs);
    }
};



static inline const char* getTypeCollectionName() {
    return "commonapi.tests.PredefinedTypeCollection";
}


} // namespace PredefinedTypeCollection

} // namespace tests
} // namespace commonapi

namespace CommonAPI {
	
	template<>
	struct BasicTypeWriter<commonapi::tests::PredefinedTypeCollection::WeirdStrangeAlienEnum> {
	    inline static void writeType (CommonAPI::TypeOutputStream& typeStream) {
	        typeStream.writeInt32EnumType();
	    }
	};
	
	template<>
	struct InputStreamVectorHelper<commonapi::tests::PredefinedTypeCollection::WeirdStrangeAlienEnum> {
	    static void beginReadVector(InputStream& inputStream, const std::vector<commonapi::tests::PredefinedTypeCollection::WeirdStrangeAlienEnum>& vectorValue) {
	        inputStream.beginReadInt32EnumVector();
	    }
	};
	
	template <>
	struct OutputStreamVectorHelper<commonapi::tests::PredefinedTypeCollection::WeirdStrangeAlienEnum> {
	    static void beginWriteVector(OutputStream& outputStream, const std::vector<commonapi::tests::PredefinedTypeCollection::WeirdStrangeAlienEnum>& vectorValue) {
	        outputStream.beginWriteInt32EnumVector(vectorValue.size());
	    }
	};
	
}


namespace std {
    template<>
    struct hash<commonapi::tests::PredefinedTypeCollection::WeirdStrangeAlienEnum> {
        inline size_t operator()(const commonapi::tests::PredefinedTypeCollection::WeirdStrangeAlienEnum& weirdStrangeAlienEnum) const {
            return static_cast<int32_t>(weirdStrangeAlienEnum);
        }
    };
}

#endif // COMMONAPI_TESTS_PREDEFINED_TYPE_COLLECTION_H_
