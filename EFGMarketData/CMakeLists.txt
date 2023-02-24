cmake_minimum_required(VERSION 3.16.3)

project(MarketData VERSION 0.1)

set(EXECUTABLE ${PROJECT_NAME}-bin)

# GNU Defaults
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(DEFAULT_BUILD_TYPE "Release")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set compiler flags
set(GCC_COVERAGE_COMPILE_FLAGS "-pipe -std=c++17 -mcmodel=medium")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "${CMAKE_CXX_FLAGS} -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)



# Include src
file(GLOB_RECURSE ${PROJECT_NAME}_APP_SRC CONFIGURE_DEPENDS "Application/*.cpp")
file(GLOB_RECURSE ${PROJECT_NAME}_CLIENT_SRC CONFIGURE_DEPENDS "Client/*.cpp")
file(GLOB_RECURSE ${PROJECT_NAME}_CONFIG_SRC CONFIGURE_DEPENDS "Config/*.cpp")
file(GLOB_RECURSE ${PROJECT_NAME}_CONFIGMANAGER_SRC CONFIGURE_DEPENDS "ConfigManager/*.cpp")
file(GLOB_RECURSE ${PROJECT_NAME}_VENUES_SRC CONFIGURE_DEPENDS "VenueSession/*.cpp")
file(GLOB_RECURSE ${PROJECT_NAME}_UTILS_SRC CONFIGURE_DEPENDS "Utils/*.cpp")

set(${PROJECT_NAME}_SRC 
    ${${PROJECT_NAME}_APP_SRC} ${${PROJECT_NAME}_CONFIG_SRC} 
    ${${PROJECT_NAME}_CONFIGMANAGER_SRC} ${${PROJECT_NAME}_CLIENT_SRC} 
    ${${PROJECT_NAME}_VENUES_SRC} ${${PROJECT_NAME}_UTILS_SRC}
   )
# file(GLOB_RECURSE ${PROJECT_NAME}_INC CONFIGURE_DEPENDS "*.h")
add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}_SRC})
# add_executable(${EXECUTABLE} ${${PROJECT_NAME}_SRC} csvMDRecorder.cpp)
add_executable(${EXECUTABLE} ${${PROJECT_NAME}_SRC} main.cpp)

# Include dirs
execute_process(COMMAND find ${CMAKE_CURRENT_SOURCE_DIR} -type d -name "include" ! -path ${CMAKE_CURRENT_SOURCE_DIR}/lib/* -printf "%p " OUTPUT_VARIABLE INCLUDE_DIRS OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(INCLUDE_DIRS UNIX_COMMAND ${INCLUDE_DIRS})

foreach(INCLUDE_DIR ${INCLUDE_DIRS})
  message(STATUS "Including Directory: " ${INCLUDE_DIR}/)
  target_include_directories(${PROJECT_NAME} PUBLIC "$<BUILD_INTERFACE:${INCLUDE_DIR}/>")
  target_include_directories(${EXECUTABLE} PRIVATE ${INCLUDE_DIR})
endforeach()
target_include_directories(${PROJECT_NAME} PUBLIC "$<INSTALL_INTERFACE:${PROJECT_NAME}/${CMAKE_INSTALL_INCLUDE_DIR}>")

# target_include_directories(${PROJECT_NAME} PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIRS}>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

# Link libraries
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/EFGCore")
find_package(EFGCore REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE EFGCore)
target_link_libraries(${EXECUTABLE} PRIVATE EFGCore)

find_package(OpenSSL REQUIRED)
target_link_libraries(${EXECUTABLE} PRIVATE OpenSSL::SSL OpenSSL::Crypto)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
target_link_libraries(${EXECUTABLE} PRIVATE Threads::Threads)

find_package(Armadillo REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${ARMADILLO_INCLUDE_DIRS})
target_link_Libraries(${PROJECT_NAME} PRIVATE ${ARMADILLO_LIBRARIES})
target_include_directories(${EXECUTABLE} PRIVATE ${ARMADILLO_INCLUDE_DIRS})
target_link_Libraries(${EXECUTABLE} PRIVATE ${ARMADILLO_LIBRARIES})


find_library(LIBRT rt REQUIRED)
target_link_Libraries(${PROJECT_NAME} PRIVATE ${LIBRT})
target_link_Libraries(${EXECUTABLE} PRIVATE ${LIBRT})

find_package(Boost 1.55 REQUIRED serialization)
target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES})
target_link_libraries(${EXECUTABLE} PRIVATE ${Boost_LIBRARIES})
target_include_directories(${EXECUTABLE} PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE uv)
target_link_libraries(${EXECUTABLE} PRIVATE uv)

target_link_libraries(${PROJECT_NAME} PRIVATE uWS)
target_include_directories(${PROJECT_NAME} PRIVATE "/usr/include/uWS")
target_link_libraries(${EXECUTABLE} PRIVATE uWS)
target_include_directories(${EXECUTABLE} PRIVATE "/usr/include/uWS")


find_package(ZLIB REQUIRED)
target_include_directories(${EXECUTABLE} PRIVATE ${ZLIB_INCLUDE_DIRS})
target_link_Libraries(${EXECUTABLE} PRIVATE ${ZLIB_LIBRARIES})

# Install
install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}_TARGETS
        LIBRARY DESTINATION ${PROJECT_NAME}/${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${PROJECT_NAME}/${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${PROJECT_NAME}/${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${PROJECT_NAME}/${CMAKE_INSTALL_INCLUDEDIR}
)

foreach(INCLUDE_DIR ${INCLUDE_DIRS})
    install(DIRECTORY ${INCLUDE_DIR}/ DESTINATION ${PROJECT_NAME}/${CMAKE_INSTALL_INCLUDEDIR})
endforeach()
install(EXPORT ${PROJECT_NAME}_TARGETS
        FILE ${PROJECT_NAME}_TARGETS.cmake
        DESTINATION ${PROJECT_NAME}/${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  INSTALL_DESTINATION $${PROJECT_NAME}/{CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

set(version 1.0.0)
set_property(TARGET ${PROJECT_NAME} PROPERTY VERSION ${version})
set_property(TARGET ${PROJECT_NAME} PROPERTY SOVERSION 1)
set_property(TARGET ${PROJECT_NAME} PROPERTY
  INTERFACE_${PROJECT_NAME}_MAJOR_VERSION 1)

set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY
  COMPATIBLE_INTERFACE_STRING ${PROJECT_NAME}_MAJOR_VERSION
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  VERSION "${version}"
  COMPATIBILITY AnyNewerVersion
)

install(FILES
          "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${PROJECT_NAME}/${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
