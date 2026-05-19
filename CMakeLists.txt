cmake_minimum_required(VERSION 3.16)
project(ocu_sdk_example CXX)

set(CMAKE_CXX_STANDARD 17)

# Example source
add_executable(ocu_example example/main_with_sdk.cpp)

# Include paths
target_include_directories(ocu_example PRIVATE
    include/   # CoreSDK.h
    deps/      # httplib.h, json.hpp, opqr/
    example/   # AuthServer.h, Routes.hpp, login.h, whitelist.h
)

# Link CoreSDK
if(WIN32)
    target_link_libraries(ocu_example PRIVATE
        ${CMAKE_SOURCE_DIR}/bin/windows/coresdk.lib)
    # Copy DLL next to the executable
    add_custom_command(TARGET ocu_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/bin/windows/coresdk.dll
            $<TARGET_FILE_DIR:ocu_example>/coresdk.dll
    )
else()
    target_link_libraries(ocu_example PRIVATE
        ${CMAKE_SOURCE_DIR}/bin/linux-arm64/libcoresdk.so)
endif()

# Copy test.cfg next to the executable (only if it exists)
if(EXISTS ${CMAKE_SOURCE_DIR}/test.cfg)
    add_custom_command(TARGET ocu_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/test.cfg
            $<TARGET_FILE_DIR:ocu_example>/test.cfg
    )
else()
    message(WARNING "test.cfg not found — copy test.cfg.example to test.cfg before running")
endif()
# Copy certs folder next to the executable
add_custom_command(TARGET ocu_example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/example/certs
        $<TARGET_FILE_DIR:ocu_example>/certs
)