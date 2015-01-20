# This file is included by ../CMakeLists.txt so path are relative to it.

#######################################################################
####################Library path finding related tools#################
#######################################################################
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${SAC_SOURCE_DIR}/cmake/modules")

#ensure that a lib is installed and link it to "target" executable
function (check_and_link_libs target)
   #you shouldn't touch that
    foreach(lib ${ARGN})
        string(TOUPPER ${lib} LIB) #in uppercase

        find_package (${lib})
        if (${LIB}_FOUND)
          include_directories(${${LIB}_INCLUDE_DIR})
          target_link_libraries (${target} ${${LIB}_LIBRARY})
        else()
            message(SEND_ERROR "You NEED ${lib} library.")
        endif ()
    endforeach()
endfunction()

#######################################################################
####################File autogeneration################################
#######################################################################
# Header/Source file autogenerated by CMake should stand in
# ${CMAKE_BINARY_DIR}/gen directory. Use these functions to do so.

# This function returns the tmp file associated for the expected filename
function (auto_generate_filename filepath outputtmpfile)
    set(${outputtmpfile} ${CMAKE_BINARY_DIR}/gen/${filepath}.cmake-tmp
        PARENT_SCOPE)
endfunction()

# filepath should be an absolute path, generated by auto_generate_filename
# and located in ${CMAKE_BINARY_DIR}/gen folder.
function (auto_generate_source TMP_FILE force_remove)
    string(REGEX REPLACE ".cmake-tmp$" "" FINAL_FILE ${TMP_FILE})

    if (force_remove)
        FILE(REMOVE ${FINAL_FILE})
    else()
        # only modify output FILE if a change has been made to avoid
        # recompilation just because of "last modified date" has changed.
        execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files
            ${TMP_FILE} ${FINAL_FILE}
            RESULT_VARIABLE file_are_differents
            OUTPUT_QUIET
            #ERROR_QUIET
        )
        if (file_are_differents)
            FILE(RENAME ${TMP_FILE} ${FINAL_FILE})
        endif()
    endif()
    FILE(REMOVE ${TMP_FILE})
endfunction()
# Autogenerated files must be included with '#include "gen/path_to_file.h"'
include_directories(${CMAKE_BINARY_DIR})
