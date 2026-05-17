SET(TARGET_SRC ${TARGET_SRC} 
	main.cpp 
	SkyBox.cpp 
	TritonDrawable.cpp 
)

SETUP_EXAMPLE(TritonOcean)
target_link_libraries(${TARGET_TARGETNAME} debug ${TRITON_LIBRARY_DEBUG} optimized ${TRITON_LIBRARY_RELEASE})
target_include_directories(${TARGET_TARGETNAME} PRIVATE ${TRITON_INCLUDE_DIR})