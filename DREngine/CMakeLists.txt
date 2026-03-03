set(DR_ENGINE_NAME OSGEngine)
add_executable(${DR_ENGINE_NAME} main.cpp deferred.cpp teapot.cpp)
target_link_libraries(${DR_ENGINE_NAME} PRIVATE osgViewer osgDB osgGA osgUtil osg osgShadow)


add_custom_command(TARGET ${DR_ENGINE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:${DR_ENGINE_NAME}>
        $<TARGET_FILE_DIR:${DR_ENGINE_NAME}>
    COMMAND_EXPAND_LISTS
)

add_custom_command(TARGET ${DR_ENGINE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_PDB_FILE:osgViewer>
        $<TARGET_PDB_FILE:osgDB>
        $<TARGET_PDB_FILE:osgGA>
        $<TARGET_PDB_FILE:osgUtil>
        $<TARGET_PDB_FILE:osg>
        $<TARGET_PDB_FILE:osgShadow>
        $<TARGET_FILE_DIR:${DR_ENGINE_NAME}>
)
