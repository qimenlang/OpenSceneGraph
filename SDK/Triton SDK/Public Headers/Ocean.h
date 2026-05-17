// Copyright (c) 2011-2021 Sundog Software, LLC. All rights reserved worldwide.

#ifndef TRITON_OCEAN_H
#define TRITON_OCEAN_H

/** \file Ocean.h
   \brief Triton's Ocean model interface.
 */

#ifdef SWIG
#define TRITONAPI
%module TritonOcean
%include "carrays.i"
%include "cpointer.i"
%import "Environment.h"
%pointer_functions(float, floatp);
%array_functions( double, double_array )
SWIG_CSBODY_PROXY(public, public, SWIGTYPE)
SWIG_CSBODY_TYPEWRAPPER(public, public, public, SWIGTYPE)
%{
#include "Ocean.h"
using namespace Triton;
%}
#endif

#include "Environment.h"
#include "WakeGenerator.h"
#include "RotorWash.h"
#include "TidalStreamWake.h"

#pragma pack(push)
#pragma pack(8)

namespace Triton
{
class ProjectedGrid;
class WakeManager;
class ParticleSystemManager;
class SprayManager;
class DecalManager;
class Mutex;
class GodRays;
class Camera;
class WaterModelInitializationParams;
class ComplexNumber;

/** Enumerates the different water models available for simluating ocean waves. */
enum WaterModelTypes {
    TESSENDORF,  /** Tessendorf's ocean wave model with the Phillips wave spectrum, using fast Fourier transforms to simulate large numbers of waves at once. Good for open ocean when you value speed over accuracy. */
    PIERSON_MOSKOWITZ, /** An FFT-powered Pierson-Moskowitz wave spectrum. Produces accurate waves that assume an infinite wind fetch. */
    JONSWAP, /** The most accurate wave model, which takes wind fetch length into account. FFT-powered so it's still fast, but it responds more slowly when the wind conditions are changed. */
    UNKNOWN_WATERMODEL
};

/** Enumerates the different shader programs used internally by Triton. */
enum Shaders {
      WATER_SURFACE
    , WATER_SURFACE_PATCH
    , SPRAY_PARTICLES
    , WAKE_SPRAY_PARTICLES
    , GOD_RAYS
    , WATER_DECALS
};

/** Enumerates the ocean quality settings used in Ocean::SetQuality() */
enum OceanQuality {
    GOOD,
    BETTER,
    BEST
};

/// A structure describing a complex number, for use in Environment::OverrideFFTInputArray.
class TritComplex : public MemObject
{
public:
	float real, imaginary;
};

/** The Ocean class allows you to configure and draw Triton's water simulation. */
class Ocean : public MemObject
{
public:
    /** Creates an Ocean instance tied to the given Environment, using the specified wave model.
        Make sure this is called only after your graphics context has been initialized and is active!

        \param env      A pointer to an Environment object created previously, which contains the environmental
                        conditions, coordinate system, camera, and rendering system used by the Ocean. The caller
                        is responsible for deleting this object after the Ocean is deleted.
        \param type     You may choose between the faster TESSENDORF implementation, or the more physically accurate 
                        JONSWAP and PIERSON_MOSKOWITZ spectral models. JONSWAP is an extension of PIERSON_MOSKOWITZ 
                        that accounts for wind "fetch length," so be sure to specify realistic distances (~100km) 
                        traveled by the wind in your WindFetch objects to make the most of JONSWAP. 
        \param enableHeightTests Specifies whether the application will call Ocean::GetHeight() or not. If false,
                        Triton may be able to keep the ocean simulation entirely on the GPU leading to better
                        performance, but any calls to Ocean::GetHeight() may return 0. Set to true if you need
                        to read back height information from the water surface.
        \param enableBreakingWaves Specifies whether shoreline effects with breaking waves are enabled. This does
                        create additional demands on the vertex and fragment programs and should only be enabled if
                        you're going to use them. Breaking waves require a floating-point height map containing 
                        bathymetry data to be passed in via Environment::SetHeightMap(). \sa Environment::SetBreakingWavesParameters()
        \param quality  Specifies the tradeoff you want between wave detail and performance. Choose from GOOD, BETTER, or BEST.
        \return         An instance of Ocean that may be used for rendering. The caller is responsible for deleting
                        this object when finished. NULL may be returned if the ocean could not initialize itself;
                        in this case, enable the setting enable-debug-messages in resource/triton.config to get
                        more details on what went wrong sent to your debugger output window. Contact
                        support@sundog-soft.com with this output if necessary.
    */
    static Ocean * TRITONAPI Create(Environment *env, WaterModelTypes type = JONSWAP, bool enableHeightTests = false,
        bool enableBreakingWaves = false, OceanQuality quality = GOOD);

#ifndef SWIG
    /** Creates an Ocean instance tied to the given Environment, using additional user-supplied OpenGL shader objects.
        Make sure this is called only after your graphics context has been initialized and is active!

        \param env      A pointer to an Environment object created previously, which contains the environmental
                        conditions, coordinate system, camera, and rendering system used by the Ocean. The caller
                        is responsible for deleting this object after the Ocean is deleted.
        \param userShaders A vector of OpenGL shader objects (created via glCreateShader) that will be linked into
                        all shader programs created by Triton. This allows you to provide your own shader functions to
                        Triton, which your modified Triton shaders may then call into. Triton::Ocean::GetShaderObject()
                        may be used to retrieve the linked shader programs, allowing you to set any uniforms your 
                        functions require. The caller is responsible for deleting these custom shader object after
                        this Ocean object has been deleted.
        \param type     You may choose between the faster TESSENDORF implementation, or the more physically accurate 
                        JONSWAP and PIERSON_MOSKOWITZ spectral models. JONSWAP is an extension of PIERSON_MOSKOWITZ 
                        that accounts for wind "fetch length," so be sure to specify realistic distances (~100km) 
                        travelled by the wind in your WindFetch objects to make the most of JONSWAP. 
        \param enableHeightTests Specifies whether the application will call Ocean::GetHeight() or not. If false,
                        Triton may be able to keep the ocean simulation entirely on the GPU leading to better
                        performance, but any calls to Ocean::GetHeight() may return 0. Set to true if you need
                        to read back height information from the water surface.
        \param enableBreakingWaves Specifies whether shoreline effects with breaking waves are enabled. This does
                        create additional demands on the vertex and fragment programs and should only be enabled if
                        you're going to use them. Breaking waves require a floating-point height map containing 
                        bathymetry data to be passed in via Environment::SetHeightMap(). \sa Environment::SetBreakingWavesParameters()
        \param quality  Specifies the tradeoff you want between wave detail and performance. Choose from GOOD, BETTER, or BEST.
        \return         An instance of Ocean that may be used for rendering. The caller is responsible for deleting
                        this object when finished. NULL may be returned if the ocean could not initialize itself;
                        in this case, enable the setting enable-debug-messages in resource/triton.config to get
                        more details on what went wrong sent to your debugger output window. Contact
                        support@sundog-soft.com with this output if necessary.
    */
    static Ocean * TRITONAPI Create(Environment *env, const TRITON_VECTOR(unsigned int)& userShaders, WaterModelTypes type = JONSWAP, 
        bool enableHeightTests = false, bool enableBreakingWaves = false, OceanQuality quality = GOOD);
#endif

    /** Virtual destructor. To ensure proper cleanup, be sure to delete your Ocean object before your Environment object. */
    virtual ~Ocean();

    /** Draws an infinite ocean surrounding the camera (as specified in the Environment object) for the simulated
        conditions at the given time.

        \param time The simulated point in time to render, in seconds. Note that this is an absolute time which
                    can be relative to any arbitrary point in time. It's not the delta time between frames.
                    In order to avoid precision issues; keep this value as small as possible. Best practice is 
                    to make it relative to the application startup time.
        \param depthWrites Whether the ocean will write to the depth buffer; you should probably leave this set
                    to true, since disabling it will lead to some artifacts. One technique for integrating
                    the ocean with terrain is to draw the ocean first with depth writes disabled, then
                    draw the terrain on top of it, thereby avoiding any depth buffer precision issues. Be sure
                    to remove or disable any existing ocean surfaces from your terrain database first if using
                    this technique. Be aware that disabling depth writes will lead to artifacts at certain camera
                    angles, since the waves won't be able to depth-sort against themselves; a better and simpler
                    approach would be to leave depthWrites on, and then clear the depth buffer after calling Draw
                    early in your frame.
        \param drawWater Whether the water surface should be drawn in this call. You may want to draw the water
                    (which is non-transparent) separately from the spray particles (which are transparent.) This parameter,
                    together with drawParticles, lets you choose what combination of water and particles should be drawn
                    with this call.
        \param drawParticles Whether spray particles from breaking waves, rotor wash, impacts, etc. are drawn with this 
                    call.
        \param context An optional context, depending on the rendering system in question. For example, you can
                    render in DX 11 into a command list in a deferred context and execute the command list on
                    the immediate context at a later time.

        \param camera An optional camera. If you have multiple views, you can pass in the camera directly. This is the preferred
               method than setting the view using Environment class methods: Environment::SetCameraMatrix(), Environment::SetProjectionMatrix().
               In conjunction with multiple contexts, you can do multi-threaded rendering as well (different draw calls can be in different threads).
               However, for multiple contexts, multiple-threads and scenarios like stereo-rendering where the left/right views don't differ by much, 
               its faster to use Ocean::DrawConcurrent(). See Ocean::DrawConcurrent().
    */
    virtual void TRITONAPI Draw(double time, bool depthWrites = true, bool drawWater = true, bool drawParticles = true, void* context = 0, const Camera* camera = 0);


    /** Draws an infinite ocean surrounding the camera (as specified in the Environment object) for the simulated
        conditions at the given time. This is a specialized method for any variation of multi-threaded, multi-context or multi-view
        scenarios. If for example you have multiple contexts, each with a different view, you call this method from different threads
        drawing into different contexts (contexts here means not a separate window necessarily but for example a deferred DX11 context or
		an OpenGL command list based drawing application with multiple views). 
        Following this you *must* call Ocean::PostDrawAll() after all the concurrent draws have completed from the main thread. 
		Ocean::PostDrawConcurrent() is automatically called when using the Ocean::Draw() method. This method is inherently thread safe
		, so the thread-safe option in Triton.config is ignored.

        \param time The simulated point in time to render, in seconds. Note that this is an absolute time which
                    can be relative to any arbitrary point in time. It's not the delta time between frames.
                    In order to avoid precision issues; keep this value as small as possible. Best practice is 
                    to make it relative to the application startup time.
        \param depthWrites Whether the ocean will write to the depth buffer; you should probably leave this set
                    to true, since disabling it will lead to some artifacts. One technique for integrating
                    the ocean with terrain is to draw the ocean first with depth writes disabled, then
                    draw the terrain on top of it, thereby avoiding any depth buffer precision issues. Be sure
                    to remove or disable any existing ocean surfaces from your terrain database first if using
                    this technique. Be aware that disabling depth writes will lead to artifacts at certain camera
                    angles, since the waves won't be able to depth-sort against themselves; a better and simpler
                    approach would be to leave depthWrites on, and then clear the depth buffer after calling Draw
                    early in your frame.
        \param drawWater Whether the water surface should be drawn in this call. You may want to draw the water
                    (which is non-transparent) separately from the spray particles (which are transparent.) This parameter,
                    together with drawParticles, lets you choose what combination of water and particles should be drawn
                    with this call.
        \param drawParticles Whether spray particles from breaking waves, rotor wash, impacts, etc. are drawn with this 
                    call.
        \param context An optional context, depending on the rendering system in question. For example, you can
                    render in DX 11 into a command list in a deferred context and execute the command list on
                    the immediate context at a later time.
        \param camera An optional camera. In conjunction with multiple contexts, you can do multi-threaded rendering as well using this method, and is
                    the preferred method when cameras don't differ much (e.g. left/right stereo rendering). The sequence of calls is:
                    Environment::SetProjectionMatrix() -> for the main view from which the left/right views are derived
                    Environment::SetCameraMatrix()     -> for the main view from which the left/right views are derived
                    Environment::UpdateSimulation()    -> needs to be called only once!
                    Ocean::DrawConcurrent(left camera)
                    Ocean::DrawConcurrent(right camera)
                    Ocean::PostDrawConcurrent
                    
    */
    virtual void TRITONAPI DrawConcurrent(double time, bool depthWrites = true, bool drawWater = true, bool drawParticles = true, void* context = 0, const Camera* camera = 0);

    /** Method that *must* be called from the main thread after all Ocean::DrawConcurrent calls have completed
        to give a chance to Triton to do some post draw house keeping after the draw calls have completed. This
        is automatically called when Ocean::Draw is uesd.
    */
    void TRITONAPI PostDrawConcurrent();

    /** Sets the shaders and state necessary for rendering a user-provided patch of geometry as water. Use this if you
        don't want Triton to draw an infinite ocean using Ocean::Draw(), but only want to draw your own water patches.
        All that's required is the rendering of vertex data containing 3D position data following this call, and
        a call to Ocean::UnsetPatchShader() must be called after you've drawn your water geometry.

        The geometry drawn should be flat, relative to sea level. Our shaders will displace the vertices rendered
        and apply all the textures and reflections to make it look like water. Remember to call Ocean::SetDepth() if
        you are drawing an area of shallow water and don't want this patch to look like open ocean.

        For best results, ensure your depth tests are set to "less than or equal" to ensure proper sorting of
        Triton's waves against each other.

        Note that particle-based spray effects won't be rendered when you're drawing your own geometry, all this does
        is shade the geometry you draw following this call to make your geometry look like water. You'll still get
        foam, reflections, and refractions. This method only works with TESSENDORF waves.

        If you are using a WGS84 or SPHERICAL CoordinateSystem, our shaders assume that your mesh is positioned relative
        to the center of the Earth being at 0, 0, 0. To preserve precision, your vertices when transformed by the modelMatrix
        provided should be relative to the camera position. Our shaders are compiled at runtime, so you may modify them 
        if your data is of an unusual format. Look for the shaders ending with -patch.fx or -patch.glsl inside the Resources 
        directory; these are used only by this method.

        If your application is using Triton's volumetric decal features, you must draw your patch a second time with the
        depthPass parameter set to "true" on both SetPatchShader() and UnsetPatchShader(). This will generate a depth
        texture needed for projecting the volumetric decals properly.

        OpenGL users should ensure that your vertex array is bound prior to calling this method; DirectX11 users
        should set their vertex buffer with the input assembly stage prior to calling this method.

        \sa UnsetPatchShader()
        \sa SetPatchMatrix()

        \param time The simulated point in time to render, in seconds. Note that this is an absolute time which
                    can be relative to any arbitrary point in time. It's not the delta time between frames.
                    TAKE CARE that this value is not too large; you may want to make it relative to your
                    application start time, or fmod it with the number of seconds in one day for example.
                    The GPU wave simulation is single precision, and very large time values will result in
                    jittery wave animations.
        \param vertexStride The number of bytes between vertices in your vertex array. This is only used in OpenGL 3.2+
                            and DirectX11 to allow binding of your vertex array with the appropriate vertex attribute in
                            our shaders.
        \param positionOffset The number of bytes from the start of each vertex to the x,y,z floating point position
                              data. Used only in OpenGL 3.2+.
        \param doublePrecisionVertices Pass in true if your vertex data is "double" precision, or false for "float" single
                                       precision data. Only relevant in OpenGL 3.2 or newer.
        \param modelMatrix Receives an array of 16 doubles defining a 4x4 model matrix for the ocean patch. If set to NULL,
                           an identity matrix is assumed.
        \param decalPass If you are using Triton::Decal objects, or the decal option on the rotor wash effect, you must 
                         draw your patch of water a second time while passing true for the decalPass parameter on 
                         SetPatchShader() and UnsetPatchShader(). This will generate the depth texture that volumetric
                         decals need in order to project themselves properly onto the water surface. If SetPatchShader
                         returns false while decalPass is true, then no decals are in view and you should not proceed
                         to re-draw your ocean geometry. You do still need to balance this call with a call to 
                         UnsetPatchShader() no matter what, however.
        \param context An optional context, depending on the rendering system in question. For example, you can
                       render in DX 11 into a command list in a deferred context and execute the command list on
                       the immediate context at a later time.
        \param camera An optional camera for the view being rendered.
        \return True if the state was successfully set and drawing may continue.
    */
    virtual bool TRITONAPI SetPatchShader(double time, int vertexStride, int positionOffset, bool doublePrecisionVertices,
                                          const double *modelMatrix = 0, bool decalPass = false, void* context = 0, const Camera* camera = 0);

    /** If you are drawing many of your own water meshes using SetPatchShader() at once, it will be much faster to call
        SetPatchShader() once, then call SetPatchMatrix() for each individual mesh to set its location, followed by
        UnsetPatchShader() when you're done drawing all of them. This is only useful if you are drawing many water patches
        in the same scene, but it can have a large performance benefit if so.

        \param modelMatrix Receives an array of 16 doubles defining a 4x4 model matrix for the ocean patch. If set to NULL,
                           an identity matrix is assumed.
        \param context An optional context, depending on the rendering system in question. For example, you can
                       render in DX 11 into a command list in a deferred context and execute the command list on
                       the immediate context at a later time.
        \param camera An optional camera for the view being rendered.
        \return True if the matrix was set successfully.
   */
    virtual bool TRITONAPI SetPatchMatrix(const double *modelMatrix, void* context = 0, const Camera* camera = 0);

    /** Restores the graphics state prior to a previous call to Ocean::SetPatchShader(). Every call to Ocean::SetPatchShader()
        must be matched with a call to Ocean::UnsetPatchShader() following the drawing of any user-defined patches of water.
        If you want particle-based spray effects, you may optionally pass in a timestamp and bounding box for your patch to
        enable these effects.
        \param time The current time, in seconds, used for particle animations. Optional; set to 0.0 if unused.
        \param patchBounds A bounding box defining the bounds of your patch, used for particle animations. Optional.
        \param decalPass If you are using Triton::Decal objects, or the decal option on the rotor wash effect, you must 
                         draw your patch of water a second time while passing true for the decalPass parameter on 
                         SetPatchShader() and UnsetPatchShader(). This will generate the depth texture that volumetric
                         decals need in order to project themselves properly onto the water surface. The decals themselves
                         will also be rendered at this point.
        \param context An optional context, depending on the rendering system in question. For example, you can
                       render in DX 11 into a command list in a deferred context and execute the command list on
                       the immediate context at a later time.
        \param camera An optional camera for the view being rendered.
        \sa SetPatchShader()
    */
    virtual void TRITONAPI UnsetPatchShader(double time = 0.0, const TBoundingBox* patchBounds = 0, bool decalPass = false, void* context = 0, const Camera* camera = 0);

#ifndef SWIG
    /** OpenGL only: Reload the underlying shader programs, linking in a new list of user-supplied shader object ID's with each program. 
        \param reloadSurface Indicates whether the shaders used for the water surface should be recompiled and relinked.
        \param reloadWakes Indicates whether the shaders for wake spray effects should be recompiled and relinked.
        \param reloadSprays Indicates whether the shaders for spray effects on high waves should be recompiled and relinked.
        \param reloadDecals Indicates whether the shaders for ocean decals should be recompiled and relinked.
        \param reloadGodRays Indicates whether the shaders for underwater crepuscular rays should be recompiled and relinked. */
    virtual bool ReloadShaders(const TRITON_VECTOR(unsigned int)& shaders, bool reloadSurface = true, 
        bool reloadWakes = true, bool reloadSprays = true, bool reloadDecals = true, bool reloadGodRays = true);
#endif

    /** In OpenGL, this must be called with each set of context and camera that you intend to render with
        See the OpenGL multi-contexts sample*/
    virtual bool Initialize(void* context, const Camera* camera);

    /** Updates the underlying wave simulation; calling this is optional and only necessary if you wish to perform
        physics updates from a pass or thread different from the rendering pass or thread. If UpdateSimulation() is
        not called prior to Draw() or SetPatchShader(), then Draw() or SetPatchShader() will call it automatically.
        A mutex is enforced between this method and the Draw(), SetPatchShader(), and GetHeight() methods. A valid GL
        context must still be present while calling this.

        \param time The simulated point in time to render, in seconds. Note that this is an absolute time which
                    can be relative to any arbitrary point in time. It's not the delta time between frames.
                    TAKE CARE that this value is not too large; you may want to make it relative to your
                    application start time, or fmod it with the number of seconds in one day for example.
                    The GPU wave simulation is single precision, and very large time values will result in
                    jittery wave animations.

        \param camera An optional camera. Simulations are view dependent. If you have multiple views and are
                      doing the updates manually using this method, you must specify the camera in that case.
        \sa DrawConcurrent()
    */
    virtual void TRITONAPI UpdateSimulation(double time, const Camera* camera = 0);

    /** DirectX9 users must call D3D9DeviceLost() in response to lost devices, prior to resetting the device. A lost
        device may occur when the user locks and unlocks a system or changes the monitor resolution, and must be
        explicitly handled under DX9. Call D3D9DeviceReset() once the device has been recreated. */
    void TRITONAPI D3D9DeviceLost();

    /** DirectX9 users must call D3D9DeviceReset() in response to device resets done in response to lost devices.
        You must have called D3D9DeviceLost() first in response to the lost device. */
    void TRITONAPI D3D9DeviceReset();

    /** Retrieves the choppiness setting of the Ocean, which controls how peaked the waves are. \sa SetChoppiness() */
    float TRITONAPI GetChoppiness() const;

    /** Set the choppiness of the waves, which controls how peaked the waves are. \sa GetChoppiness()
        \param chop The choppiness parameter; 0.0 yields no chop, 3.0 yields strong chop. Values that are too
                    high may result in wave geometry folding over itself, so take care to set reasonable
                    values.
    */
    void TRITONAPI SetChoppiness(float chop);

    /** Retrieves the looping period of the Ocean, which define time after which wave simulation repeats. \sa SetLoopingPeriod() */
    float TRITONAPI GetLoopingPeriod() const;

    /** Set the looping period of the Ocean, which define time after which wave simulation repeats. \sa GetLoopingPeriod()
        If set to non zero, wave sumulation loops after that period. 0 is the default. FFT Wave model uses mixture of compound
        frequency waves. When Period value is set, these compound frequencies are constrained to be a multiply of looping frequence.
        Hence period may affect the wave shapes. General rule is to use highest possible period length to minimize the imapact
        on wave field realism. We recommend periods larger than 30 seconds.
    */
    void TRITONAPI SetLoopingPeriod(float loopingPeriod);

    /** Retrieves the simulated depth of the water in world units, and the surface
        normal of the sea floor, both at the current camera position. Only returns
        the values set by SetDepth(). \sa SetDepth()
        \param floorNormal A reference to a Vector3 to retrieve the surface normal
                           of the sea floor as set by SetDepth().
        \param camera An optional camera. Some computations of depth are camera dependent. So
                      you need to specify the camera in question.
        \return The depth of the sea floor at the camera position, as set by SetDepth().
    */
    float TRITONAPI GetDepth(Triton::Vector3& floorNormal, const Camera* camera = 0) const;

    /** Sets the simulated depth of the water in world units at the camera position,
        and the slope of the sea floor as specified by its surface normal at the
        camera position. This information is used to interpolate the water depth
        at various positions in the scene, affecting the transparency of the water
        as well as the height of the waves. Avoid changing this every frame for
        performance reasons. \sa GetDepth()

        \param depth The depth of the water, in world units, at the camera position.
                     Negative values will be clamped to zero. For open ocean, either
                     do not call this method or set depth to a large number (like 1000).
        \param floorNormal The surface normal of the sea floor at the camera position.
                     The point defined by the depth parameter under the camera position
                     together with this normal will define a plane that approximates the
                     position of the sea floor surrounding the current location.
        \param camera An optional camera. Some computations of depth are camera dependent. So
                     you need to specify the camera in question.
    */
    void TRITONAPI SetDepth(float depth, const Triton::Vector3& floorNormal, const Camera* camera = 0);

    /** Enables or disables wireframe rendering of the ocean's mesh.
        \param wireframeOn Set to true to render in wireframe mode, false to render normally.
    */
    void TRITONAPI EnableWireframe(bool wireframeOn);

    /** Returns a description of the FFT transform being used, if a FFT water model
        is active. */
    const char * TRITONAPI GetFFTName() const;

    /** Returns the number of triangles rendered by the underlying projected grid. */
    unsigned int TRITONAPI GetNumTriangles() const;

    /** Retrieves an underlying shader object used to render the water. If you make modifications
        to our shaders to add additional effects, you can use this in order to pass your own
        uniform variables into the shaders. Depending on the renderer you're using, you'll need
        to cast this to a GLuint, ID3DXEffect, or ID3DX11Effect.

        If you are shading the water surface, be sure to pass uniforms to both WATER_SURFACE and
        WATER_SURFACE_PATCH. The latter is used when the camera is near the water surface.

        \param shaderProgram Specifies which shader program you want to retrieve: WATER_SURFACE,
        WATER_SURFACE_PATCH, SPRAY_PARTICLES, WAKE_SPRAY_PARTICLES, or WATER_DECALS.

        \param context Shaders are context and camera specific. Internally, there is a different
        shader for each key of context (DX 11 command list/deferred context or OpenGL Stream) and
        Camera. This is so as to support multi-threaded rendering seamlessly. Therefore, you must
        provide context and camera to get the shader in question for that key of context and camera
        If no context is specified, Triton will use the immediate context for DX 11 and the immediate
        OpenGL stream for OpenGL. If no camera is  specified, the default environment camera is used 
        as as part of the key. 

        \param camera Shaders are context and camera specific. Internally, there is a different
        shader for each key of context (DX 11 command list/deferred context or OpenGL Stream) and
        Camera. This is so as to support multi-threaded rendering seamlessly. Therefore, you must
        provide context and camera to get the shader in question for that key of context and camera
        If no context is specified, Triton will use the immediate context for DX 11 and the immediate
        OpenGL stream for OpenGL. If no camera is  specified, the default environment camera is used
        as as part of the key.

        \return The GLuint, ID3DXEffect, or ID3DX11Effect representing the shader object,
        or 0 if no shader is loaded.
    */
    ShaderHandle TRITONAPI GetShaderObject(Shaders shaderProgram, void* context = 0, const Camera* camera = 0) const;

    // Retrives the underlying WakeManager. For internal use only.
    WakeManager * TRITONAPI GetWakeManager() {
        return wakeManager;
    }

    // Retrieves the underlying DecalManager. For internal use only.
    DecalManager * TRITONAPI GetDecalManager() {
        return decalManager;
    }

    /** Applies a decal texture to the dynamic ocean surface, useful for effects involving films, debris, or foam
        on the water.
        
        \param texture The texture to apply to the water surface. Under OpenGL, this must be a GLuint indicating the ID of the GL_TEXTURE_2D returned from 
        glGenTextures. Under DirectX9, this must be a LPDIRECT3DTEXTURE9. Under DirectX11, this must be a 
        ID3D11ShaderResourceView pointer with an underlying ViewDimension of D3D11_SRV_DIMENSION_TEXTURE2D.

        \param size The size of the decal, in world units.
        
        \param position The center position of the decal on the water surface, in world units.

        \return An identifier for this decal, which may be passed to RemoveDecal() to later dispose of this decal.
    */
    DecalHandle TRITONAPI AddDecal(TextureHandle texture, float size, const Vector3& position);

    /** Scales an existing decal in width and depth at runtime.
        \param decal A decal handle retrieved from a previous call to AddDecal().
        \param scaleWidth The scale factor for the decal's width (ie, 0.5 = half size, 2.0 = double.)
        \param scaleDepth The scale factor for the decal's depth.
    */
    void TRITONAPI ScaleDecal(DecalHandle decal, float scaleWidth, float scaleDepth);

    /** Moves an existing decal to the given location. */
    void TRITONAPI MoveDecal(DecalHandle decal, const Vector3& position);

    /** Rotates an existing decal by the given amount in radians.
        \param decal A decal handle retrieved from a previous call to AddDecal().
        \param radians The rotation of the decal, given in radians.
    */
    void TRITONAPI RotateDecal(DecalHandle decal, double radians);

    /** Sets a decal's alpha blending amount (default is 1.0.)
        \param decal A decal handle retrieved from a previous call to AddDecal().
        \param alpha The alpha value applied to the decal texture. 0 = transparent, 1.0 = opaque.
    */
    void TRITONAPI SetDecalAlpha(DecalHandle decal, float alpha);

    /** Removes a decal texture previously applied with AddDecal(). */
    void TRITONAPI RemoveDecal(DecalHandle decal);

    /** Retrieves the height and normal of the ocean surface at the intersection point of the given ray. The
        results of this method are only valid if Ocean::Create() was called with the parameter
        enableHeightReads set to true. The height returned is relative to sea level, as specified
        by Triton::Environment::SetSeaLevel(). For example, the crest of a one-meter-high wave will
        always return a height of one meter, irrespective of the environment's sea level height.
        Depending on the application, you may want to add in the result of
        Triton::Environment::GetSeaLevel() to the height returned.

        \param point The origin of the ray to test the height against.
        \param direction The normalized direction vector of the ray.
        \param height Receives the height at the ray's intersection with the ocean, if an
                      intersection was found.
        \param normal Receives a normalized unit vector pointing in the direction of the normal
                      vector of the sea surface at the intersection point.
        \param visualCorrelation Set to true in order to have the height returned match the visuals,
                      which dampen height offsets with distance to avoid sampling artifacts. To return
                      the true wave height at the given location, set to false.
        \param includeWakes Whether this height query should include waves from rotor wash 
                      and impacts in its results. Ship Kelvin wakes will be approximated.
        \param highResolution If true, the ocean grid points surrounding the intersection point will be
                      sampled and interpolated. If false, the height will be based on the nearest
                      grid point alone.
        \param threadSafe Whether a mutex will be locked with this call to ensure thread safety with
                      Ocean::Draw(). If you are conducting many intersection tests and are only single
                      threaded, pass false for better performance. You may also use Ocean::Lock() and
                      Ocean::Unlock() to manually lock and unlock the mutex surrounding many GetHeight calls.
		\param intersectionPoint If you need an intersection point on the water surface and not just a height
		              value, pass in a pointer to a Triton::Vector3 object and we'll populate it with the result.
					  Otherwise, pass NULL. NOTE that this intersection is computed as the intersection of the
                      ray with sea level, with the height of the waves at that point added in. It will become
                      less accurate as your ray becomes more oblique with the ocean surface. For accurate
                      intersection tests including waves from oblique angles, see Triton::Ocean::GetWavesIntersection().
        \param autoFlip If true, GetHeight() will flip your direction vector automatically if it does not intersect
                      the water surface at all, in an attempt to return an intersection. This lets you not worry
                      about whether your origin point is above or below the water surface. If you'd rather just
                      have GetHeight() return false when your direction is pointing away from the water, set 
                      autoFlip to false instead.
        \param camera An optional camera. Some computations of height are camera dependent. So
                      you need to specify the camera in question. This is particularly needed if you are syncing
					  height queries in a NO_RENDERER channel with other visual channels; you must pass
					  consistent camera positions for consistent results.
        \return True if an intersection was found, false if not. If a height map was passed in using
                Environment::SetHeightMap(), this will return false if the intersection is over terrain.
    */
    bool TRITONAPI GetHeight(const Vector3& point, const Vector3& direction, float& height, Vector3& normal,
                             bool visualCorrelation = true, bool includeWakes = true, bool highResolution = true,
                             bool threadSafe = true, Vector3 *intersectionPoint = 0, bool autoFlip = true, const Camera* camera = 0);

#ifndef SWIG
    /** This is a batch version of Triton::Ocean::GetHeight(), which will avoid stalling the OpenGL
    pipeline waiting for synchronization when retrieving heights for each point. This can be more efficient
    when retrieving many height lookups at once, but its optimizations are only useful under OpenGL.

    This method works in our internal tests, but is considered experimental for now.
    
    Retrieves the height and normal of the ocean surface at the intersection points of the given rays. The
    results of this method are only valid if Ocean::Create() was called with the parameter
    enableHeightReads set to true. The heights returned are relative to sea level, as specified
    by Triton::Environment::SetSeaLevel(). For example, the crest of a one-meter-high wave will
    always return a height of one meter, irrespective of the environment's sea level height.
    Depending on the application, you may want to add in the result of
    Triton::Environment::GetSeaLevel() to the height returned.

    \param points A vector of the origins of the rays to test the height against.
    \param directions A vector of the normalized direction vectors of each ray. This vector
    must match up with the points vector passed in.
    \param heights A reference to a vector of floats, which receives the heights at each
    ray's intersection with the ocean, if an intersection was found. These will be returned
    in the same order as the points passed in, as is the normal and intersectionPoints results.
    \param normals A reference to a vector of Vector3 objects, which receives normalized unit 
    vectors pointing in the direction of the normal vector of the sea surface at each intersection point.
    \param intersectionPoints A reference to a vector of Vector3 objects, which receives each intersection 
    point on the water surface. NOTE that this intersection is computed as the intersection of the
    ray with sea level, with the height of the waves at that point added in. It will become
    less accurate as your ray becomes more oblique with the ocean surface. For accurate
    intersection tests including waves from oblique angles, see Triton::Ocean::GetWavesIntersection().
    \param visualCorrelation Set to true in order to have the height returned match the visuals,
    which dampen height offsets with distance to avoid sampling artifacts. To return
    the true wave height at the given location, set to false.
    \param includeWakes Whether this height query should include waves from rotor wash
    and impacts in its results. Ship Kelvin wakes won't be included.
    \param highResolution If true, the ocean grid points surrounding the intersection point will be
    sampled and interpolated. If false, the height will be based on the nearest
    grid point alone.
    \param threadSafe Whether a mutex will be locked with this call to ensure thread safety with
    Ocean::Draw(). If you are conducting many intersection tests and are only single
    threaded, pass false for better performance. You may also use Ocean::Lock() and
    Ocean::Unlock() to manually lock and unlock the mutex surrounding many GetHeight calls.
    \param autoFlip If true, GetHeight() will flip your direction vector automatically if it does not intersect
    the water surface at all, in an attempt to return an intersection. This lets you not worry
    about whether your origin point is above or below the water surface. If you'd rather just
    have GetHeight() return false when your direction is pointing away from the water, set
    autoFlip to false instead.
    \param camera An optional camera. Some computations of height are camera dependent. So
    you need to specify the camera in question. This is particularly needed if you are syncing
    height queries in a NO_RENDERER channel with other visual channels; you must pass
    consistent camera positions for consistent results.
    \return True if valid heights, normals, and intersections have been populated for every point passed in.
    If false is returned, the results cannot be assumed to be valid for any point queried and you may want to
    fall back to Ocean::GetHeight() for each individual point. If a height map was provided to Triton,
    true will be returned even if some points are over terrain and not over water.
    */
    bool TRITONAPI GetBatchHeight(const TRITON_VECTOR(Vector3) points, const TRITON_VECTOR(Vector3) directions, TRITON_VECTOR(float)& heights, 
        TRITON_VECTOR(Vector3)& normals, TRITON_VECTOR(Vector3)& intersectionPoints,
        bool visualCorrelation = true, bool includeWakes = true, bool highResolution = true,
        bool threadSafe = true, bool autoFlip = true, const Camera* camera = 0);
#endif

    /** Retrieves an intersection between a ray and the ocean surface, taking waves and wakes into account. This method can account
        for rays at an oblique angle to the ocean; internally it marches along the ray at increments defined by the "precison"
        parameter, stopping at the point where a height test directly underneath each step is at or above the height of the step.

        To maximize performance, the ray marching starts at the point where the current maximum wave height could possibly
        cause an intersection. However, this is still an expensive function to call. If your rays are perpendicular to the ocean
        surface, you will get much better performance by using Triton::Ocean::GetHeight() instead and the intersectionPoint it returns.
        In the case of perpendicular rays, a quick height map lookup is all that's needed, and you don't need the complexity of this method.

        If you don't need to take waves and wakes into account, Triton::Ocean::GetIntersection() will be even faster.

        You can make your own trade-offs between performance and precision by using the "precision" parameter.

        \param point The origin of the ray to intersect with.
        \param direction The normalized direction vector of the ray.
        \param normal Receives a normalized unit vector pointing in the direction of the normal
        vector of the sea surface at the intersection point.
        \param intersectionPoint Receives the intersection of the ray with the water surface, within the precision requested.
        \param precision The size of the steps taken along the ray, searching for an intersection with the water surface.
        Increasing this will improve performance.
        \param visualCorrelation Set to true in order to have the height returned match the visuals,
        which dampen height offsets with distance to avoid sampling artifacts. To return
        the true wave height at the given location, set to false.
        \param threadSafe Whether a mutex will be locked with this call to ensure thread safety with
        Ocean::Draw(). If you are conducting many intersection tests and are only single
        threaded, pass false for better performance. You may also use Ocean::Lock() and
        Ocean::Unlock() to manually lock and unlock the mutex surrounding many GetHeight calls.
        \param camera An optional camera. Some computations of height are camera dependent. So
        you need to specify the camera in question. This is particularly needed if you are syncing
        height queries in a NO_RENDERER channel with other visual channels; you must pass
        consistent camera positions for consistent results.
        \return True if an intersection was found, false if not. If a height map was passed in using
        Environment::SetHeightMap(), this will return false if the intersection is over terrain.
    */
    bool TRITONAPI GetWavesIntersection(const Vector3& point, const Vector3& direction, Vector3& normal, Vector3& intersectionPoint,
                                        double precision = 0.1, bool visualCorrelation = true, bool threadSafe = true, 
                                        const Camera* camera = 0);

    /** Retrieves the intersection, if any, between a ray and the ocean surface. This method does NOT take
        wave heights into account; use Ocean::GetHeight() for that level of resolution.
        \param point The origin of the ray to intersect with, in world coordinates.
        \param direction The direction of the ray to intersect with, in world coordinates.
        \param intersection Receives the intersection point if an intersection exists.
        \param camera An optional camera. Some computations of intersection are camera dependent. So
               you need to specify the camera in question.
        \return True if an intersection was found, false if not. If height map data has been given using
        Environment::SetHeightMap(), false will also be returned if the intersection point is over terrain.
    */
    bool TRITONAPI GetIntersection(const Vector3& point, const Vector3& direction, Vector3& intersection, const Camera* camera = 0);

    /** Retrieves the wave direction. Normally this is the same as the wind direction, but in shallow water
        it will align with the slope of the sea floor as specified in SetDepth(). */
    float TRITONAPI GetWaveHeading() const {
        return waveHeading;
    }

    /** Enables or disables spray particle effects on breaking waves. This does incur a performance
        penalty, so if you need faster performance, try disabling spray effects or even disable it
        entirely with the fft-enable-spray config setting in resources/Triton.config. */
    void TRITONAPI EnableSpray(bool enable);

    /** Returns if spray particle effects on breaking waves are enabled, which they are by default.
        \sa EnableSpray(). */
    bool TRITONAPI SprayEnabled() const {
        return sprayEnabled;
    }

    /** Modifies the color used for refracted light rays that go into deep water. You can use this to modify the
        color of the water in areas that are not purely reflective.
        \param refractionColor the RGB color value of the deep water color; each component should be in the range 0-1.
    */
    void TRITONAPI SetRefractionColor(const Vector3& refractionColor);

    /** Returns the color of light refracted into the water.
        \sa SetRefractionColor();
        \return The RGB value of the refraction color.
    */
    const Vector3& TRITONAPI GetRefractionColor() const;

	/** Scales the intensity of reflections on the water (environment, planar, and light reflections combined.) 
		Default is 1.0. */
	void TRITONAPI SetReflectionScale(float reflectionScale);

	/** Retrieves any reflection scale set previously by SetReflectionScale(), or 1.0 otherwise. */
	float TRITONAPI GetReflectionScale() const;

    /** Sets the prominence of planar reflections on this ocean surface, if one was set using Triton::Environment::SetPlanarReflectionMap().
        \param percent The percent by which planar reflections will be blended into the reflected color of the water (0 - 1.0).
    */
    void TRITONAPI SetPlanarReflectionBlend(float blendPercent);

    /** Retrieves the current blend percentage for planar reflections. \sa SetPlanarReflectionBlend(). */
    float TRITONAPI GetPlanarReflectionBlend() const;

    /** A helper function for using planar reflections with Triton. This will suggest useful matrices for flipping
        your scene about the plane of the local water surface, and a texture matrix to transform world vectors into
        texture coordinates in a reflection map. You may need to transpose and/or adjust these matrices for left handed
        coordinates, depending on the conventions of your engine. The texture matrix returned will take the differences
        between OpenGL and DirectX normalized device coordinates and texture coordinates into account, however.
        \sa Triton::Environment::SetPlanarReflectionMap()

        \param reflectionMatrix A matrix that will flip the scene about the local water plane, taking the current sea level
        into account. If you multiply this matrix into your modelview matrix after your camera transforms, all objects will
        be flipped about the water plane - making them suitable for rendering into a reflection texture map to later be
        passed in to Triton::Environment::SetPlanarReflectionMap(). You will need to change the winding order used for back
        face culling when this matrix is active, and you may also need to enable a user clipping plane to prevent geometry
        that is normally underwater from rendering into your reflection texture.

        \param textureMatrix A matrix to transform world vectors into texture coordinate space of the reflection map texture.
        This matrix would be passed into Triton::Environment::SetPlanarReflectionMap(). This is equal to the product of
        view * projection * NDCtoTexCoords, based on the view and projection matrices last passed to Environment::SetCameraMatrix()
        and Environment::SetProjectionMatrix(). The matrix to transform normalized device coordinates to texture coordinates
        will differ depending on whether an OpenGL or DirectX renderer is being used.

        \return True if valid matrices were computed; will return false if the camera is below sea level, in which case you
        probably don't want reflections anyhow.
    */
    bool TRITONAPI ComputeReflectionMatrices(Matrix4& reflectionMatrix, Matrix3& textureMatrix, const Camera* camera = 0);

    /** Retrieves the environment this ocean is attached to. */
    const Environment * TRITONAPI GetEnvironment() const {
        return environment;
    }

    /** Returns whether the current camera position (from Environment::SetCameraMatrix()) is above the simulated
        water surface for this Ocean. */
    virtual bool TRITONAPI IsCameraAboveWater(const Camera* camera = 0);

    /** Applies a depth offset to the water, applied in the vertex program. This can be used to mitigate "z fighting" artifacts near
        shorelines. By default, there is no depth offset. A value of 0.01 is generally effective.
        \param offset The depth offset subtracted from the Z (depth) value in normalized coordinate space when rendering the ocean.
    */
    void TRITONAPI SetDepthOffset(float offset);

    /** Retrieves the depth offset (if any) previously set via Triton::Ocean::SetDepthOffset(), used to combat "z fighting" near coastlines. */
    float TRITONAPI GetDepthOffset() const;

    /** Sets the distance at which 3D wave displacements are dampened to prevent aliasing when moving the camera. */
    void TRITONAPI SetDisplacementDampingDistance(double distance);

    /** Retrieves the distance at which 3D wave displacements are dampened to prevent aliasing when moving the camera. */
    double TRITONAPI GetDisplacementDampingDistance() const;

    /** Turns the underwater crepuscular rays effect on or off. If the Triton.config setting underwater-god-rays-enabled is set to "no", this
        will have no effect. Defaults to false.
    */
    void TRITONAPI EnableGodRays(bool enable) {
        enableGodRays = enable;
    }

    /** Returns whether the underwater crepuscular rays effect is enabled. */
    bool TRITONAPI GodRaysEnabled() const {
        return godRays != 0 && enableGodRays;
    }

    /** Fades out the underwater crepuscular rays effect by the specified amount (0 = no fading, 1 = completely faded). */
    void TRITONAPI SetGodRaysFade(float fadeAmount);

    /** Returns the god ray fading amount set in Ocean::FadeGodRays(). */
    float TRITONAPI GetGodRaysFade() const;

    /** Explicitly locks the mutex used to ensure thread safety between the draw, update, and height query methods. For example, if
        you wish to conduct many calls to Ocean::GetHeight(), it is more efficient to enclose them with calls to Lock / Unlock and then
        pass false for Ocean::GetHeight()'s threadSafety parameter. Make sure this call is balanced by a call to Triton::Unlock() under
        every circumstance. */
    void TRITONAPI Lock();

    /** Explicitly locks the ocean's mutex previously locked by Ocean::Lock(). */
    void TRITONAPI Unlock();

    /** Set a quality setting (GOOD, BETTER, or BEST.) Higher quality will result in finer wave resolution, but at lower performance. 
        Default value is GOOD. Changing the quality setting requires deleting and re-initializing most of Triton's internal objects,
        and so there will be a pause while this change is processed. */
    void TRITONAPI SetQuality(OceanQuality quality);

    /** Retrieve the current simulation quality setting, either set by Ocean::SetQuality() or the default value of GOOD. */
    OceanQuality TRITONAPI GetQuality() const {
        return quality;
    }

    /** Sets use of linear color space, in which the ocean color will be raised to the power of 2.2 to negate the effects of 
        gamma correction. Only set this if you know you are rendering in linear color space. */
    void TRITONAPI SetLinearColorSpace(bool linearOn);

    /** Gets whether linear color space rendering is enabled via SetLinearColorSpace(). */
    bool TRITONAPI GetLinearColorSpace() const;

    /** Sets a maximum wave period in the simulated wave spectra, irrespective of the wind conditions requested. */
    void TRITONAPI SetMaximumWavePeriod(double maxPeriod);

    /** Retrieves the maximum wave period enforced regardless of wind conditions requested. */
    double TRITONAPI GetMaximumWavePeriod() const;

    /** Tells Triton about the projected Z values at your near and far clipping planes.
    May be used to dynamically switch between forward and reverse depth buffers,
    for example. Defaults to the Triton.config settings for [opengl|directx]-[near|far]-clip */
    void TRITONAPI SetClipDistances(double nearClip, double farClip);

    /** Retrieves Triton's current assumptions about the projected Z values at your near
    and far clipping planes. */
    void TRITONAPI GetClipDistances(double& nearClip, double& farClip) const;

	/** Explicitly overrides the H0 input array of wave amplitudes for given wavelengths
	and directions, as described in Tessendorf's paper. This allows you to completely
	override Triton's wave model with your own, but you really have to know what
	you're doing. The size of this array must be of (N+1) * (M+1), where N and M
	are given by the Triton.config settings fft-grid-size-x and fft-grid-size-y, and
	both N and M are multiplied by 2 if BEST quality is selected for the ocean.
	You can think of this as a 2D array that ranges from (-N/2) to (N/2) and from
	(-M/2) to (M/2) with 0 in the middle. The direction from the center of the 2D
	array represents the direction of each individual wave in a given array element.
	The distance from the center represents the wavelength, normalized by the
	maximum wavelength defined by fft-grid-size-x. Each array element contains a
	complex number that represents both the amplitude and phase of the wave defined
	by that element's wavelength and direction. You first need to convert the wave
	amplitude into a Fourier amplitude by dividing the amplitude by two times the
	square root of Pi. The real component of the complex number is given by the
	Fourier amplitude times that cosine of the phase, and the imaginary component is
	the Fourier amplitude times the sine of the phase.

	Once you specify an overridden FFT input array, it will take effect until you either
	submit a new one, or clear it out with Environment::ClearFFTInputArray() to return
	to Triton's own model.

	\param H0 A pointer to an array of Complex FFT inputs as described above. This
	data will be copied internally and you may release this array when done.
	\param size The number of Complex elements in the array provided.
	\return True if the provided array is of the expected size and was accepted;
	otherwise false.
	*/
	bool TRITONAPI OverrideFFTInputArray(const TritComplex *H0, unsigned int size);

	/** Stops using an overridden FFT wave input array as defined in Environment::OverrideFFTInputArray. */
	void TRITONAPI ClearFFTInputArray();

    /** Retrieves the current FFT wave input array as described in Environment::OverrideFFTInputArray. */
    const TritComplex* GetFFTInputArray(unsigned int& size);

    /** Toggle rotor wash decals at run time. */
    void TRITONAPI SetRotorWashUseDecals(bool useDecals);
    bool TRITONAPI GetRotorWashUseDecals(void) const;

    /** Change the water model at run time. 
    * Only works if the underlying water model is an fft water model
    */
    void TRITONAPI SetWaterModel(WaterModelTypes waterModel);

    /** Get the underlying water model. */
    WaterModelTypes TRITONAPI GetWaterModel(void) const;

    // Wrappers for interpreted languages require a publicly declared default constructor. You should always use Ocean::Create, however.
    Ocean() {}

#ifndef SWIG

    // Used internally 
    void RegisterWakeGenerator(WakeGenerator* wg);
    void UnregisterWakeGenerator(WakeGenerator *wg);
    void RegisterRotorWash(RotorWash *rw);
    void UnregisterRotorWash(RotorWash *rw);
    void RegisterTidalStreamWake(TidalStreamWake *tsw);
    void UnregisterTidalStreamWake(TidalStreamWake *tsw);
    void EnvironmentDeleted();
    void NotifySwellsChanged();
    void SetConfused(bool);
    void UpdateHeightMap(void* context, const Camera* camera, TextureHandle heightMap, const Matrix4& heightMapMatrix);
    GodRays *GetGodRays() const { return godRays; }
    const TRITON_VECTOR(unsigned int)& GetUserShaders() const {return userShaders;}
    ProjectedGrid* GetProjectedGrid() const {return grid;}

    typedef void(*GETOCEANHEIGHTPROC)(const Vector3& point, Vector3& normal, float& height);

    void TRITONAPI SetOceanHeightCB(GETOCEANHEIGHTPROC _oceanHeightCB) {
        oceanHeightCB = _oceanHeightCB;
    }

    GETOCEANHEIGHTPROC TRITONAPI GetOceanHeightCB() const {
        return oceanHeightCB;
    }

protected:
    Ocean(Environment *env, WaterModelTypes type, bool enableHeightTest, bool enableBreakingWaves, OceanQuality quality);
    Ocean(Environment *env, WaterModelTypes type, bool enableHeightTest, bool enableBreakingWaves, OceanQuality quality, const TRITON_VECTOR(unsigned int)& userShaders);
    virtual void Initialize(const Environment *env, WaterModelTypes type, bool enableHeightTest, bool enableBreakingWaves);

    virtual bool CreateWaterSimulation();
    void DestroyWaterSimulation();

    void SetWaveHeading(const Camera* camera);

    void DrawInternal(double time, bool depthWrites, bool drawWater, bool drawParticles, void* context, const Camera* camera, bool respectThreadSafe, bool respectAvoidStalls);

    bool isGlobal, initialized, heightReadsEnabled, enableGodRays;

    Environment *environment;

    ProjectedGrid *grid;

    WakeManager *wakeManager;

    SprayManager *sprayManager;

    DecalManager *decalManager;

    GodRays *godRays;

    float waveHeading;

    bool sprayEnabled, enableKelvin;

    double dampingDistance;

    Mutex *mutex;

    bool simulationUpdated;

    WaterModelTypes waterModelType;

    double lastUpdateTime;
    const Camera* lastCamera;

    double timeAdjustment;

    TRITON_VECTOR(WakeGenerator *) wakeGenerators;

    TRITON_VECTOR(RotorWash *) rotorWashes;

    TRITON_VECTOR(TidalStreamWake *) tidalStreams;

    TRITON_VECTOR(unsigned int) userShaders;

    bool hasBreakingWaves, linearEnabled;

    OceanQuality quality;

    GETOCEANHEIGHTPROC oceanHeightCB;

    WaterModelInitializationParams* watermodelparams;

	double cameraMotionThreshold;

    void createAndInitializeFallbackWaterModel(void);
#endif //SWIG
};
}

#pragma pack(pop)

#endif