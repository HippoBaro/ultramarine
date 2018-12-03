
#Seastar
set(SEASTAR_ENABLE_WERROR OFF CACHE BOOL "No error on warning")
set(SEASTAR_USE_STD_OPTIONAL_VARIANT_STRINGVIEW ON CACHE BOOL "Use std over boost when possible")
set(SEASTAR_EXECUTE_ONLY_FAST_TESTS ON CACHE BOOL, "Execute only fast tests")
add_subdirectory (seastar)