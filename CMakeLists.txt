cmake_minimum_required(VERSION 3.16)

# This source tree is injected into plugins/impex/vtf in a matching Krita checkout.
# Configure and build through Krita's own CMake project so all private headers,
# generated exports, compiler flags, and ABI settings exactly match Krita.
if(NOT COMMAND kis_add_library)
    message(FATAL_ERROR "Build this directory as plugins/impex/vtf inside a Krita source tree")
endif()

add_subdirectory(src)
