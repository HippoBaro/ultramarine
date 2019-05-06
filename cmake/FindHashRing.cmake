#
# MIT License
#
# Copyright (c) 2018 Hippolyte Barraud
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

find_library (hashring_LIBRARY
        NAMES hashring
        HINTS
        ${hashring_LIBDIR}
        ${hashring_LIBRARY_DIRS})

find_path (hashring_INCLUDE_DIR
        NAMES hash_ring.h
        HINTS
        ${hashring_INCLUDEDIR}
        ${hashring_INCLUDEDIRS})

mark_as_advanced (
        hashring_LIBRARY
        hashring_INCLUDE_DIR)

include (FindPackageHandleStandardArgs)

find_package_handle_standard_args (hashring
        REQUIRED_VARS
        hashring_LIBRARY
        hashring_INCLUDE_DIR
        VERSION_VAR hashring_VERSION)

set (hashring_LIBRARIES ${hashring_LIBRARY})
set (hashring_INCLUDE_DIRS ${hashring_INCLUDE_DIR})

if (hashring_FOUND AND NOT (TARGET hashring::hashring))
    add_library (hashring::hashring SHARED IMPORTED)
    set_target_properties (hashring::hashring
            PROPERTIES
            IMPORTED_LOCATION ${hashring_LIBRARIES}
            INTERFACE_INCLUDE_DIRECTORIES ${hashring_INCLUDE_DIRS})
endif ()