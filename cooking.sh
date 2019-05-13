#!/bin/bash

#
# Copyright 2018 Jesse Haber-Kucharsky
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#
# This is cmake-cooking v0.11.0
# The home of cmake-cooking is https://github.com/hakuch/CMakeCooking
#

set -e

CMAKE=${CMAKE:-cmake}

invoked_args=("$@")
source_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
initial_wd=$(pwd)
memory_file="${initial_wd}/.cooking_memory"

recipe="${source_dir}/cooking_recipe.cmake"
build_args=""
jobs=""
declare -a excluded_ingredients
declare -a included_ingredients
build_dir="${initial_wd}/build"
build_type="Debug"
# Depends on `build_dir`.
ingredients_dir=""
generator="Ninja"
list_only=""
nested=""

usage() {
    cat <<EOF

Fetch, configure, build, and install dependencies ("ingredients") for a CMake project
in a local and repeatable development environment.

Usage: $0 [OPTIONS]

where OPTIONS are:

-a
-b BUILD_ARGS
-j JOBS
-r RECIPE
-e INGREDIENT
-i INGREDIENT
-d BUILD_DIR (=${build_dir})
-p INGREDIENTS_DIR (=${build_dir}/_cooking/installed)
-t BUILD_TYPE (=${build_type})
-g GENERATOR (=${generator})
-s VAR=VALUE
-f EXPORT_DIR
-l
-h

By default, cmake-cooking reads a file called 'cooking_recipe.cmake'.

If neither [-i] nor [-e] are specified with a recipe ([-r]), then all ingredients of the recipe
will be fetched and built.

[-i] and [-e] are mutually-exclusive options: only provide one.

Option details:

-a

    Invoke 'cooking.sh' with the arguments that were provided to it last time, instead
    of the arguments provided.

-b BUILD_ARGS

    Space-separated options which are forwarded to the build tool.

-j JOBS

    Invoke the underlying build tool with this many parallel jobs.

-r RECIPE

    Instead of reading the recipe in a file called 'cooking_recipe.cmake', use the named recipe.

    For a recipe named "foo", a file is expected to exist called 'recipe/foo.cmake' relative to
    the source directory of the project.

-e INGREDIENT

    Exclude an ingredient from a recipe. This option can be supplied many times.

    For example, if a recipe consists of 'apple', 'banana', 'carrot', and 'donut', then

        ./cooking.sh -e apple -e donut

    will prepare 'banana' and 'carrot' but not prepare 'apple' and 'donut'.

    If an ingredient is excluded, then it is assumed that all ingredients that depend on it
    can satisfy that dependency in some other way from the system (ie, the dependency is
    removed internally).

-i INGREDIENT

    Include an ingredient from a recipe, ignoring the others. This option can be supplied
    many times.

    Similar to [-e], but the opposite.

    For example, if a recipe consists of 'apple', 'banana', 'carrot', and 'donut' then

        ./cooking.sh -i apple -i donut

    will prepare 'apple' and 'donut' but not prepare 'banana' and 'carrot'.

    If an ingredient is not in the "include-list", then it is assumed that all
    ingredients that are in the list and which depend on it can satisfy that dependency
    in some other way from the system.

-d BUILD_DIR (=${build_dir})

    Configure the project and build it in the named directory.

-p INGREDIENTS_DIR (=${build_dir}/_cooking/installed)

    Install compiled ingredients into this directory.

-t BUILD_TYPE (=${build_type})

    Configure all ingredients and the project with the named CMake build-type.
    An example build type is "Release".

-g GENERATOR (=${generator})

    Use the named CMake generator for building all ingredients and the project.
    An example generator is "Unix Makfiles".

-s VAR=VALUE

    Set an environmental variable 'VAR' to the value 'VALUE' during the invocation of CMake.

-f EXPORT_DIR

    If provided, and the project is successfully configured, then the tree of installed ingredients
    is exported to this directory (the actual files: not symbolic links).

    This option requires rsync.

    This may be useful for preparing continuous integration environments, but it is not
    recommended for distribution or release purposes (since this would be counter
    to the goal of cmake-cooking).

-l

    Only list available ingredients for a given recipe, and don't do anything else.

-h

    Show this help information and exit.

EOF
}

parse_assignment() {
    IFS='=' read -ra parts <<< "${1}"
    export "${parts[0]}"="${parts[1]}"
}

yell_include_exclude_mutually_exclusive() {
    echo "Cooking: [-e] and [-i] are mutually exclusive options!" >&2
}

yell_parallel_make_not_supported() {
    echo "Cooking: The '${generator}' generator does not support parallelization" >&2
    echo "Cooking: See https://gitlab.kitware.com/cmake/cmake/issues/18663" >&2
}

while getopts "ab:j:r:e:i:d:p:t:g:s:f:lhx" arg; do
    case "${arg}" in
        a)
            if [ ! -f "${memory_file}" ]; then
                echo "No previous invocation found to recall!" >&2
                exit 1
            fi

            source "${memory_file}"
            run_previous && exit 0
            ;;
        b) build_args="${OPTARG}" ;;
        j) jobs="-j ${OPTARG}" ;;
        r) recipe="${source_dir}/recipe/${OPTARG}.cmake" ;;
        e)
            if [[ ${#included_ingredients[@]} -ne 0 ]]; then
                yell_include_exclude_mutually_exclusive
                exit 1
            fi

            excluded_ingredients+=(${OPTARG})
            ;;
        i)
            if [[ ${#excluded_ingredients[@]} -ne 0 ]]; then
                yell_include_exclude_mutually_exclusive
                exit 1
            fi

            included_ingredients+=(${OPTARG})
            ;;
        d) build_dir=$(realpath "${OPTARG}") ;;
        p) ingredients_dir=$(realpath "${OPTARG}") ;;
        t) build_type=${OPTARG} ;;
        g) generator=${OPTARG} ;;
        s) parse_assignment "${OPTARG}" ;;
        f) export_dir=$(realpath "${OPTARG}") ;;
        l) list_only="1" ;;
        h) usage; exit 0 ;;
        x) nested="1" ;;
        *) usage; exit 1 ;;
    esac
done

shift $((OPTIND - 1))

if [ -n "${jobs}" ] && [[ "${generator}" == *"Makefiles" ]]; then
    yell_parallel_make_not_supported;
    exit 1
fi

cooking_dir="${build_dir}/_cooking"
cache_file="${build_dir}/CMakeCache.txt"
ingredients_ready_file="${cooking_dir}/ready.txt"

if [ -z "${ingredients_dir}" ]; then
    ingredients_dir="${cooking_dir}/installed"
fi

mkdir -p "${build_dir}"

cat <<'EOF' > "${build_dir}/Cooking.cmake"
#
# This file was generated by cmake-cooking v0.11.0
# The home of cmake-cooking is https://github.com/hakuch/CMakeCooking
#

### BEGIN GENERATED FILE
#
# Copyright 2018 Jesse Haber-Kucharsky
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

macro (project name)
  set (_cooking_dir ${CMAKE_CURRENT_BINARY_DIR}/_cooking)

  if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set (_cooking_root ON)
  else ()
    set (_cooking_root OFF)
  endif ()

  find_program (Cooking_STOW_EXECUTABLE
    stow
    "Executable path of GNU Stow.")

  if (NOT Cooking_STOW_EXECUTABLE)
    message (FATAL_ERROR "Cooking: GNU Stow is required!")
  endif ()

  set (Cooking_INGREDIENTS_DIR
    ${_cooking_dir}/installed
    CACHE
    PATH
    "Directory where ingredients will be installed.")

  set (Cooking_EXCLUDED_INGREDIENTS
   ""
   CACHE
   STRING
   "Semicolon-separated list of ingredients that are not provided by Cooking.")

  set (Cooking_EXTERNAL_PROJECTS
    ""
    CACHE
    STRING
    ""
    FORCE)

  set (Cooking_INCLUDED_INGREDIENTS
   ""
   CACHE
   STRING
   "Semicolon-separated list of ingredients that are provided by Cooking.")

  option (Cooking_LIST_ONLY
    "Available ingredients will be listed and nothing will be installed."
    OFF)

  set (Cooking_RECIPE "" CACHE STRING "Configure ${name}'s dependencies according to the named recipe.")

  if ((NOT DEFINED Cooking_EXCLUDED_INGREDIENTS) OR (Cooking_EXCLUDED_INGREDIENTS STREQUAL ""))
    set (_cooking_is_excluding OFF)
  else ()
    set (_cooking_is_excluding ON)
  endif ()

  if ((NOT DEFINED Cooking_INCLUDED_INGREDIENTS) OR (Cooking_INCLUDED_INGREDIENTS STREQUAL ""))
    set (_cooking_is_including OFF)
  else ()
    set (_cooking_is_including ON)
  endif ()

  if (_cooking_is_excluding AND _cooking_is_including)
    message (
      FATAL_ERROR
      "Cooking: The EXCLUDED_INGREDIENTS and INCLUDED_INGREDIENTS lists are mutually exclusive options!")
  endif ()

  if (_cooking_root)
    _project (${name} ${ARGN})

    if (NOT ("${Cooking_RECIPE}" STREQUAL ""))
      set (_cooking_ready_marker_file ${_cooking_dir}/ready.txt)
      set (_cooking_recook_marker_file ${Cooking_INGREDIENTS_DIR}/.cooking_recook_ingredient_${name})

      add_custom_command (
        OUTPUT ${_cooking_recook_marker_file}
        COMMAND ${CMAKE_COMMAND} -E touch ${_cooking_recook_marker_file})

      add_custom_target (_cooking_recooked
        ALL
        DEPENDS ${_cooking_recook_marker_file})

      list (APPEND CMAKE_PREFIX_PATH ${Cooking_INGREDIENTS_DIR})
      include (ExternalProject)
      include (${Cooking_RECIPE})

      if (Cooking_LIST_ONLY)
        foreach (d ${Cooking_EXTERNAL_PROJECTS})
          set_target_properties (${d}
            PROPERTIES
              EXCLUDE_FROM_ALL ON)
        endforeach ()

        set (top_depends "")
        set (result_prefix "_cooking_")
        set (result_step "_listed")
      else ()
        set (top_depends DEPENDS ${Cooking_EXTERNAL_PROJECTS})
        set (result_prefix "")
        set (result_step "-cooking-stow")
      endif ()

      string (REGEX REPLACE
        "ingredient_([^ ;]+)"
        "${result_prefix}ingredient_\\1${result_step}"
        result_targets
        "${Cooking_EXTERNAL_PROJECTS}")

      ExternalProject_add (cooking_ingredients
        ${top_depends}
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND "")

      ExternalProject_add_step (cooking_ingredients
        ready
        DEPENDEES install
        DEPENDS ${_cooking_recook_marker_file}
        COMMAND ${CMAKE_COMMAND} -E touch ${_cooking_ready_marker_file})

      ExternalProject_add_steptargets (cooking_ingredients
        ready)

      ExternalProject_add_stepdependencies (cooking_ingredients
        ready
        # Regenerate ${cooking_ready_marker_file} every time we run `cooking.sh`.
        _cooking_recooked
        ${result_targets})

      if (NOT EXISTS ${_cooking_ready_marker_file})
        return ()
      endif ()
    endif ()
  endif ()
endmacro ()

### Calulate the union of two sets.
###
### Input:
###    x, y: Names of variables containing ;-separated lists.
###
### Ouput:
###   var: Name of a variable to be populated with the union of the contents of `x` and `y`.
function (_cooking_set_union x y var)
  set (r ${${x}})

  foreach (e ${${y}})
    list (APPEND r ${e})
  endforeach ()

  list (REMOVE_DUPLICATES r)
  set (${var} ${r} PARENT_SCOPE)
endfunction ()

### Calculate the difference of two sets.
###
### Input:
###   x, y: Names of variables containing ;-separated lists.
###
### Output:
###   var: Name of a variable to be populated with the contents of `x` without the contents of `y`.
function (_cooking_set_difference x y var)
  set (r ${${x}})

  foreach (e ${${y}})
    if (${e} IN_LIST ${x})
       list (REMOVE_ITEM r ${e})
    endif ()
  endforeach ()

  set (${var} ${r} PARENT_SCOPE)
endfunction ()

### Calculate the intersection of two sets.
###
### Input:
###   x, y: Names of variables containing ;-separated lists.
###
### Output:
###   var: Name of a variable to be populated with the intersection of the contents of `x` and `y`.
function (_cooking_set_intersection x y var)
  set (r "")

  foreach (e ${${y}})
    if (${e} IN_LIST ${x})
      list (APPEND r ${e})
    endif ()
  endforeach ()

  list (REMOVE_DUPLICATES r)
  set (${var} ${r} PARENT_SCOPE)
endfunction ()

### Query a list of key-value pairs by key.
###
### INPUT:
###   list:
###     The name of a variable containing a ;-separated list. Each two values in the list are interpretted as a
###     key-value pair. For example, given the list "NAME;JOE;AGE;23" the pairs are ("NAME", "JOE") and
###     ("AGE", "23").
###
###    key: The key value.
###
### OUTPUT:
###    var: The name of a variable to be populated with the corresponding value of `key` in `list`.
###
### If there is no value in the list following the index of `key`, then the result is the special string "NOTFOUND".
function (_cooking_query_by_key list key var)
  list (FIND ${list} ${key} index)

  if (${index} EQUAL "-1")
    set (value NOTFOUND)
  else ()
    math (EXPR value_index "${index} + 1")
    list (GET ${list} ${value_index} value)
  endif ()

  set (${var} ${value} PARENT_SCOPE)
endfunction ()

### Populate a ExternalProject parameter by querying EXTERNAL_PROJECT_ARGS, or use a default value.
###
### _cooking_populate_ep_parameter (
###   EXTERNAL_PROJECT_ARGS_LIST ep_args_list
###   PARAMETER p
###   DEFAULT_VALUE d)
###
### `list` is the name of the ;-separated list of ExternalProject parameters supplied in COOKING_INGREDIENT.
###
### Let `p_lower` be the value of `p` converted to lower-case.
###
### If the parameter `p` is supplied in ${ep_args_list}, then `_cooking_${p_lower}` is populated with its corresponding value in
### `list` and `_cooking_ep_${p_lower}`` is populated with the empty string.
###
### If the parameter is not in ${ep_args_list}, then `_cooking_${p_lower}` is populated with `d` and `_cooking_ep_${p_lower}` is
### populated with "${p} ${d}".
###
### For example, consider
###
###     _cooking_populate_ep_parameter (
###       EXTERNAL_PROJECT_ARGS_LIST ep_args_list
###       PARAMETER SOURCE_DIR
###       DEFAULT_VALUE src)
###
### If SOURCE_DIR is included in ${ep_args_list}, then `_cooking_source_dir` will have that value and
### `_cooking_ep_source_dir` will be empty.
###
### Otherwise, `_cooking_source_dir` will be populated with "src" and `_cooking_ep_source_dir` will be populated with
### "SOURCE_DIR src".
function (_cooking_populate_ep_parameter)
  cmake_parse_arguments (
    pa
    ""
    "EXTERNAL_PROJECT_ARGS_LIST;PARAMETER;DEFAULT_VALUE"
    ""
    ${ARGN})

  string (TOLOWER ${pa_PARAMETER} parameter_lower)
  _cooking_query_by_key (${pa_EXTERNAL_PROJECT_ARGS_LIST} ${pa_PARAMETER} ${parameter_lower})
  set (value ${${parameter_lower}})
  set (var_name _cooking_${parameter_lower})
  set (ep_var_name _cooking_ep_${parameter_lower})

  if (NOT value)
    set (${var_name} ${pa_DEFAULT_VALUE} PARENT_SCOPE)
    set (${ep_var_name} ${pa_PARAMETER} ${pa_DEFAULT_VALUE} PARENT_SCOPE)
  else ()
    set (${var_name} ${value} PARENT_SCOPE)
    set (${ep_var_name} "" PARENT_SCOPE)
  endif ()
endfunction ()

### Adjust the requirements of an ingredient based on what is included or excluded.
###
### _cooking_adjust_requirements (
###   IS_EXLUDING e
###   IS_INCLUDING i
###   REQUIREMENTS x1 x2 ... xn
###   OUTPUT_LIST list)
###
### The required ingredients of an ingredient (in REQUIREMENTS) need to be adjusted based on which ingredients are
### included or excluded (if any) from the reicpe.
###
### The adjusted requirements list is populated in the variable with the name stored in `list`.
###
### If `e` evaluates to "true", then we remove from the requirements list those ingredients which are excluded from
### this recipe (in `Cooking_EXCLUDED_INGREDIENTS`).
###
### If `i` evaluates to "true", then we only include those ingredients which are specified in
### `Cooking_INCLUDED_INGREDIENTS`.
###
### `e` and `i` must be mutually exclusive: either both evaluate to "false" or a single one must evaluate to "true".
function (_cooking_adjust_requirements)
  cmake_parse_arguments (
    pa
    ""
    "IS_EXCLUDING;IS_INCLUDING;OUTPUT_LIST"
    "REQUIREMENTS"
    ${ARGN})

  if (pa_IS_EXCLUDING)
    # Strip out any dependencies that are excluded.
    _cooking_set_difference (
      pa_REQUIREMENTS
      Cooking_EXCLUDED_INGREDIENTS
      pa_REQUIREMENTS)
  elseif (_cooking_is_including)
    # Eliminate dependencies that have not been included.
    _cooking_set_intersection (
      pa_REQUIREMENTS
      Cooking_INCLUDED_INGREDIENTS
      pa_REQUIREMENTS)
  endif ()

  set (${pa_OUTPUT_LIST} ${pa_REQUIREMENTS} PARENT_SCOPE)
endfunction ()

### Populate the value of the DEPENDS parameter for EXTERNALPROJECT_ADD.
###
### _cooking_populate_ep_depends (
###   REQUIREMENTS x1 x2 ... xn)
###
### `REQUIREMENTS` is assumed to be the adjusted (see _COOKING_ADJUST_REQUIREMENTS) set of requirements of an
### ingredient.
###
### This function populates, if REQUIREMENTS is non-empty,  `_cooking_ep_depends` with
### "DEPENDS ingredient_x1 ingredient_x2 ... ingredient_xn". If REQUIREMENTS is empty,
### the result is the empty string.
function (_cooking_populate_ep_depends)
  cmake_parse_arguments (
    pa
    ""
    ""
    "REQUIREMENTS"
    ${ARGN})

  if (pa_REQUIREMENTS)
    set (value DEPENDS)

    foreach (d ${pa_REQUIREMENTS})
      list (APPEND value ingredient_${d})
    endforeach ()
  else ()
    set (value "")
  endif ()

  set (_cooking_ep_depends ${value} PARENT_SCOPE)
endfunction ()

### Construct command-line arguments for `cooking.sh` related to restrictions.
###
### _cooking_prepare_restrictions_arguments (
###   IS_EXCLUDING e
###   IS_INCLUDING i
###   REQUIREMENTS x1 x2 .. xn
###   OUTPUT_LIST list)
###
### Following similar logical same rules as _COOKING_ADJUST_REQUIREMENTS, this function populates in a variable with
### the name stored in `list` a ;-separated list of command-line arguments to be included in the invocation of
### `cooking.sh` for ingredients with their own recipe.
function (_cooking_prepare_restrictions_arguments)
  cmake_parse_arguments (
    pa
    ""
    "IS_EXCLUDING;IS_INCLUDING;OUTPUT_LIST"
    "REQUIREMENTS"
    ${ARGN})

  set (args "")

  if (pa_IS_INCLUDING)
    _cooking_set_difference (
      Cooking_INCLUDED_INGREDIENTS
      pa_REQUIREMENTS
      included)

    foreach (x ${included})
      list (APPEND args -i ${x})
    endforeach ()
  elseif (pa_IS_EXCLUDING)
    _cooking_set_union (
      Cooking_EXCLUDED_INGREDIENTS
      pa_REQUIREMENTS
      excluded)

    foreach (x ${excluded})
      list (APPEND args -e ${x})
    endforeach ()
  else ()
    foreach (x ${pa_REQUIREMENTS})
      list (APPEND args -e ${x})
    endforeach ()
  endif ()

  set (${pa_OUTPUT_LIST} ${args} PARENT_SCOPE)
endfunction ()

### Construct a set of command-line arguments to `cmake` that are common to all ingredients.
###
### OUTPUT:
###   output: Name of a variable to be populated with the ;-separated list of arguments.
function (_cooking_determine_common_cmake_args output)
  string (REPLACE ";" ":::" prefix_path_with_colons "${CMAKE_PREFIX_PATH}")

  set (${output}
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DCMAKE_PREFIX_PATH=${prefix_path_with_colons}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    PARENT_SCOPE)
endfunction ()

### Populate the value of the CONFIGURE_COMMAND parameter for EXTERNALPROJECT_ADD.
###
### _cooking_populate_ep_configure_command (
###   IS_EXCLUDING e
###   IS_INCLUDING i
###   [RECIPE r]
###   EXTERNAL_PROJECT_ARGS_LIST ep_args_list
###   REQUIREMENTS x1 x2 ... xn
###   COOKING_CMAKE_ARGS c1 c2 ... cn)
###
### This function populates `_cooking_ep_configure_command`, which can be forwarded to EXTERNALPROJECT_ADD.
###
### `e` and `i` must either both evaluate to "false" or be mutually exclusive.
###
### `REQUIREMENTS` is assumed to be the adjusted (see _COOKING_ADJUST_REQUIREMENTS) set of requirements of an
### ingredient.
###
### If a RECIPE is provided, then configuring an ingredient requires running `cooking.sh` with the (undocumented)
### `-x` argument and with the list of `COOKING_CMAKE_ARGS`.
###
### Otherwise, the result is empty.
function (_cooking_populate_ep_configure_command)
  cmake_parse_arguments (
    pa
    ""
    "IS_EXCLUDING;IS_INCLUDING;RECIPE;EXTERNAL_PROJECT_ARGS_LIST"
    "REQUIREMENTS;COOKING_CMAKE_ARGS"
    ${ARGN})

  if (pa_RECIPE)
    if (pa_RECIPE STREQUAL <DEFAULT>)
      set (recipe_args "")
    else ()
      set (recipe_args -r ${pa_RECIPE})
    endif ()

    _cooking_prepare_restrictions_arguments (
      IS_EXCLUDING ${pa_IS_EXCLUDING}
      IS_INCLUDING ${pa_IS_INCLUDING}
      REQUIREMENTS ${pa_REQUIREMENTS}
      OUTPUT_LIST restrictions_args)

    set (value
      CONFIGURE_COMMAND
      <SOURCE_DIR><SOURCE_SUBDIR>/cooking.sh
      ${recipe_args}
      -d <BINARY_DIR>
      -p ${Cooking_INGREDIENTS_DIR}
      -g ${CMAKE_GENERATOR}
      -x
      ${restrictions_args}
      --
      ${pa_COOKING_CMAKE_ARGS})
  else ()
    set (value "")
  endif ()

  set (_cooking_ep_configure_command ${value} PARENT_SCOPE)
endfunction ()

### Populate the value of the BUILD_COMMAND parameter for EXTERNALPROJECT_ADD.
###
### INPUT:
###   ep_args_list: The name of a ;-separated list of arguments to EXTERNALPROJECT_ADD.
###
### This function populates `_cooking_ep_build_command`.
###
### If BUILD_COMMAND is present in the ExternalProject argument list, the result is an empty string.
###
### Otherwise, the resulting command invokes `cmake` with the `--build` option.
function (_cooking_populate_ep_build_command ep_args_list)
  if (NOT (BUILD_COMMAND IN_LIST ${ep_args_list}))
    set (value BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR>)
  else ()
    set (value "")
  endif ()

  set (_cooking_ep_build_command ${value} PARENT_SCOPE)
endfunction ()

### Populate the value of the INSTALL_COMMAND parameter for EXTERNALPROJECT_ADD.
###
### INPUT:
###   ep_args_list: The name of a ;-separated list of arguments to EXTERNALPROJECT_ADD.
###
### This function populates `_cooking_ep_install_command`.
###
### If INSTALL_COMMAND is present in the ExternalProject argument list, the result is an empty string.
###
### Otherwise, the resulting command invokes `cmake` with the `--build` option and the `install` target.
function (_cooking_populate_ep_install_command ep_args_list)
  if (NOT (INSTALL_COMMAND IN_LIST ${ep_args_list}))
    set (value INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target install)
  else ()
    set (value "")
  endif ()

  set (_cooking_ep_install_command ${value} PARENT_SCOPE)
endfunction ()

### Invoke EXTERNALPROJECT_ADD to define an ingredient.
###
### _cooking_define_ep (
###   NAME name
###   [RECIPE r]
###   REQUIREMENTS ...
###   INGREDIENT_DIR ingredient_dir
###   STOW_DIR stow_dir
###   LOCAL_RECONFIGURE {ON,OFF}
###   LOCAL_REBUILD {ON,OFF}
###   EXTERNAL_PROJECT_ARGS_LIST ep_args_list
###   EP_DEPENDS ...
###   EP_SOURCE_DIR ...
###   EP_SOURCE_SUBDIR ...
###   EP_BINARY_DIR ...
###   EP_CONFIGURE_COMMAND ...
###   EP_BUILD_COMMAND ...
###   EP_INSTALL_COMMAND ...)
###
### ${ep_args_list} is forwarded to EXTERNALPROJECT_ADD after all of the named arguments with the special value
### "<DISABLE>" replaced by the empty string (to invoke the default behavior from the ExternalProject module).
function (_cooking_define_ep)
  cmake_parse_arguments (
    pa
    ""
    "NAME;EXTERNAL_PROJECT_ARGS_LIST;RECIPE;INGREDIENT_DIR;STOW_DIR;LOCAL_RECONFIGURE;LOCAL_REBUILD"
    "REQUIREMENTS;EP_DEPENDS;EP_SOURCE_DIR;EP_SOURCE_SUBDIR;EP_BINARY_DIR;EP_CONFIGURE_COMMAND;EP_BUILD_COMMAND;EP_INSTALL_COMMAND;CMAKE_ARGS"
    ${ARGN})

  string (REPLACE "<DISABLE>" "" forwarded_ep_args "${${pa_EXTERNAL_PROJECT_ARGS_LIST}}")
  set (ep_name ingredient_${pa_NAME})
  set (stamp_dir ${pa_INGREDIENT_DIR}/stamp)

  # Useful for debugging.
  set (all_ep_args
    ${pa_EP_DEPENDS}
    ${pa_EP_SOURCE_DIR}
    ${pa_EP_SOURCE_SUBDIR}
    ${pa_EP_BINARY_DIR}
    ${pa_EP_CONFIGURE_COMMAND}
    ${pa_EP_BUILD_COMMAND}
    ${pa_EP_INSTALL_COMMAND}
    PREFIX ${pa_INGREDIENT_DIR}
    STAMP_DIR ${stamp_dir}
    INSTALL_DIR ${pa_STOW_DIR}/${pa_NAME}
    CMAKE_ARGS ${pa_CMAKE_ARGS}
    LIST_SEPARATOR :::
    INDEPENDENT_STEP_TARGETS download
    "${forwarded_ep_args}")

  ExternalProject_add (${ep_name}
    "${all_ep_args}")

  set (stow_marker_file ${Cooking_INGREDIENTS_DIR}/.cooking_ingredient_${pa_NAME})
  set (lock_file ${Cooking_INGREDIENTS_DIR}/.cooking_stow.lock)

  if (Cooking_LIST_ONLY)
    ##
    ## Listing target.
    ##

    set (listing_commands COMMAND ${CMAKE_COMMAND} -E touch ${stow_marker_file})

    if (pa_RECIPE)
      set (step_dependencies ${ep_name}-download)

      if (pa_RECIPE STREQUAL <DEFAULT>)
        set (recipe_args "")
      else ()
        set (recipe_args -r ${pa_RECIPE})
      endif ()

      ExternalProject_get_property (${ep_name} SOURCE_DIR)
      ExternalProject_get_property (${ep_name} SOURCE_SUBDIR)

      list (INSERT listing_commands 0
        COMMAND
        ${SOURCE_DIR}${SOURCE_SUBDIR}/cooking.sh
        ${recipe_args}
        -p ${Cooking_INGREDIENTS_DIR}
        -g ${CMAKE_GENERATOR}
        -x
        -l)
    else ()
      set (step_dependencies "")
    endif ()

    add_custom_command (
      OUTPUT ${stow_marker_file}
      DEPENDS
        ${step_dependencies}
        ${_cooking_recooked}
        ${_cooking_recook_marker_file}
      ${listing_commands})

    add_custom_target (_cooking_${ep_name}_listed
      DEPENDS ${stow_marker_file})

    foreach (d ${pa_REQUIREMENTS})
      add_dependencies (_cooking_${ep_name}_listed
        _cooking_ingredient_${d}_listed)
    endforeach ()
  else ()
    ##
    ## Stowing step.
    ##

    ExternalProject_add_step (${ep_name}
      cooking-stow
      EXCLUDE_FROM_MAIN yes
      DEPENDEES install
      DEPENDS ${_cooking_recook_marker_file}
      COMMAND
        flock
        --wait 30
        ${lock_file}
        ${Cooking_STOW_EXECUTABLE}
        -t ${Cooking_INGREDIENTS_DIR}
        -d ${pa_STOW_DIR}
        ${pa_NAME}
      COMMAND ${CMAKE_COMMAND} -E touch ${stow_marker_file})

    ExternalProject_add_steptargets (${ep_name}
      cooking-stow)

    # Re-stow every ingredient whenever `cooking.sh` is run.
    ExternalProject_add_stepdependencies (${ep_name}
      cooking-stow
      _cooking_recooked)

    foreach (d ${pa_REQUIREMENTS})
      ExternalProject_add_stepdependencies (${ep_name}
        configure
        ingredient_${d}-cooking-stow)
    endforeach ()

    if (pa_RECIPE)
      ##
      ## Ingredients with their own recipe need to be reconfigured every time we run `cooking.sh`.
      ##

      ExternalProject_add_step (${ep_name}
        cooking-reconfigure
        DEPENDERS configure
        DEPENDS ${_cooking_recook_marker_file}
        COMMAND ${CMAKE_COMMAND} -E echo_append)

      ExternalProject_add_stepdependencies (${ep_name}
        cooking-reconfigure
        _cooking_recooked)
    endif ()
  endif ()

  list (APPEND Cooking_EXTERNAL_PROJECTS ${ep_name})

  set (Cooking_EXTERNAL_PROJECTS
    "${Cooking_EXTERNAL_PROJECTS}"
    CACHE
    STRING
    ""
    FORCE)

  if (pa_LOCAL_RECONFIGURE OR pa_LOCAL_REBUILD)
    if (pa_LOCAL_RECONFIGURE)
      set (step configure)
    else ()
      set (step build)
    endif ()

    ExternalProject_add_step (${ep_name}
      cooking-local-${step}
      DEPENDERS ${step}
      DEPENDS ${_cooking_recook_marker_file}
      COMMAND ${CMAKE_COMMAND} -E echo_append)

    ExternalProject_add_stepdependencies (${ep_name}
      cooking-local-${step}
      _cooking_recooked)
  endif ()
endfunction ()

macro (cooking_ingredient name)
  set (_cooking_args "${ARGN}")

  if ((_cooking_is_excluding AND (${name} IN_LIST Cooking_EXCLUDED_INGREDIENTS))
      OR (_cooking_is_including AND (NOT (${name} IN_LIST Cooking_INCLUDED_INGREDIENTS))))
    # Nothing.
  else ()
    set (_cooking_ingredient_dir ${_cooking_dir}/ingredient/${name})

    cmake_parse_arguments (
      _cooking_pa
      "LOCAL_RECONFIGURE;LOCAL_REBUILD"
      "COOKING_RECIPE"
      "COOKING_CMAKE_ARGS;EXTERNAL_PROJECT_ARGS;REQUIRES"
      ${_cooking_args})

    _cooking_populate_ep_parameter (
      EXTERNAL_PROJECT_ARGS_LIST _cooking_pa_EXTERNAL_PROJECT_ARGS
      PARAMETER SOURCE_DIR
      DEFAULT_VALUE ${_cooking_ingredient_dir}/src)

    _cooking_populate_ep_parameter (
      EXTERNAL_PROJECT_ARGS_LIST _cooking_pa_EXTERNAL_PROJECT_ARGS
      PARAMETER SOURCE_SUBDIR
      DEFAULT_VALUE ".")

    _cooking_populate_ep_parameter (
      EXTERNAL_PROJECT_ARGS_LIST _cooking_pa_EXTERNAL_PROJECT_ARGS
      PARAMETER BINARY_DIR
      DEFAULT_VALUE ${_cooking_ingredient_dir}/build)

    _cooking_populate_ep_parameter (
      EXTERNAL_PROJECT_ARGS_LIST _cooking_pa_EXTERNAL_PROJECT_ARGS
      PARAMETER BUILD_IN_SOURCE
      DEFAULT_VALUE OFF)

    if (_cooking_build_in_source)
       set (_cooking_ep_binary_dir "")
    endif ()

    _cooking_adjust_requirements (
      IS_EXCLUDING ${_cooking_is_excluding}
      IS_INCLUDING ${_cooking_is_including}
      REQUIREMENTS ${_cooking_pa_REQUIRES}
      OUTPUT_LIST _cooking_pa_REQUIRES)

    _cooking_populate_ep_depends (
      REQUIREMENTS ${_cooking_pa_REQUIRES})

    _cooking_determine_common_cmake_args (_cooking_common_cmake_args)

    _cooking_populate_ep_configure_command (
      IS_EXCLUDING ${_cooking_is_excluding}
      IS_INCLUDING ${_cooking_is_including}
      RECIPE ${_cooking_pa_COOKING_RECIPE}
      REQUIREMENTS ${_cooking_pa_REQUIRES}
      EXTERNAL_PROJECT_ARGS_LIST _cooking_pa_EXTERNAL_PROJECT_ARGS
      COOKING_CMAKE_ARGS
        ${_cooking_common_cmake_args}
        ${_cooking_pa_COOKING_CMAKE_ARGS})

    _cooking_populate_ep_build_command (_cooking_pa_EXTERNAL_PROJECT_ARGS)
    _cooking_populate_ep_install_command (_cooking_pa_EXTERNAL_PROJECT_ARGS)

    _cooking_define_ep (
      NAME ${name}
      RECIPE ${_cooking_pa_COOKING_RECIPE}
      REQUIREMENTS ${_cooking_pa_REQUIRES}
      EP_DEPENDS ${_cooking_ep_depends}
      EP_SOURCE_DIR ${_cooking_ep_source_dir}
      EP_SOURCE_SUBDIR ${_cooking_ep_source_subdir}
      EP_BINARY_DIR ${_cooking_ep_binary_dir}
      EP_CONFIGURE_COMMAND ${_cooking_ep_configure_command}
      EP_BUILD_COMMAND ${_cooking_ep_build_command}
      EP_INSTALL_COMMAND ${_cooking_ep_install_command}
      INGREDIENT_DIR ${_cooking_ingredient_dir}
      STOW_DIR ${_cooking_dir}/stow
      CMAKE_ARGS ${_cooking_common_cmake_args}
      EXTERNAL_PROJECT_ARGS_LIST _cooking_pa_EXTERNAL_PROJECT_ARGS
      LOCAL_RECONFIGURE ${_cooking_pa_LOCAL_RECONFIGURE}
      LOCAL_REBUILD ${_cooking_pa_LOCAL_REBUILD})
  endif ()
endmacro ()
### END GENERATED FILE
EOF

cmake_cooking_args=(
    "-DCooking_INGREDIENTS_DIR=${ingredients_dir}"
    "-DCooking_RECIPE=${recipe}"
)

#
# Remove any `Cooking.cmake` file from the source directory. We now generate this file in the build directory, and old
# copies will cause conflicts.
#

old_cooking_file="${source_dir}/cmake/Cooking.cmake"

if [ -f "${old_cooking_file}" ]; then
    grep 'This file was generated by cmake-cooking' "${old_cooking_file}" > /dev/null && rm "${old_cooking_file}"
fi

#
# Clean-up from a previous run.
#

if [ -e "${ingredients_ready_file}" ]; then
    rm "${ingredients_ready_file}"
fi

if [ -e "${cache_file}" ]; then
    rm "${cache_file}"
fi

if [ -d "${ingredients_dir}" -a -z "${nested}" ]; then
    rm -r --preserve-root "${ingredients_dir}"
fi

mkdir -p "${ingredients_dir}"

#
# Validate recipe.
#

if [ -n "${recipe}" ]; then
    if [ ! -f "${recipe}" ]; then
        echo "Cooking: The '${recipe}' recipe file does not exist!" >&2
        exit 1
    fi
fi

#
# Prepare lists of included and excluded ingredients.
#

if [ -n "${excluded_ingredients}" ] && [ -z "${list_only}" ]; then
    cmake_cooking_args+=(
        -DCooking_EXCLUDED_INGREDIENTS=$(printf "%s;" "${excluded_ingredients[@]}")
        -DCooking_INCLUDED_INGREDIENTS=
    )
fi

if [ -n "${included_ingredients}" ] && [ -z "${list_only}" ]; then
    cmake_cooking_args+=(
        -DCooking_EXCLUDED_INGREDIENTS=
        -DCooking_INCLUDED_INGREDIENTS=$(printf "%s;" "${included_ingredients[@]}")
    )
fi

#
# Configure and build ingredients.
#

mkdir -p "${cooking_dir}"/stow
touch "${cooking_dir}"/stow/.stow
cd "${build_dir}"

if [ -n "${list_only}" ]; then
    cmake_cooking_args+=("-DCooking_LIST_ONLY=ON")
fi

${CMAKE} -DCMAKE_BUILD_TYPE="${build_type}" "${cmake_cooking_args[@]}" -G "${generator}" "${source_dir}" "${@}"
${CMAKE} --build . --target cooking_ingredients-ready ${jobs} -- ${build_args[@]}

#
# Report what we've done (if we're not nested).
#

if [ -z "${nested}" ]; then
    ingredients=($(find "${ingredients_dir}" -name '.cooking_ingredient_*' -printf '%f\n' | sed -r 's/\.cooking_ingredient_(.+)/\1/'))

    if [ -z "${list_only}" ]; then
        printf "\nCooking: Installed the following ingredients:\n"
    else
        printf "\nCooking: The following ingredients are necessary for this recipe:\n"
    fi

    for ingredient in "${ingredients[@]}"; do
        echo "  - ${ingredient}"
    done

    printf '\n'
fi

if [ -n "${list_only}" ]; then
    exit 0
fi

#
# Configure the project, expecting all requirements satisfied.
#

${CMAKE} -DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=ON "${@}" .

#
# Optionally export the installed files.
#

if [ -n "${export_dir}" ]; then
    rsync "${ingredients_dir}/" "${export_dir}" -a --copy-links
    printf "\nCooking: Exported ingredients to ${export_dir}\n"
fi

#
# Save invocation information.
#

cd "${initial_wd}"

cat <<EOF > "${memory_file}"
run_previous() {
    "${0}" ${invoked_args[@]@Q}
}
EOF