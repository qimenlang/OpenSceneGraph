SET(TARGET_SRC ${TARGET_SRC} 
	src/main.cpp 
	src/SkyBox.cpp 
	src/TritonDrawable.cpp 
	src/PlanarReflection.cpp
)

SETUP_EXAMPLE(TritonOceanReflection)
target_link_libraries(${TARGET_TARGETNAME} debug ${TRITON_LIBRARY_DEBUG} optimized ${TRITON_LIBRARY_RELEASE})
target_include_directories(${TARGET_TARGETNAME} PRIVATE ${TRITON_INCLUDE_DIR})