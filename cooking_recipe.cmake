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
message(STATUS "Detected Seastar arguments, forwarding " ${seastarVars})

cooking_ingredient (Seastar
    CMAKE_ARGS
        ${seastarVars}
    EXTERNAL_PROJECT_ARGS
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/seastar)