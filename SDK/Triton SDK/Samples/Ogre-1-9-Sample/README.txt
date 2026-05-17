Be sure the OGRE_HOME environment variable is set on your system, pointing to your Ogre installation. Otherwise, this sample won't be able to link.

Also ensure your debugger settings set the working directory to be the Ogre bin debug or release directory, otherwise you will get a DLL not found error upon starting. If you are using the OpenGL renderer, you will also need to add the following line into your resources.cfg and resources_d.cfg files:

FileSystem=../../Media/materials/programs/GLSL
