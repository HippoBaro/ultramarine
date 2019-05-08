/*
 * MIT License
 *
 * Copyright (c) 2018 Hippolyte Barraud
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <string>
#include <seastar/rpc/rpc.hh>
#include <seastar/rpc/rpc_types.hh>

namespace ultramarine::cluster {

    struct serializer {
    };

    using rpc_proto = seastar::rpc::protocol<ultramarine::cluster::serializer>;

    template<typename T, typename Output>
    inline static
    void write_arithmetic_type(Output &out, T v) {
        static_assert(std::is_arithmetic<T>::value, "must be arithmetic type");
        return out.write(reinterpret_cast<const char *>(&v), sizeof(T));
    }

    template<typename T, typename Input>
    inline static
    T read_arithmetic_type(Input &in) {
        static_assert(std::is_arithmetic<T>::value, "must be arithmetic type");
        T v;
        in.read(reinterpret_cast<char *>(&v), sizeof(T));
        return v;
    }

    template<typename Output>
    inline static void write(serializer, Output &output, int32_t v) { return write_arithmetic_type(output, v); }

    template<typename Output>
    inline static void write(serializer, Output &output, uint32_t v) { return write_arithmetic_type(output, v); }

    template<typename Output>
    inline static void write(serializer, Output &output, int64_t v) { return write_arithmetic_type(output, v); }

    template<typename Output>
    inline static void write(serializer, Output &output, uint64_t v) { return write_arithmetic_type(output, v); }

    template<typename Output>
    inline static void write(serializer, Output &output, double v) { return write_arithmetic_type(output, v); }

    template<typename Input>
    inline static int32_t read(serializer, Input &input, seastar::rpc::type<int32_t>) {
        return read_arithmetic_type<int32_t>(input);
    }

    template<typename Input>
    inline static uint32_t
    read(serializer, Input &input, seastar::rpc::type<uint32_t>) { return read_arithmetic_type<uint32_t>(input); }

    template<typename Input>
    inline static uint64_t
    read(serializer, Input &input, seastar::rpc::type<uint64_t>) { return read_arithmetic_type<uint64_t>(input); }

    template<typename Input>
    inline static uint64_t read(serializer, Input &input, seastar::rpc::type<int64_t>) {
        return read_arithmetic_type<int64_t>(input);
    }

    template<typename Input>
    inline static double read(serializer, Input &input, seastar::rpc::type<double>) {
        return read_arithmetic_type<double>(input);
    }

    template<typename Output>
    inline static void write(serializer, Output &out, const std::string &v) {
        write_arithmetic_type(out, uint32_t(v.size()));
        out.write(v.c_str(), v.size());
    }

    template<typename Input>
    inline static std::string read(serializer, Input &in, seastar::rpc::type<std::string>) {
        auto size = read_arithmetic_type<uint32_t>(in);
        std::string ret(size, '\0');
        in.read(ret.data(), size);
        return ret;
    }

    template <typename Output>
    inline void write(serializer, Output& out, const seastar::sstring& v) {
        write_arithmetic_type(out, uint32_t(v.size()));
        out.write(v.c_str(), v.size());
    }

    template <typename Input>
    inline seastar::sstring read(serializer, Input& in, seastar::rpc::type<seastar::sstring>) {
        auto size = read_arithmetic_type<uint32_t>(in);
        seastar::sstring ret(seastar::sstring::initialized_later(), size);
        in.read(ret.begin(), size);
        return ret;
    }

    template <typename Output, typename T>
    inline void write(serializer s, Output& out, const std::vector<T>& v) {
        write_arithmetic_type(out, uint32_t(v.size()));
        for (const auto &item : v) {
            item.serialize(s, out);
        }
    }

    template <typename Input, typename T>
    inline std::vector<T> read(serializer s, Input& in, seastar::rpc::type<std::vector<T>>) {
        auto size = read_arithmetic_type<uint32_t>(in);
        std::vector<T> ret(size);
        for (int j = 0; j < size; ++j) {
            T::deserialize(s, in);
        }
        return ret;
    }

    //Fallback on object member
    template <typename Output, typename T>
    inline void write(serializer s, Output& out, const T& v) {
        v.serialize(s, out);
    }

    //Fallback on object member
    template <typename Input, typename T>
    inline T read(serializer s, Input& in, seastar::rpc::type<T>) {
        return T::deserialize(s, in);
    }

}