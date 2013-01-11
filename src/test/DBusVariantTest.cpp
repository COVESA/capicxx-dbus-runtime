/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <iostream>
#include <string>
#include <tuple>
#include <type_traits>

template <typename _SearchType, typename _CurrentType, typename... _RestTypes>
struct VariantTypeSelector: VariantTypeSelector<_SearchType, _RestTypes...> {
};

template <typename _SearchType, typename... _RestTypes>
struct VariantTypeSelector<_SearchType, _SearchType, _RestTypes...> {
    typedef _SearchType type;
};

template <typename... _Types>
class Variant {
 private:
    typedef std::tuple_size<std::tuple<_Types...>> TypesTupleSize;

 public:
    Variant(): valueType_(TypesTupleSize::value) {
    }

    Variant(const Variant& fromVariant):
        valueType_(fromVariant.valueType_),
        valueStorage_(fromVariant.valueStorage_) {
    }

    Variant(Variant&& fromVariant):
        valueType_(std::move(fromVariant.valueType_)),
        valueStorage_(std::move(fromVariant.valueStorage_)) {
        fromVariant.valueType_ = TypesTupleSize::value;
    }

    ~Variant() {
        if (hasValue()) {
            // TODO call value destructor
        }
    }

    Variant& operator=(const Variant& fromVariant) {
        // TODO
        return *this;
    }

    Variant& operator=(Variant&& fromVariant) {
        // TODO
        return *this;
    }

    // TODO use std::enable_if
    template <typename _Type>
    Variant(const _Type& value) {
        // TODO index by type
        valueType_ = 0;
        new (&valueStorage_) _Type(value);
    }

    // TODO use std::enable_if
    template <typename _Type>
    Variant(_Type && value) {
        // TODO index by type
        valueType_ = 0;
        new (&valueStorage_) typename std::remove_reference<_Type>::type(std::move(value));
    }

    template <typename _Type>
    const typename VariantTypeSelector<_Type, _Types...>::type & get(bool& success) const {
        // TODO assert _Type in _Types
        success = true;
        return *(reinterpret_cast<const _Type *>(&valueStorage_));
    }

 private:
    inline bool hasValue() const {
        return valueType_ < TypesTupleSize::value;
    }

    size_t valueType_;
    // TODO calculate maximum storage
    std::aligned_storage<80>::type valueStorage_;
};



int main(int argc, char** argv) {
    int fromInt = 5;
    Variant<int, double, double, std::string> myVariant(fromInt);
    bool success;
    const int& myInt = myVariant.get<int>(success);
//    const float& myFloat = myVariant.get<float>(success);

    std::cout << "myInt = " << myInt << " (" << std::boolalpha << success << ")\n";
    return 0;
}
