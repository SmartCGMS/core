CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

# Add "factory" project - this is a mandatory core module

ADD_SUBDIRECTORY("factory")

# Add the rest of core modules

SUBDIRLIST(CORE_LIB_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
FOREACH(subdir ${CORE_LIB_DIRS})
	# ignore git index directory and factory directory
	IF(NOT "${subdir}" STREQUAL ".git" AND NOT "${subdir}" STREQUAL "factory")
		IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/CMakeLists.txt")
			MESSAGE(STATUS "Configuring core module: ${subdir}")
			ADD_SUBDIRECTORY(${subdir})
		ELSE()
			MESSAGE(WARNING "Core module '${subdir}' does not contain CMakeLists.txt file")
		ENDIF()
	ENDIF()
ENDFOREACH()
