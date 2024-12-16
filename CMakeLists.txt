option(UFOVIZ_BUILD_DOCS     "Generate documentation" OFF)
option(UFOVIZ_BUILD_TESTS    "Unit testing"           OFF)
option(UFOVIZ_BUILD_COVERAGE "Test Coverage"          OFF)

add_subdirectory(apps)

add_subdirectory(glfw_webgpu)

add_library(Viz
	src/load_shader_module.cpp
	src/viz.cpp
)
add_library(UFO::Viz ALIAS Viz)

set_target_properties(Viz 
	PROPERTIES
		VERSION ${PROJECT_VERSION}
		SOVERSION ${PROJECT_VERSION_MAJOR}
		CXX_STANDARD 17
		CXX_EXTENSIONS OFF
		OUTPUT_NAME "UFOViz"
)

# add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/imgui")

target_link_libraries(Viz
	PUBLIC
		UFO::Compute
		UFO::Math
		UFO::Vision
		# imgui
	PRIVATE
		glfw_webgpu
)

include(GNUInstallDirs)

target_include_directories(Viz
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
)

if(UFO_BUILD_TESTS OR UFOVIZ_BUILD_TESTS)
  add_subdirectory(tests)
endif()

if(UFO_BUILD_DOCS OR UFOVIZ_BUILD_DOCS)
	add_subdirectory(docs)
endif()

install(TARGETS Viz glfw_webgpu EXPORT Viz-targets
	COMPONENT Viz
	LIBRARY  DESTINATION lib
	ARCHIVE  DESTINATION lib
	RUNTIME  DESTINATION bin
  INCLUDES DESTINATION include
)

install(EXPORT Viz-targets
  FILE "Viz-targets.cmake"
  NAMESPACE UFO::
  DESTINATION lib/cmake/${PROJECT_NAME}
	COMPONENT Viz
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/Viz-config-version.cmake"
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/Viz-config.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/Viz-config.cmake"
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/Viz-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/Viz-config-version.cmake"
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  COMPONENT Viz
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
	COMPONENT Viz
	DESTINATION ${CMAKE_INSTALL_PREFIX}
)