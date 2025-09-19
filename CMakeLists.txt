# ============================= PROJECT SETUP =================================================
cmake_minimum_required(VERSION 3.22)
project(circus VERSION 0.1 LANGUAGES CXX)
set(LIBRARY ${PROJECT_NAME})
set(PY_LIBRARY "${PROJECT_NAME}py")
set(PROJECT circus)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_compile_definitions(PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
# =============================================================================================


# ============================ BUILD OPTIONS ==================================================
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_PYTHON_BINDINGS "Build python bindings" ON)
option(GENERATE_CLANGD "Generate .clangd in project root" ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions("DEBUG")
endif()

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
# =============================================================================================

# ============================= Dependencies ==================================================
find_package(mujoco REQUIRED)
find_package(pugixml REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
# ==============================================================================================


# ============================ CIRCUS LIBRARY ==================================================
set(CMAKE_AUTOMOC ON)
file(GLOB SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/Src/*.cpp)
list(REMOVE_ITEM SOURCES ${CMAKE_SOURCE_DIR}/Src/Bindings.cpp)
file(GLOB HEADERS CONFIGURE_DEPENDS  ${CMAKE_SOURCE_DIR}/Include/*.h)
add_library(${LIBRARY} STATIC ${SOURCES})
target_sources(${LIBRARY} PRIVATE ${HEADERS}) # Just used for IDE discovering headers

target_include_directories(${LIBRARY}
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Include>
)

set(EXTRA_LIBRARIES
  mujoco
  pugixml
  yaml-cpp
  Qt6::Widgets
  Qt6::OpenGLWidgets
  Eigen3::Eigen
  CURL::libcurl
  nlohmann_json::nlohmann_json
)

target_link_libraries(${LIBRARY} PUBLIC ${EXTRA_LIBRARIES})
# ==============================================================================================


# =============================== EXPORT =======================================================
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# 1. Install the library target and headers
set_target_properties(${LIBRARY} PROPERTIES PUBLIC_HEADER "${HEADERS}")
install(TARGETS ${LIBRARY}
    EXPORT ${LIBRARY}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

# 2. Generate the relocatable package config file in the build directory
configure_package_config_file( # creates config file for find_package
    "cmake/config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY}Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${LIBRARY}"
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
)
write_basic_package_version_file( # sets version of the package
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT}ConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

# 3. Install the package config files and the export set
install(
    EXPORT ${LIBRARY}Targets
    FILE ${LIBRARY}Targets.cmake
    NAMESPACE spqr::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${LIBRARY}"
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY}ConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${LIBRARY}
)

# ==============================================================================================

# ==================================== EXECUTABLES ==============================================
add_executable(main Src/main.cpp)
target_link_libraries(main PRIVATE ${LIBRARY} ${EXTRA_LIBRARIES})
# =============================================================================================

# ============================= PYTHON BINDINGS ================================================
find_package(Python 3.8 COMPONENTS Interpreter Development.Module REQUIRED)

execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE nanobind_ROOT
)
find_package(nanobind CONFIG REQUIRED)

include_directories(${python_INCLUDE_DIRS})
execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import sysconfig; print(sysconfig.get_path('purelib'))"
    OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

nanobind_add_module(${PY_LIBRARY} ${PROJECT_SOURCE_DIR}/Src/Bindings.cpp)
target_link_libraries(${PY_LIBRARY} PRIVATE ${LIBRARY} ${EXTRA_LIBRARIES})

install(
    TARGETS ${PY_LIBRARY}
    EXPORT ${PY_LIBRARY}Targets
    LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${BINDIR}
)
# ===============================================================================================


# ============================== CLANGD =========================================================
if(GENERATE_CLANGD)
  file(GLOB BUILD_ROOT_LIST RELATIVE "${CMAKE_SOURCE_DIR}" ".pixi/build/work/circus*/work/build")
  list(GET BUILD_ROOT_LIST 0 BUILD_ROOT_REL)
  cmake_path(APPEND BUILD_ROOT ${CMAKE_SOURCE_DIR} ${BUILD_ROOT_REL})
  file(WRITE "${CMAKE_SOURCE_DIR}/.clangd"
    "CompileFlags:\n  CompilationDatabase: ${BUILD_ROOT}\n")
endif()
# ===============================================================================================
