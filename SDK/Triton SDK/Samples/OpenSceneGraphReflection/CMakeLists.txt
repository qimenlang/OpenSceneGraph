cmake_minimum_required (VERSION 2.6)

if(POLICY CMP0015)
   cmake_policy(SET CMP0015 NEW)
endif(POLICY CMP0015)

set( APP_NAME osgTritonReflectionSample )

project (${APP_NAME})

if( EXISTS $ENV{OSGDIR} )
	set (OSGDIR $ENV{OSGDIR})
else()
	set (OSGDIR $ENV{OSG_DIR})	
endif()

if(NOT OSGDIR)
	message(FATAL_ERROR "OSGDIR or OSG_DIR Environment Variable not set!")
else(NOT OSGDIR)
	message(STATUS "OSGDIR environment variable is " ${OSGDIR})
endif(NOT OSGDIR)

if( EXISTS $ENV{TRITON_DIR} )
	set (TRITON_DIR $ENV{TRITON_DIR} )
elif( EXISTS $ENV{TRITON_PATH} )
	set (TRITON_DIR $ENV{TRITON_PATH} )	
else()
	set (TRITON_DIR ${CMAKE_SOURCE_DIR}/../..)
endif()

message(STATUS "TRITON_DIR environment variable is " ${TRITON_DIR})
	
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")

find_package(OpenSceneGraph REQUIRED osgDB osgUtil osgGA osgViewer osgText )
find_package(OpenGL REQUIRED)
find_package(Triton REQUIRED)

include_directories(
	${OPENSCENEGRAPH_INCLUDE_DIR}
	${OPENGL_INCLUDE_DIR}
	${TRITON_INCLUDE_DIR}
)

file(GLOB cpp_files *.cpp src/*.cpp)
file(GLOB h_files *.h src/*.h)

add_executable(${APP_NAME} ${cpp_files} ${h_files} )

link_directories("../../lib")

IF (UNIX)
target_link_libraries(${APP_NAME}
	debug ${TRITON_LIBRARY_DEBUG} optimized ${TRITON_LIBRARY_RELEASE}
	${OPENSCENEGRAPH_LIBRARIES}
	${OPENGL_LIBRARY}
	${CMAKE_DL_LIBS}
	fftss
)
ELSE ()
target_link_libraries(${APP_NAME}
	debug ${TRITON_LIBRARY_DEBUG} optimized ${TRITON_LIBRARY_RELEASE}
	${OPENSCENEGRAPH_LIBRARIES}
	${OPENGL_LIBRARY}
	${CMAKE_DL_LIBS}
)
ENDIF ()
