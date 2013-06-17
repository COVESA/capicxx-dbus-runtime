/* Copyright (C) 2013 BMW Group
 * Author: Manfred Bathelt (manfred.bathelt@bmw.de)
 * Author: Juergen Gehring (juergen.gehring@bmw.de)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined (COMMONAPI_INTERNAL_COMPILATION)
#error "Only <CommonAPI/CommonAPI.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef DBUSLEGACYVARIANT_H_
#define DBUSLEGACYVARIANT_H_

#include <CommonAPI/SerializableVariant.h>
#include "DBusOutputStream.h"
#include "DBusInputStream.h"

namespace CommonAPI {
namespace DBus {

template<class Visitor, class Variant, typename ... _Types>
struct ApplyIndexForStringVisitor
;

template<class Visitor, class Variant>
struct ApplyIndexForStringVisitor<Visitor, Variant> {
    static const uint8_t index = 0;

    static uint8_t visit(Visitor&, const Variant&) {
        //won't be called
        assert(false);
        return 0;
    }
};

template<class Visitor, class Variant, typename _Type, typename ... _Types>
struct ApplyIndexForStringVisitor<Visitor, Variant, _Type, _Types...> {
    static const uint8_t index = ApplyIndexForStringVisitor<Visitor, Variant,
                    _Types...>::index + 1;

    static uint8_t visit(Visitor& visitor, const Variant& var) {
        DBusTypeOutputStream typeStream_;
        TypeWriter<_Type>::writeType(typeStream_);
        const std::string sig = typeStream_.retrieveSignature();
        if (visitor.template operator()<_Type>(sig)) {
            return index;
        } else {
            return ApplyIndexForStringVisitor<Visitor, Variant, _Types...>::visit(visitor,
                            var);
        }
    }
};

template<typename ... _Types>
struct TypeOutputStreamCompareVisitor {
public:
    TypeOutputStreamCompareVisitor(const std::string& type) :
        type_(type) {
    }

    template<typename _Type>
    bool operator()(const std::string& ntype) const {
        int comp = type_.compare(0, type_.size(), ntype);
        if (comp == 0) {
            return true;
        } else {
            return false;
        }
    }
private:
    const std::string type_;

};

template< class >
class DBusLegacyVariantWrapper;

template <
    template <class...> class _Type, class... _Types>
class DBusLegacyVariantWrapper<_Type<_Types...>> {
public:

    DBusLegacyVariantWrapper() :
                    contained_() {
    }

    DBusLegacyVariantWrapper(CommonAPI::Variant<_Types...>& cont) :
    contained_(cont) {
    }


    uint8_t getIndexForType(const std::string& type) const {
        TypeOutputStreamCompareVisitor<_Types...> visitor(type);
        return ApplyIndexForStringVisitor<TypeOutputStreamCompareVisitor<_Types...>, Variant<_Types...>, _Types...>::visit(
                        visitor, contained_);
    }

    CommonAPI::Variant<_Types...> contained_;
};

} /* namespace DBus */

//template <template <class...> class _Type, class... _Types>
template <typename ... _Types>
inline OutputStream& operator<<(OutputStream& outputStream, const DBus::DBusLegacyVariantWrapper<Variant<_Types...> >& serializableVariant) {
    DBus::DBusTypeOutputStream typeOutputStream;
    serializableVariant.contained_.writeToTypeOutputStream(typeOutputStream);
    std::string sigStr = typeOutputStream.retrieveSignature();
    uint8_t length = (uint8_t) sigStr.length();
    assert(length < 256);
    outputStream << length;
    outputStream.writeRawData(sigStr.c_str(), length + 1);
    serializableVariant.contained_.writeToOutputStream(outputStream);
    return outputStream;
}

//template <template <class...> class _Type, class... _Types>
template <typename ... _Types>
inline InputStream& operator>>(InputStream& inputStream, DBus::DBusLegacyVariantWrapper<Variant<_Types...> >& serializableVariant) {
    uint8_t signatureLength;
    inputStream.readValue(signatureLength);
    assert(signatureLength < 256);
    char * buf = inputStream.readRawData(signatureLength + 1);
    std::string sigString;
    sigString.assign(buf, buf + signatureLength);
    uint8_t containedTypeIndex = serializableVariant.getIndexForType(sigString);
    serializableVariant.contained_.readFromInputStream(containedTypeIndex, inputStream);
    return inputStream;
}

} /* namespace CommonAPI */

#endif /* DBUSLEGACYVARIANT_H_ */
