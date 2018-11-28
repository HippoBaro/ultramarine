
#Seastar
set(SEASTAR_ENABLE_WERROR OFF CACHE BOOL "No error on warning")
set(SEASTAR_USE_STD_OPTIONAL_VARIANT_STRINGVIEW ON CACHE BOOL "Use std over boost when possible")
add_subdirectory (seastar)