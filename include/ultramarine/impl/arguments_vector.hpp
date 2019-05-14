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

#include <vector>
#include <boost/range.hpp>

namespace ultramarine::impl {
    template<typename T>
    struct arguments_vector;

    // And argument vector is a standard vector
    template<typename ...Args>
    struct arguments_vector<std::tuple<Args...>> : private std::vector<std::tuple<Args...>> {
        using std::vector<std::tuple<Args...>>::vector;
        using std::vector<std::tuple<Args...>>::emplace_back;
        using std::vector<std::tuple<Args...>>::begin;
        using std::vector<std::tuple<Args...>>::end;
        using std::vector<std::tuple<Args...>>::size;
    };

    // Template specialization for empty argument pack
    template<>
    struct arguments_vector<std::tuple<>> {
        class constant_value_iterator : public std::iterator<std::input_iterator_tag, std::tuple<>> {
            std::size_t index;
            std::tuple<> dummy;
        public:
            explicit constant_value_iterator(std::size_t index) : index(index) {}

            constant_value_iterator(const constant_value_iterator &) = default;

            constant_value_iterator &operator++() {
                ++index;
                return *this;
            }

            constant_value_iterator operator++(int) {
                constant_value_iterator tmp(*this);
                operator++();
                return tmp;
            }

            bool operator==(const constant_value_iterator &rhs) const { return index == rhs.index; }

            bool operator!=(const constant_value_iterator &rhs) const { return index != rhs.index; }

            std::size_t operator-(const constant_value_iterator &rhs) const { return index - rhs.index; }

            std::tuple<> &operator*() { return dummy; }
        };

        boost::iterator_range<constant_value_iterator> range;

        arguments_vector<std::tuple<>>() : range(constant_value_iterator(0), constant_value_iterator(0)) {}

        arguments_vector(arguments_vector const &) = default;

        arguments_vector(arguments_vector &&) = default;

        void emplace_back(std::tuple<> &&) {
            range = boost::iterator_range<constant_value_iterator>(constant_value_iterator(0),
                                                                   constant_value_iterator(++range.end()));
        }

        auto begin() const { return range.begin(); }

        auto end() const { return range.end(); }

        auto size() const { return range.end() - range.begin(); }
    };
}
