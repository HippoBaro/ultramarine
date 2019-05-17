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

function (getListOfVarsStartingWith _prefix _varResult)
    get_cmake_property(_vars VARIABLES)
    string (REGEX MATCHALL "(^|;)${_prefix}[A-Za-z0-9_]*" _matchedVars "${_vars}")
    set (${_varResult} ${_matchedVars} PARENT_SCOPE)
endfunction()

getListOfVarsStartingWith("Seastar" matchedVars)
foreach (_var IN LISTS matchedVars)
    set(_var "-D${_var}=${${_var}}")
    list(APPEND seastarVars ${_var})
endforeach()
if (seastarVars)
    message(STATUS "Detected Seastar arguments, forwarding " ${seastarVars})
endif()

cooking_ingredient (Seastar
    COOKING_RECIPE
        <DEFAULT>
    COOKING_CMAKE_ARGS
        ${seastarVars}
        -DSeastar_TESTING=OFF
    EXTERNAL_PROJECT_ARGS
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/seastar)

cooking_ingredient (Hash-ring
    EXTERNAL_PROJECT_ARGS
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/hash-ring
        BUILD_IN_SOURCE YES
        CONFIGURE_COMMAND <DISABLE>
        BUILD_COMMAND make lib
        INSTALL_COMMAND PREFIX=<INSTALL_DIR> make install)

