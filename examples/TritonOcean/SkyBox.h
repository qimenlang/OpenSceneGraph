#ifndef SKYBOX_H
#define SKYBOX_H

namespace osg
{

class Node;
class TextureCubeMap;
};

osg::Node*  createSkyBox( osg::TextureCubeMap* & cubeMap );

#endif