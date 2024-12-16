add_executable(ufoviz 
	src/ufoviz.cpp
)

set_target_properties(ufoviz 
	PROPERTIES
		VERSION ${PROJECT_VERSION}
		SOVERSION ${PROJECT_VERSION_MAJOR}
		CXX_STANDARD 17
		CXX_EXTENSIONS OFF
)

target_link_libraries(ufoviz 
	PRIVATE
		UFO::Viz
)

include(GNUInstallDirs)

install(TARGETS ufoviz EXPORT ufoviz-targets
	LIBRARY  DESTINATION lib
	ARCHIVE  DESTINATION lib
	RUNTIME  DESTINATION bin
  INCLUDES DESTINATION include
)

install(EXPORT ufoviz-targets
  FILE "ufoviz-targets.cmake"
  NAMESPACE UFO::
  DESTINATION lib/cmake/${PROJECT_NAME}
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/ufoviz-config-version.cmake"
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/ufoviz-config.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/ufoviz-config.cmake"
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ufoviz-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ufoviz-config-version.cmake"
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)