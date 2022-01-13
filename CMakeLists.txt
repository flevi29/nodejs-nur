cmake_minimum_required(VERSION 3.20)
project(nur-typescript)
set(CMAKE_CXX_STANDARD 20)

#add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG>")

set(sources "${PROJECT_SOURCE_DIR}/src/addon/NUR")
set(headers "${PROJECT_SOURCE_DIR}/src/addon/include")
set(libdll "${PROJECT_SOURCE_DIR}/NUR-lib")

include_directories(
        ${CMAKE_JS_INC}
        ${headers}
        "./NUR-includes"
        # Required for some IDE(s) to recognize node_api.h location
        "C:/$ENV{homepath}/.cmake-js/electron-x64/v16.0.5/include/node"
)
link_directories("${libdll}")

file(GLOB SOURCE_FILES
        "${sources}/index.cpp"
        "${sources}/napi-functions.cpp"
        "${sources}/nur-main.cpp"
        "${sources}/nur-wrapper-util.cpp"
        )
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} "${libdll}/x64/NURAPI.lib")

# Include node-addon-api wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
        )
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})

add_definitions(-DNAPI_VERSION=8)
add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)
