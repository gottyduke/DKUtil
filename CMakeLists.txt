cmake_minimum_required(VERSION 3.21)

# info
project(
	DKUtil
	VERSION 1.9.1
	LANGUAGES CXX
)

# out-of-source builds only
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
	message(
		FATAL_ERROR
			"In-source builds are not allowed."
	)
endif()

# runtime
add_library(
	${PROJECT_NAME}
	INTERFACE
)
add_library(
	${PROJECT_NAME}::${PROJECT_NAME} 
	ALIAS 
	${PROJECT_NAME}
)

target_include_directories(
	${PROJECT_NAME}
	INTERFACE
	$<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>
	$<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include/external>
)

target_compile_features(
	${PROJECT_NAME}
	INTERFACE
		cxx_std_20
)

# install
install(
	DIRECTORY
		include
	DESTINATION 
		include
)

install(
	TARGETS 
		${PROJECT_NAME}
	EXPORT 
		${PROJECT_NAME}-targets
)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.in
	${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}Config.cmake
	@ONLY
)

install(
	FILES 
		${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
	DESTINATION 
		${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

install(
	EXPORT 
		${PROJECT_NAME}-targets
	DESTINATION 
		${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
	NAMESPACE 
		${PROJECT_NAME}::
)
