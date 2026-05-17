SET(TARGET_SRC ${TARGET_SRC}
	src/main.cpp
	src/SkyBox.cpp
	src/TritonDrawable.cpp
)

# osgdb_ive depends on these modules at runtime (see src/osgPlugins/ive/CMakeLists.txt)
SET(TARGET_ADDED_LIBRARIES osgTerrain osgSim osgFX osgVolume)

SETUP_EXAMPLE(TritonDynamicHeightMapSample)
target_link_libraries(${TARGET_TARGETNAME} debug ${TRITON_LIBRARY_DEBUG} optimized ${TRITON_LIBRARY_RELEASE})
target_include_directories(${TARGET_TARGETNAME} PRIVATE ${TRITON_INCLUDE_DIR})

# In-tree builds leave versioned DLLs under src/<module>/; copy them beside the example so osgdb_ive can load.
if(WIN32)
	foreach(_osg_ive_dep osgTerrain osgSim osgFX osgVolume)
		if(TARGET ${_osg_ive_dep})
			add_custom_command(TARGET ${TARGET_TARGETNAME} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
					$<TARGET_FILE:${_osg_ive_dep}>
					$<TARGET_FILE_DIR:${TARGET_TARGETNAME}>
				COMMENT "Deploy ${_osg_ive_dep} for osgdb_ive (.ive reader)"
			)
		endif()
	endforeach()
endif()
