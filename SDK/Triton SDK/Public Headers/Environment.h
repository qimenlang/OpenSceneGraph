// Copyright (c) 2011-2018 Sundog Software LLC. All rights reserved worldwide.

#ifndef TRITON_ENVIRONMENT_H
#define TRITON_ENVIRONMENT_H

/** \file Environment.h
   \brief The public interface for setting Triton's environmental parameters.
 */

// Data for SWIG follows, used for automatically creating our C# wrapper. Move along...
#ifdef SWIG
#define TRITONAPI
#define SL_VECTOR(a) std::vector<a>
%module TritonEnvironment
%include "carrays.i"
%include "typemaps.i"
%include "ResourceLoader.h"
%include "WindFetch.h"
%include "Matrix3.h"
%include "Matrix4.h"
%include "CoordinateSystem.h"
%array_functions( double, double_array )
%typemap(ctype) void * "void *"
%typemap(imtype) void * "System.IntPtr"
%typemap(cstype) void * "System.IntPtr"
%typemap(csin) void * "$csinput"
%typemap(in) void * %{ $1 = $input; %}
%typemap(out) void * %{ $result = $1; %}
%typemap(csout) void * {
     return $imcall;
   }
%typemap(ctype) TextureHandle "void *"
%typemap(imtype) TextureHandle"System.IntPtr"
%typemap(cstype) TextureHandle "System.IntPtr"
%typemap(csin) TextureHandle "$csinput"
%typemap(in) TextureHandle %{ $1 = $input; %}
%typemap(out) TextureHandle %{ $result = $1; %}
%typemap(csout) TextureHandle {
     return $imcall;
   }
SWIG_CSBODY_PROXY(public, public, SWIGTYPE)
SWIG_CSBODY_TYPEWRAPPER(public, public, public, SWIGTYPE)
%{
#include "Environment.h"
using namespace Triton;
%}
#endif

// And now, our actual C++ header:
#include "TritonCommon.h"
#include "ResourceLoader.h"
#include "RandomNumberGenerator.h"
#include "WindFetch.h"
#include "Matrix3.h"
#include "Matrix4.h"
#include "OrientedBoundingBox.h"
#include "CoordinateSystem.h"

#include "TritonMap.h"
#include "TritonVector.h"
#include "TritonSet.h"

#pragma pack(push)
#pragma pack(8)

namespace Triton
{
class Frustum;
class Ocean;
class Camera;
class Mutex;
class Stream;
class BindlessCapsGL;
class OpenGLDevice;

/** Support renderers for the Environment constructor. As newer versions of OpenGL emerge, specifying
an older version in this enumeration should be fine as long as your context is backward-compatible. */
enum Renderer {
    OPENGL_2_0,         /** Targets devices capable of OpenGL 2.0 functionality */
    OPENGL_3_2,         /** OpenGL 3.2, compatible with backward or non-backward-compatible contexts */
    OPENGL_4_0,         /** OpenGL 4.0, compatible with backward or non-backward-compatible contexts */
    OPENGL_4_1,         /** OpenGL 4.1, compatible with backward or non-backward-compatible contexts */
    OPENGL_4_5,         /** OpenGL 4.5, compatible with backward or non-backward-compatible contexts */
    DIRECTX_9,          /** DirectX9 device */
    DIRECT3D9_EX,       /** Direct3D9Ex device */
    DIRECTX_11,         /** DirectX11 device */
    NO_RENDERER         /** No renderer; use for physics only. */
};

/** Error codes returned from Environment::Initialize(). */
enum EnvironmentError {
    SUCCEEDED = 0,          /** Indicates the initialization succeeded. */
    NO_CONFIG_FOUND,        /** The Triton.config file could not be loaded, which likely means you created your ResourceLoader with an invalid path. */
    NULL_RESOURCE_LOADER,   /** A null pointer was passed in for the ResourceLoader object. */
    NO_DEVICE,              /** A null device was passed in, and you're using a DirectX renderer that requires one. */
    NO_CONTEXT              /** No GL context was active when calling this function. */
};

class HeightMapContainer
{
public:
    HeightMapContainer();
    ~HeightMapContainer();
    TextureHandle heightMap;
    Matrix4 heightMapMatrix;
};

/** Parameters to control behavior of breaking waves at shorelines, used by
    Environment::SetBreakingWaves(). */
class BreakingWavesParameters : public MemObject {
public:
    /** The constructor sets reasonable default values, except for the waveDirection member which we can't really guess at. */
    BreakingWavesParameters();

    /** The "k" value controlling the steepness of the waves; 0 is rounded sine wave, 1.0 is pointy. 
        This will automatically increase as the wave approaches the shore; you're just specifying
        the starting value here. Default: 0.5*/
    void SetSteepness(float pSteepness) {
        steepness = pSteepness; ComputeDerivedValues();
    }

    /** The starting wavelength of the breaking waves. Default: 1500.0 */
    void SetWavelength(float pWavelength) {
        wavelength = pWavelength; ComputeDerivedValues();
    }

    /** How much the wavelength varies as the wave approaches the shore. Default: 500.0 */
    void SetWavelengthVariance(float pWavelengthVariance) {
        wavelengthVariance = pWavelengthVariance;
    }

    /** The normalized direction vector pointing toward the shoreline. Used only if 
        SetAutoWaveDirection(false) has been called, or if automatic detection of the
        ocean floor's slope fails for some reason. */
    void SetWaveDirection(const Vector3& pDirection) {
        waveDirection = pDirection;
        waveDirection.Normalize();
    }

    /** Sets whether the direction of the waves should be automatically determined by examining
        the overall slope of the terrain described by the current height map. If false,
        the explicit wave direction set via SetWaveDirection() will be used instead. Defaults
        to off. */
    void SetAutoWaveDirection(bool on) {
        autoWaveDirection = on;
    }

    /** The amplitude of the breaking waves. Default: 3.0 */
    void SetAmplitude(float pAmplitude) {
        amplitude = pAmplitude; ComputeDerivedValues();
    }

    /** The depth at which the wavelength will rapidly expand to simulate surging surf. Default: 8.0 */
    void SetSurgeDepth(float pSurgeDepth) {
        surgeDepth = pSurgeDepth;
    }

    /** The variance in steepness as the wave approaches the shore. Default: 0.5 */
    void SetSteepnessVariance(float pSteepnessVariance) {
        steepnessVariance = pSteepnessVariance;
    }

    /** How quickly breaking waves fade off as a function of water depth. 1.0
        will give you "physically realistic" results, but often not what you expect.
        Larger values will cause breaking waves to fade away closer to shore.
        Default: 5.0 */
    void SetDepthFalloff(float pFalloff) {
        depthFalloff = pFalloff;
    }

    float GetSteepness() const {return steepness;}
    float GetWavelength() const {return wavelength;}
    float GetWavelengthVariance() const {return wavelengthVariance;}
    const Vector3& GetWaveDirection() const {return waveDirection;}
    float GetAmplitude() const {return amplitude;}
    float GetSurgeDepth() const {return surgeDepth;}
    float GetSteepnessVariance() const {return steepnessVariance;}
    float GetPhaseConstant() const {return phaseConstant;}
    float GetExpK() const {return kexp;}
    float GetSpeed() const {return speed;}
    bool GetAutoWaveDirection() const {return autoWaveDirection;}
    float GetDepthFalloff() const {return depthFalloff;}

private:
    float steepness;            
    float wavelength;           
    float wavelengthVariance;   
    Vector3 waveDirection;
    float amplitude;            
    float surgeDepth;           
    float steepnessVariance;    
    void ComputeDerivedValues();
    float phaseConstant, kexp, speed;
    bool autoWaveDirection;
    float depthFalloff;
};

/// A structure containing a description of a swell in addition to local wind waves (from a distant storm perhaps.)
class SwellDescription : public MemObject
{
public:
    float wavelength; /// Wavelength in world units, from peak to peak.
    float height;     /// Wave height, from peak to trough.
    float direction;  /// Direction in radians.
    float phase;      /// Phase offset in radians.
};

// Putting this in here for lack of a better place.
typedef float (*GETUSERHEIGHTPROC)( const Vector3& position );

/** Triton's public interface for specifying the environmental conditions and camera properties. The Ocean
    constructor requires an Environment object, so you'll need to create this first. */
class Environment : public MemObject
{
public:
    /** Constructor. */
    Environment();

    /**
        Initializes the environment prior to use.

        \param cs       The CoordinateSystem your application is using, which may be a geocentric system based
                        on an elliptical WGS84 or spherical earth model, or a flat cartesian model, with
                        up pointing along either the Y or Z axes.
        \param ren      Specifies the version of OpenGL or DirectX your application is using. Triton will
                        render the ocean using the same graphics subsystem.
        \param rl       A ResourceLoader object that is used by Triton for loading its graphics, shader, and
                        configuration resources. This may be an instance of Triton's default ResourceLoader
                        class that loads loose files in Triton's resources directory directly from disk,
                        or your own derived class that handles resource management in some other way.
        \param device   Unused for OpenGL contexts. For DirectX users, this must be a pointer to your
                        valid and initialized DIRECT3DDEVICE9 or ID3D11Device.
		\param hdr		Whether High Dynamic Range rendering is desired, meaning colors will not be clamped to [0.0,1.0].
						If true, HDR lighting values passed in via Environment::SetDirectionalLight(), Environment::SetAmbientLight(),
						and/or via floating point reflection or environment maps will be preserved.
        \return         An error code if the environment failed to initialize, in which case you can't use this environment.
                        To receive more details on why it failed, enable the setting enable-debug-messages in
                        resources/triton.config which will send more info to your debugger's output. If initialization
                        succeeded, you'll get back SUCCEEDED.
    */
    EnvironmentError TRITONAPI Initialize(CoordinateSystem cs, Renderer ren, ResourceLoader *rl, void *device = NULL, bool hdr = false);

    /** Virtual destructor. To ensure proper cleanup, destroy your Environment object after deleting your Ocean object(s). */
    virtual ~Environment();

    /** Licensed users must call SetLicenseCode with your user name and registration code prior to
        using the Environment object. Visit http://www.sundog-soft.com/ to purchase a license. If
        you don't call SetLicenseCode or pass invalid parameters to it, Triton will run in evaluation mode,
        which will terminate your application after five minutes of runtime.

        \param userName         The user name given to you with your license purchase.
        \param registrationCode The registration code given to you with your license purchase.
    */
    void TRITONAPI SetLicenseCode(const char *userName, const char *registrationCode);

#ifndef SWIG
    /** Set a custom RandomNumberGenerator - derived random number generator, to override Triton's default
        use of stdlib's rand() function. This may be useful for ensuring deterministic behavior across
        channels (although a simpler approach may be calling srand() with a consistent seed from your
        application.) If this method is not called, a default random number generator will be used
        automatically.

        \param rng An instance of a class derived from RandomNumberGenerator that will handle all random
                   number generation within Triton.
    */
    void TRITONAPI SetRandomNumberGenerator(RandomNumberGenerator *rng);

    /** Returns either the default RandomNumberGenerator used for all random numbers in Triton, or a custom
        subclass of RandomNumberGenerator that was passed in via Environment::SetRandomNumberGenerator().
        \return The RandomNumberGenerator instance in use by Triton.
    */
    RandomNumberGenerator * TRITONAPI GetRandomNumberGenerator() const {
        return randomNumberGenerator;
    }
#endif // SWIG

    /** Retrieves the ResourceLoader object passed in to the Environment() constructor. */
    ResourceLoader * TRITONAPI GetResourceLoader() const {
        return resourceLoader;
    }

    /** Retrieves the DirectX device pointer passed in to the Environment() constructor, or an internal OpenGL
        device pointer for OpenGL users (clients cannot use this device directly). */
    void * TRITONAPI GetDevice() const {
        return device;
    }

    /** Sets the color and direction of directional light used to light the water, as from the
        sun or moon. Take care that the direction points toward the light source, and not from
        it - invalid coloration of the water will result if this direction is negated.

        \param direction    A normalized vector pointing toward the infinitely distant light source.
        \param color        The RGB color of the light.
    */
    void TRITONAPI SetDirectionalLight(const Vector3& direction, const Vector3& color);

    /** Sets the color of ambient light used to light the water, as from skylight.

        \param color the RGB color of the ambient light.
    */
    void TRITONAPI SetAmbientLight(const Vector3& color) {
        ambientColor = color;
    }

    /** Retrieves the vector toward the infinitely distant directional light source passed in via SetDirectionalLight(). */
    const Vector3& TRITONAPI GetLightDirection() const {
        return lightDirection;
    }

    /** Retrieves the RGB color of the directional light source passed in via SetDirectionalLight(). */
    const Vector3& TRITONAPI GetDirectionalLightColor() const {
        return lightColor;
    }

    /** Retrieves the RGB color of the ambient light passed in via SetAmbientLight(). */
    const Vector3& TRITONAPI GetAmbientLightColor() const {
        return ambientColor;
    }

    /** Passes in an optional environment cube map used for rendering reflections in the water. If unused,
        Triton will instead reflect a constant color based on the ambient light passed in via
        SetAmbientLight(). The caller is responsible for releasing or deleting this resource at shutdown.
        \sa SetEnvironmentMap()

        \param cubeMap  A cube map texture resource, which should be cast to a TextureHandle.
                        Under OpenGL, this must be a GLuint indicating the ID of the GL_TEXTURE_CUBE_MAP
                        returned from glGenTextures. Under DirectX9, this must be a LPDIRECT3DCUBETEXTURE9.
                        Under DirectX11, this must be a ID3D11ShaderResourceView pointer with an underlying
                        ViewDimension of D3D11_SRV_DIMENSION_TEXTURECUBE.

        \param textureMatrix An optional texture matrix used to transform the 3D coordinates used to
                             access the cube map. If your cube map isn't oriented with the same cartesian
                             axes used by your simulation, you can use this parameter to account for any
                             differences in your cube map's coordinate system and your simulation's. When using
                             DirectX, it's likely that you will need to use this matrix to flip the cube map
                             upside-down due to DirectX's left-handed convention. For example, if Y is "up", pass
                             a scaling matrix to scale Y by -1 if reflections seem to be coming from the bottom
                             of the environment map instead of from the top.
    */
    void TRITONAPI SetEnvironmentMap(TextureHandle cubeMap, const Matrix3& textureMatrix = Matrix3::Identity) {
        envMap = cubeMap;
        envMapMatrix = textureMatrix;
    }

    /** Retrieves the environment cube map passed in via SetEnvironmentMap(), which may be a GLuint,
        LPDIRECT3DCUBETEXTURE9, or ID3D11ShaderResourceView* depending on the renderer being used. */
    TextureHandle TRITONAPI GetEnvironmentMap() const {
        return envMap;
    }

    /** Retrieves the texture matrix used to transform the environment map lookups at runtime, which
        was optionally passed in via SetEnvironmentMap(). */
    Matrix3 TRITONAPI GetEnvironmentMapMatrix() const {
        return envMapMatrix;
    }

    /** Optionally sets a height map used by Triton for improved water / shoreline interactions. If a height map is provided
        that includes bathymetry data - that is, it extends below sea level to include the surface of the sea floor - Triton
        can use this to obtain the depth of the water at each point. This is used for transparency effects and breaking waves
        at shorelines, and to prevent Triton from drawing water over the terrain.

        Take care to only call this method when your height map's content changes. Triton must make a system memory copy of it
        for height queries, which impacts performance.

        For accurate water surface height queries near the shore, DirectX9 users must provide a lockable texture (meaning it is
        not created in the default pool, or it is created with render target usage,) in D3DFMT_R32F format. DirectX11 users
        must use format DXGI_FORMAT_R32_FLOAT. OpenGL users may use any floating-point format, with the height in the red or 
        luminance channel, such as GL_LUMINANCE32F_ARB.

        If no height map is provided, you can instead use Triton::Ocean::SetDepth() to specify a uniformly sloping sea floor
        from the current camera location, and the depth buffer will be used to properly sort terrain against the water.
        Another option is to pass a depth texture every frame using Triton::Environment::SetDepthMap().

        \sa Environment::SetBreakingWavesParameters()
        \sa Environment::SetDepthMap()

        \param pHeightMap Under OpenGL, this must be a GLuint indicating the ID of the GL_TEXTURE_2D returned from 
        glGenTextures. Under DirectX9, this must be a LPDIRECT3DTEXTURE9. Under DirectX11, this must be a 
        ID3D11ShaderResourceView pointer with an underlying ViewDimension of D3D11_SRV_DIMENSION_TEXTURE2D. This texture 
        is expected to contain a single 16 or 32-bit-per-component floating-point channel representing the height at each point, 
        in world units. Pass NULL to disable any previously set height map.

        \param worldToTextureCoords A matrix to transform world coordinates to texture coordinates in the height map. Generally
        this is an orthographic matrix looking down at the height field, scaled and translated into texture coordinate space.

        \param context An optional context, depending on the rendering system in question. For example, you can
        render in DX 11 into a command list in a deferred context and execute the command list on
        the immediate context at a later time.

        \param camera An optional camera. Height maps are camera specific. This parameter allows a height map
		to be specified for the camera in question. This is useful when multiple views are at vastly different locations.
		In cases where views don't differ by much (e.g. left, right stereo rendering), you can get away by not specifying
		anything (or 0), in which case you have to ensure that the environment camera (gotten using GetCamera()) matches
		the main view for the split left, right views
    */
    void TRITONAPI SetHeightMap(TextureHandle pHeightMap, const Matrix4& worldToTextureCoords, void* context=0, const Camera* camera=0);

    /** Retrieves the height map passed in via SetHeightMap(), which may be a GLuint,
        LPDIRECT3DTEXTURE9, or ID3D11ShaderResourceView* depending on the renderer being used.

		\param camera An optional camera. Height maps are camera specific. This parameter allows you 
		to retrieve the height map for the camera in question. See SetHeightMap()
 	*/
    TextureHandle TRITONAPI GetHeightMap(const Camera* camera = 0) const;

    /** Retrieves the texture matrix used to transform the height map lookups at runtime, which
    	was passed in via SetHeightMap(). This matrix transforms world coordinates into height map texture coordinates.

		\param camera An optional camera. Height maps are camera specific. This parameter allows you 
		to retrieve the height map matrix for the height map, for the camera in question. See SetHeightMap()
 	*/
    Matrix4 TRITONAPI GetHeightMapMatrix(const Camera* camera = 0) const;

#ifndef SWIG
    /** Sets an optional user-provided callback to obtain height data. */
    void TRITONAPI SetUserHeightCB( GETUSERHEIGHTPROC _userHeightCB ) {
      userHeightCB = _userHeightCB;
    }

    /** Retrieves the function pointer for the user-provided height callback. */
    GETUSERHEIGHTPROC TRITONAPI GetUserHeightCB() const {
      return userHeightCB;
    }
#endif

    /** Sets a height map multiplier.

        For non-floating point textures, a texture call in the shader will return
        a value in the range 0.0 - 1.0. When used in conjunction with SetHeightMapOffset,
        the range can be transformed into something else.  */
    void TRITONAPI SetHeightMapRange( float _range ) {
        heightMapRange = _range;
    }

    /** Sets a height map offset.

        For non-floating point textures, a texture call in the shader will return
        a value in the range 0.0 - 1.0. When used in conjunction with SetHeightMapRange,
        the range can be transformed into something else.  */
    void TRITONAPI SetHeightMapOffset( float _offset ) {
        heightMapOffset = _offset;
    }

    /** Retrieves the height map range. */
    float TRITONAPI GetHeightMapRange() const {
        return heightMapRange;
    }

    /** Retrieves the heightmap offset. */
    float TRITONAPI GetHeightMapOffset() const {
        return heightMapOffset;
    }

    /**
        Optionally sets a depth texture map used for terrain / water blending at coastlines. The preferred approach is to use
        SetHeightMap() instead, but sometimes height maps cannot be generated easily or quickly enough. SetDepthMap()
        provides a simpler alternative, especially if you already have a depth texture available as part of your engine.

        To go this route, you would pass your depth texture in via SetDepthMap() every frame once it contains terrain
        information. Triton will de-project it into world space, and use it to compute how much water lies behind each 
        vertex of the ocean mesh. 
        
        Be sure Triton's ocean itself has not been rendered into the depth map!

        There are a few downsides to this approach, which is why we recommend using SetHeightMap() instead whenever 
        possible:

        - The depth map cannot be used to provide accurate height queries in shallow water; as a result, you may receive
          height queries representing high waves in shallow harbors when using SetDepthMap() together with 
          Triton::Ocean::GetHeight().

        - The depth map must be updated every frame, while height maps may be set once and regenerated only if the viewpoint
          leaves the area it covers.

        - The depths computed using the depth map will be along the line of sight from the camera to the seafloor, instead of
          directly underneath each ocean point. This can lead to the transparency of water at the shoreline varying with the
          view angle, exposing z-fighting only from certain angles. You will probably need to use higher underwater visibility
          settings with SetDepthMap() than you used with SetHeightMap() for smooth blending.

        - The effect is only as good as your depth buffer resolution. If you have a distant far clip plane, and/or a close near
          clip plane, banding may occur.

        - It hasn't been used in the field as much as SetHeightMap(), especially the DirectX implementation. If you discover
          bugs with it, please contact support@sundog-soft.com.

        If a height map has been provided via SetHeightMap(), the depth map will not be used.

        \sa Environment::SetHeightMap()
        \sa Environment::GetDepthMap()

        \param pDepthMap Under OpenGL, this must be a GLuint indicating the ID of the GL_TEXTURE_2D returned from 
        glGenTextures. Under DirectX9, this must be a LPDIRECT3DTEXTURE9. Under DirectX11, this must be a 
        ID3D11ShaderResourceView pointer with an underlying ViewDimension of D3D11_SRV_DIMENSION_TEXTURE2D. This texture 
        is expected to contain a copy of the depth buffer for this frame. Pass NULL to disable any previously set depth map.
    */
    void TRITONAPI SetDepthMap(TextureHandle pDepthMap);

    /** Retrieves the depth map passed in via SetDepthMap(), which may be a GLuint,
        LPDIRECT3DTEXTURE9, or ID3D11ShaderResourceView* depending on the renderer being used. */
    TextureHandle TRITONAPI GetDepthMap() const {
        return depthMap;
    }

    /** Configures the parameters used to simulate breaking waves at shorelines. This will only have an effect if
        you created your Ocean object with the enableBreakingWaves parameter of Ocean::Ocean() set to true, and if
        you have also passed in a height map containing bathymetry information via Environment::SetHeightMap().
        Please see the documentation for Triton::BreakingWavesParameters for a description of the various 
        settings. Generally, you'll be OK just setting the direction of the waves to point toward the nearest shoreline,
        and leave the rest of the settings at their defaults. \sa GetBreakingWavesParmeters() */
    void TRITONAPI SetBreakingWavesParameters(const BreakingWavesParameters& params) {
        breakingWavesParameters = params;
    }

    /** Retrieves the current parameters for breaking waves. \sa Environment::SetBreakingWavesParameters() */
    const BreakingWavesParameters& TRITONAPI GetBreakingWavesParameters() const {
        return breakingWavesParameters;
    }

    /** Passes in an optional planar reflection map used for rendering local reflections in the water.
        Triton can use planar reflection map & environment map together. Alpha channel in planar
        reflection map is used to blend between planar reflection & environment map. If planar reflection
        is not used Triton falls back to environment map (it is the same result as if planar reflection had
        0 on alpha in every texel).

        \sa SetEnvironmentMap()
        \sa Triton::Ocean::SetPlanarReflectionBlend()
        \sa Triton::Ocean::ComputeReflectionMatrices()

        \param textureMap    A 2D map texture resource, which should be cast to a TextureHandle.
                             Under OpenGL, this must be a GLuint indicating the ID of the GL_TEXTURE_2D
                             returned from glGenTextures. Under DirectX9, this must be a LPDIRECT3DTEXTURE9.
                             Under DirectX11, this must be a ID3D11ShaderResourceView pointer with an underlying
                             ViewDimension of D3D11_SRV_DIMENSION_TEXTURE2D.

        \param textureMatrix A required texture matrix used to project the vector computed by triton to reflection map
                             texture coordinates. Generally, this will be the main scene's view rotation matrix * projection matrix,
                             multiplied by a translation of (1, 1, 1) and then by a scale of (0.5, 0.5, 0.5) to transform
                             normalized device coordinates to texture coordinates. Triton's "Input" is a view vector
                             perturbed by normal.xy components. Such a vector approximates wave reflection wiggle and
                             can be used to directly access planar reflection map. See description of parameter
                             normalDisplacementScale to learn why reflection vector cannot be used directly to access
                             the planar reflection map. The input Vector passed to textureMatrix will be defined in
                             world space coordinates translated to the view point. In other words this coordinate space
                             has the same orientation as world space but its origin (point 0,0,0) is moved to the camera
                             location. This is the same coordinate space that env map projection uses. Advanced users may
                             see how reflection (P) variable is handled in Triton pixel (fragment) shaders.

        \param normalDisplacementScale
                             A scale factor used to perturb vertex by normal.xy to get more realistic reflection from rough waves.
                             Range of reasonable values is 0..4. Default is 0.125
                             Realtime bumpy surface reflection approaches are usually based on aproximation of reflection
                             from flat surface. However, method of reflection based on planar projection has serious limitation.
                             It assumes that view vector(and reflection vector) angle strictly corresponds to the incident point
                             on the surface where vector was reflected.
                             If surface is not perfectly flat and water waves are of course an example of such surface,
                             above assumption fails and many points at the ocean surface can reflect vectors at the same direction.
                             If such reflection was used to address a planar map texel we could see reflections of the objects at
                             random points on the surface. To avoid such effect, usually some limits are imposed on reflected vector or
                             reflected texture coords.
                             Triton adopts classic approach to the above problem which works by actually using a view vector
                             perturbed by an offset computed from normal.xy scaled by normalDisplacementScale. Since normal.xy
                             components are never larger than unit value we can be sure that reflection vector will fit in finite
                             margin defined by normalDisplacementScale.
    */

    void TRITONAPI SetPlanarReflectionMap(TextureHandle textureMap,
                                          const Matrix3& textureMatrix,
                                          float normalDisplacementScale = 0.125f) {
        planarReflectionMap = textureMap;
        planarReflectionMapMatrix = textureMatrix;
        planarReflectionDisplacementScale = normalDisplacementScale;
    }

    /** Retrieves the environment cube map passed in via SetPlanarReflectionMap(), which may be a GLuint,
        LPDIRECT3DTEXTURE9, or ID3D11ShaderResourceView* depending on the renderer being used. */
    TextureHandle TRITONAPI GetPlanarReflectionMap() const {
        return planarReflectionMap;
    }

    /** Retrieves the texture matrix used to transform the planar reflection map lookups at runtime, which
        was passed in via SetPlanarReflectionMap(). */
    Matrix3 TRITONAPI GetPlanarReflectionMapMatrix() const {
        return planarReflectionMapMatrix;
    }

    /** Retrieves normal displacement scale set for planar reflections via SetPlanarReflectionMap(). */
    float TRITONAPI GetPlanarReflectionDisplacementScale( ) const {
        return planarReflectionDisplacementScale;
    }

    /** Simulates a specific sea state on the Beaufort scale, by clearing out any existing wind fetches
        passed into the Environment and setting up a new one consistent with the state specified. Any
        subsequent calls to AddWindFetch() will create wind additive to that created for the given sea
        state, so be sure to call ClearWindFetches() if you intend to mix and match calls to SimulateSeaState()
        and AddWindFetch().

        See http://en.wikipedia.org/wiki/Beaufort_scale for detailed descriptions of Beaufort numbers and the
        wave conditions they specify. At a high level,

        0:  Calm
        1:  Light air
        2:  Light breeze
        3:  Gentle breeze
        4:  Moderate breeze
        5:  Fresh breeze
        6:  Strong breeze
        7:  High wind
        8:  Gale
        9:  Storm
        10: Strong Storm
        11: Violent Storm
        12: Hurricane

        \param beaufortScale    The Beaufort scale number specifying the desired wind and sea conditions. This may be a floating
                                point value.
        \param windDirection    The direction of the wind, specified in radians.
        \param leftHanded If you are using a left-handed coordinate system (for example, Y is "up" but positive Z is "North"),
                          pass true in order to ensure your wave direction is represented correctly.
    */
    void TRITONAPI SimulateSeaState(double beaufortScale, double windDirection, bool leftHanded = false);

    /** Adds a Triton::WindFetch to the environment, which specifies an area of wind of a given speed and direction, which may or
        may not be localized. This WindFetch will be added to any other wind passed in previously, unless ClearWindFetches() is
        called first.

        All waves in Triton are a result of simulated wind conditions. Without wind, there will be no waves. Stronger winds
        traveling across longer distances will result in higher waves.

        \param fetch The WindFetch to add to the simulated environment.
        \param leftHanded If you are using a left-handed coordinate system (for example, Y is "up" but positive Z is "North"),
                          pass true in order to ensure your wave direction is represented correctly.
    */
    void TRITONAPI AddWindFetch(const WindFetch& fetch, bool leftHanded = false);

    /** Removes all wind fetches or sea state simulations from the simulated environment, resulting in a perfectly calm sea.
        Call AddWindFetch() or SimulateSeaState() to add wind back in and generate waves as a result. */
    void TRITONAPI ClearWindFetches();

    /** Adds a swell wave to the ocean conditions in addition to the local wind waves. Swells may sometimes originate from distant
        storms and not have anything to do with the local conditions. You may add as many swells as you wish; they incur no
        run-time performance impact. \sa ClearSwells()

        \param waveLength The wavelength of the swell wave, from peak to peak, in world units. Swells are generally around 100-200 m.
        \param waveHeight The swell wave height, from peak to trough, in world units.
        \param direction The direction of the swell wave, in radians. This could be different from the local wind direction.
        \param phase The phase offset of the swell wave, in radians.
        \param leftHanded If you are using a left-handed coordinate system (for example, Y is "up" but positive Z is "North"),
                          pass true in order to ensure your wave direction is represented correctly.
    */
    void TRITONAPI AddSwell(float waveLength, float waveHeight, float direction, float phase = 0, bool leftHanded = false);

    /** Clears any swells previously added via AddSwell(). \sa AddSwell() */
    void TRITONAPI ClearSwells();

#ifndef SWIG
    /** Retrieves the list of swells added via AddSwell() following startup or the last call to ClearSwells(). \sa AddSwell(). */
    const TRITON_VECTOR(SwellDescription)& TRITONAPI GetSwells() const;
#endif

    /** Simulate conditions as described by the Douglas sea scale (http://en.wikipedia.org/wiki/Douglas_Sea_Scale). This will clear out
        any previously set WindFetches, Beaufort scale setting from SimulateSeaState(), and swells created with AddSwell(). 

        \param seaState A value from 0 to 9, 0 describing "calm and glassy" and 9 describing "phenomenal" conditions.
        \param windWaveDirection The direction of wind waves in radians.
        \param swellState A value from 0 to 9 describing high-wavelength swell waves, 0 describing no swell and 9 describing "confused" seas.
        \param swellDirection The direction of the swell waves in radians.
        \param leftHanded If you are using a left-handed coordinate system (for example, Y is "up" but positive Z is "North"),
                          pass true in order to ensure your wave direction is represented correctly.
    */
    void TRITONAPI SetDouglasSeaScale(int seaState, float windWaveDirection, int swellState, float swellDirection, bool leftHanded = false);

    /** Computes the wind conditions at a given location, by evaluating all WindFetch objects that include the position.
        \sa AddWindFetch()
        \sa SimulateSeaState()

        \param pos              A const reference to the position at which wind conditions should be computed.
        \param windSpeed        A reference to a double that will receive the overall wind speed at this location,
                                in world units per second.
        \param windDirection    A reference to a double that will receive the overall wind direction at this location,
                                in radians.
        \param fetchLength      A reference to a double that will receive the fetch length of the farthest WindFetch affecting
                                this location. May return 0 if the local WindFetches have no fetch length specified
                                via WindFetch::SetLocalization() or WindFetch::SetFetchLength().
    */
    void TRITONAPI GetWind(const Vector3& pos, double& windSpeed, double& windDirection, double& fetchLength) const;

    /** If you want to change the mean sea level from a height of 0 in flat-earth coordinates, or from the WGS84 ellipsoid
        in geocentric coordinates, you may do so here.
        \sa GetSeaLevel()

        \param altitudeMSL  The offset in world units to displace mean sea level by.
    */
    void TRITONAPI SetSeaLevel(double altitudeMSL) {
        seaLevel = altitudeMSL;
    }

    /** Returns the offset for the mean sea level previously set by SetSeaLevel().
        \return The offset in world units that the mean sea level is displaced by.
    */
    double TRITONAPI GetSeaLevel() const {
        return seaLevel;
    }

    /** Sets the simulated atmospheric visibility above the water, used to fog out
        the surface of the water when viewed from above. You may use this method to
        fog the ocean consistently with other objects in your scene.

        The visibility specified will be transformed into an exponential fog extinction
        value using the Koschmieder equation: visibility = 3.912 / extinction

        \param visibility   The visibility, in world units, above the water.
        \param fogColor     The fog color, in normalized RGB units.
    */
    void TRITONAPI SetAboveWaterVisibility(double visibility, const Vector3& fogColor) {
        aboveWaterVisibility = visibility;
        aboveWaterFogColor = fogColor;
    }

    /** Retrieves the above-water visibility settings previously set with SetAboveWaterVisibility().
        \param visibility   Receives the visibility, in world units, above the water.
        \param fogColor     Receives the fog color, in normalized RGB units.
    */
    void TRITONAPI GetAboveWaterVisibility(double& visibility, Vector3& fogColor) const {
        visibility = aboveWaterVisibility;
        fogColor = aboveWaterFogColor;
    }

    /** Sets the simulated atmospheric visibility below the water, used to fog out
        the surface of the water when viewed from below. You may use this method to
        fog the ocean consistently with other objects in your scene. While underwater,
        the application is responsible for clearing the back buffer to the fog color
        to match the color passed in here.

        The visibility specified will be transformed into an exponential fog extinction
        value using the Koschmieder equation: visibility = 3.912 / extinction

        \param visibility   The visibility, in world units, below the water.
        \param fogColor     The fog color, in normalized RGB units.
    */
    void TRITONAPI SetBelowWaterVisibility(double visibility, const Vector3& fogColor) {
        belowWaterVisibility = visibility;
        belowWaterFogColor = fogColor;
    }

    /** Retrieves the below-water visibility settings previously set with SetBelowWaterVisibility().
        \param visibility   Receives the visibility, in world units, above the water.
        \param fogColor     Receives the fog color, in normalized RGB units.
    */
    void TRITONAPI GetBelowWaterVisibility(double& visibility, Vector3& fogColor) const {
        visibility = belowWaterVisibility;
        fogColor = belowWaterFogColor;
    }

    /** Sets the ocean depth at which wave heights will start to become dampened, and the water
        will start becoming transparent, for smooth coastline blending. Calling this is optional;
        if not called, the visibility distance specified in Environmnet::SetBelowWaterVisibility()
        will be used instead.

        \param depth    The water depth, in world units, at which coastal blending begins.
    */
    void TRITONAPI SetWaveBlendDepth(double depth) {
        blendDepth = depth;
        hasBlendDepth = true;
    }

    /** Retrieves the ocean depth at which wave heights will start to become dampened, and the
        water will start becoming transparent, for smooth coastline blending. If 
        Environment::SetWaveBlendDepth() has been called, then that value will be returned.
        Otherwise, the below water visiblity set via Environment::SetBelowWaterVisibility()
        will be returned instead. */
    double TRITONAPI GetWaveBlendDepth() const {
        if (hasBlendDepth) {
            return blendDepth;
        } else {
            return belowWaterVisibility;
        }
    }

    /** Sets the intensity of the sunlight visible at the ocean surface; used to modulate the
        specular highlights of the sun on the water surface. Normally this is 1.0, but you 
        might want to decrease it for example if the sun is obscured by clouds. */
    void TRITONAPI SetSunIntensity(float intensity) {
        sunIntensity = intensity;
    }

    /** Retrieves the intensity of the transmitted direct sunlight on the water surface, as set
        with SetSunIntensity(). */
    float TRITONAPI GetSunIntensity() const {
        return sunIntensity;
    }

    /** Sets the size of one world unit in meters. By default, one world unit is assumed to mean one meter. If this is
        not the case for your coordinate system, be sure to call SetWorldUnits() immediately after instantiating your
        Environment class. */
    void TRITONAPI SetWorldUnits(double worldUnits) {
        units = worldUnits;
    }

    /** Retrieves the size of one world unit, in meters. \sa SetWorldUnits() */
    double TRITONAPI GetWorldUnits() const {
        return units;
    }

    /** Returns the CoordinateSystem passed into the Environment() constructor, indicating the up vector and
        the presence of a geocentric or flat coordinate system. */
    CoordinateSystem TRITONAPI GetCoordinateSystem() const {
        return coordinateSystem;
    }

    /** Returns whether the CoordinateSystem passed into the Environment() constructor is geocentric,
        indicating an elliptical or spherical coordinate system where all points are relative to the center
        of the Earth. */
    bool TRITONAPI IsGeocentric() const {
        return coordinateSystem < FLAT_ZUP;
    }

    /** Returns the Renderer specified in the Environment() constructor, telling you what flavor of OpenGL or
        DirectX is being used to render Triton's water. */
    Renderer TRITONAPI GetRenderer() const {
        return renderer;
    }

    /** Returns whether the Renderer specified in the Envrionment() constructor is an OpenGL renderer. */
    bool TRITONAPI IsOpenGL() const {
        return renderer < DIRECTX_9;
    }

    /** Returns whether the Renderer specified in the Environment() constructor is a DirectX renderer. */
    bool TRITONAPI IsDirectX() const {
        return renderer >= DIRECTX_9 && renderer != NO_RENDERER;
    }

    /** Sets the modelview matrix used for rendering the ocean; this must be called every frame prior to calling
        Ocean::Draw() if your camera orientation or position changes.

		*Deprecated* See CreateCamera(), DestroyCamera(), GetCameras()
	
        \param m A pointer to 16 doubles representing a 4x4 modelview matrix.
        \param explicitCameraPosition In flat coordinate systems, this parameter can be used to "fool"
        Triton into using a camera position that is different from the one embedded in the view matrix
        provided in the "m" parameter. This can be useful if you need to center the high-resolution ocean
        geometry someplace other than the camera position, for example in very tight zooms on very distant
        locations. In normal situations, you'll want to just leave this set to NULL. This trick doesn't
        work in WGS84 systems as its projected grid LOD scheme is independent of the camera position.
        When used, this parameter should point to 3 doubles representing the camera position's XYZ coordinates.
    */
    void TRITONAPI SetCameraMatrix(const double *m, const double *explicitCameraPosition = 0);

    /** Sets the projection matrix used for rendering the ocean; this must be called every frame prior to calling
        Ocean::Draw().

		*Deprecated* See CreateCamera(), DestroyCamera(), GetCameras()

        \param p A pointer to 16 doubles representing a 4x4 projection matrix.
    */
    void TRITONAPI SetProjectionMatrix(const double *p);

    /** Informs Triton of the current viewport position and size. Calling this is optional, but allows Triton to avoid
        querying OpenGL or DirectX for the current viewport parameters, which can cause a pipeline stall. If you call
        this method, you are responsible for calling it whenever the viewport changes.

		*Deprecated* See CreateCamera(), DestroyCamera(), GetCameras()
        
        \param x The x position of the current viewport origin.
        \param y The y position of the current viewport origin.
        \param width The width of the current viewport.
        \param height The height of the current viewport.
    */
    void TRITONAPI SetViewport(int x, int y, int width, int height);

    /** If your camera is being zoomed from its typical field of view, use this method to let Triton know about the
        zoom factor. This will automatically adjust things like LOD switch distances, noise blending distances, etc.
        to ensure the ocean looks as you would expect when zoomed in. This method does NOT modify the projection
        matrix you passed in with SetProjectionMatrix(), so it will NOT actually change the field of view of the
        ocean rendering - that's up to you when constructing the projection matrix. 

        \param zoom The zoom level of the current camera, if any. 1.0 represents no zoom, 10.0 would represent 10X.
    */
    void TRITONAPI SetZoomLevel(float zoom) { zoomLevel = zoom; }

    /** Retrieves any zoom level previously set with SetZoomLevel(), or 1.0 if default.

        \return The zoom level specified by Environment::SetZoomLevel() if any, 1.0 otherwise.
    */
    float TRITONAPI GetZoomLevel() const { return zoomLevel; }

    /** Sets a user defined string to be prepended to all vertex shaders.
     */
    void TRITONAPI SetUserDefinedVertString( const char *userDefinedString );

    /** Sets a user defined string to be prepended to all fragment shaders.
     */
    void TRITONAPI SetUserDefinedFragString( const char *userDefinedString );

    /** Retrieves the user defined vertex string previously set with SetUserDefinedVertString.
     */
    const char * TRITONAPI GetUserDefinedVertString() const { return userDefinedVertString; }

    /** Retrieves the user defined fragment string previously set with SetUserDefinedFragString.
     */
    const char * TRITONAPI GetUserDefinedFragString() const { return userDefinedFragString; }

    /** Retrieves any viewport information previously set via SetViewport(). If SetViewport() has not been called, this
        method will return false and return zeros for all parameters. 

        *Deprecated* See CreateCamera(), DestroyCamera(), GetCameras()
        
        \param x The x position of the current viewport origin.
        \param y The y position of the current viewport origin.
        \param width The width of the current viewport.
        \param height The height of the current viewport.
        
        \return true if SetViewport was previously called, and valid information is returned.
    */
    bool TRITONAPI GetViewport(int& x, int& y, int& width, int& height) const;

	/** Deprecated method to get Triton Camera. See CreateCamera(), DestroyCamera(), GetCameras()
	*/
    const Camera* TRITONAPI GetCamera() const { return camera; };

	/** Deprecated method to get Triton Camera. See CreateCamera(), DestroyCamera(), GetCameras()
	*/
    Camera* TRITONAPI GetCamera() { return camera; };

	/** Create a camera
	*/
    Camera* TRITONAPI CreateCamera(void);

	/** Destroy a camera
	*/
    void TRITONAPI DestroyCamera(Camera* camera);

#ifndef SWIG
	/** Get all cameras
	*/
    const TRITON_SET(Camera*)& GetCameras(void) const;
#endif

    /** Retrieves an array of 16 doubles representing the modelview matrix passed in via SetCameraMatrix(). 

		*Deprecated* See CreateCamera(), DestroyCamera(), GetCameras()
	*/
    const double * TRITONAPI GetCameraMatrix() const;

    /** Retrieves an array of 16 doubles representing the projection matrix passed in via SetProjectionMatrix(). 
		
		*Deprecated* See CreateCamera(), DestroyCamera(), GetCameras()
	*/
    const double * TRITONAPI GetProjectionMatrix() const;

    /** Retrieves an array of 3 doubles representing the X, Y, and Z position of the camera, extracted from the
        modelview matrix passed in via SetCameraMatrix(). 
	
		*Deprecated* See CreateCamera(), DestroyCamera(), GetCameras()
	*/
    const double * TRITONAPI GetCameraPosition() const;

    /** Retrieves a normalized vector pointing "up", based on the coordinate system specified in
        Environment::Initialize() and the current position from the modelview matrix passed in through
        Environment::SetCameraMatrix().
	
		*Deprecated* See CreateCamera(), DestroyCamera(), GetCameras()
	*/
    Vector3 TRITONAPI GetUpVector() const;

    /** Retrieves a normalized vector pointing "right", based on the coordinate system specified in
        Environment::Initialize() and the current position from the modelview matrix passed in through
        Environment::SetCameraMatrix().

		*Deprecated* See CreateCamera(), DestroyCamera(), GetCameras()
	*/
    Vector3 TRITONAPI GetRightVector() const;

    /** Sets a configuration setting (defaults in resources/triton.config.) Many settings are read at initialization,
        so call this as early as possible after initializing the Environment.
        \param key  The configuration entry name to modify
        \param value The value to set this entry to.
    */
    void TRITONAPI SetConfigOption(const char *key, const char *value);

    /** Retrieves the configuration setting for the given configuration key, as set in resources/triton.config or via
        SetConfigOption().
        \param key The configuration key to retrieve
        \return The value of the key specified, or NULL if not found.
    */
    const char * TRITONAPI GetConfigOption(const char *key);

    /** Returns true if the given sphere lies within the view frustum, as defined by the modelview - projection
        matrix passed in via SetCameraMatrix() and SetProjectionMatrix().
        \param position The center of the sphere in world coordinates.
        \param radius The radius of the sphere in world coordinates.
        \return True if the sphere is not visible and should be culled.
    */
    bool TRITONAPI CullSphere(const Vector3& position, double radius) const;

    bool TRITONAPI CullOrientedBoundingBox(const OrientedBoundingBox& obb) const;

	/** Retrieves whether HDR mode is enabled, indicating whether color values are clamped to [0,1.0], as set in Environment::Initialize() */
	const bool TRITONAPI GetHDREnabled() const {
		return hdr;
	}

    /** Sets Triton's usage of OpenMP to enable parallel proccessing of CPU-intensive tasks across multiple
        CPU cores. You might want to disable this if you are more concerned about limiting CPU usage than 
        maintaining the fastest possible ocean rendering. OpenMP is enabled by default. Note, if the IPP
        FFT implementation is being used instead of CUDA or OpenCL, multi-core usage will happen regardless 
        of this setting.*/
    void TRITONAPI EnableOpenMP(bool enabled) {
        useOpenMP = enabled;
    }

    /** Retrieves whether OpenMP has been enabled to take advantage of multi-core CPU's. 
        \sa EnableOpenMP() 
    */
    bool TRITONAPI GetOpenMPEnabled() const {
        return useOpenMP;
    }

    /** Gets the estimated maximum wave height in meters at the camera position, given the current wind and 
        swell conditions. */
    float TRITONAPI GetMaximumWaveHeight() const;

    /** Sets a simulated current in the given direction at the given speed. The effects of this
        current will probably expand in future releases, but at present it only causes propeller
        backwash effects on ship wakes to drift. To set the direction of the waves, specify a
        wind value instead, or set up swell waves in the direction you want.
        \param direction        A unit vector pointing in the direction you want the current to
                                run. This must be parallel to the water surface, and is specified
                                in world space.
        \param speed            The speed of the current in meters per second.
    */
    void TRITONAPI SetCurrent(const Vector3& direction, double speed) { currentDirection = direction; currentSpeed = speed; }

    /** Retrieves the current direction and speed previously set with SetCurrent(). */
    void TRITONAPI GetCurrent(Vector3& direction, double& speed) const { direction = currentDirection, speed = currentSpeed; }

    /** Sets an explicit transparency on the water surface. This is in addition to any transparency resulting from shallow water.
    \param t The transparency of the water; 0 = transparent; 1 = opaque. */
    void TRITONAPI SetWaterTransparency(float t) {
        transparency = t;
    }

    /** Gets the explicit transparency of the water set via SetWaterTransparency(). */
    float TRITONAPI GetWaterTransparency() const {
        return transparency;
    }

    /** Create an OpenGL stream*/
    Stream* TRITONAPI CreateOpenGLStream(void);

    /** Destroy an OpenGL stream*/
    void    TRITONAPI DestroyOpenGLStream(Stream* stream);

    /** Execute an OpenGL stream*/
    void    TRITONAPI ExecuteOpenGLStream(Stream* stream);

    /** Reset an OpenGL stream*/
    void    TRITONAPI ResetOpenGLStream(Stream* stream);

#ifndef SWIG

    // Internal use only:
    void RegisterOcean(Ocean *ocean);
    void UnregisterOcean(Ocean *ocean);
    bool GetConfusedSeas() const {return confusedSeas;}
    void TRITONAPI SetBreakingWaveMap(TextureHandle pBreakingWaveMap, const Matrix4& worldToTextureCoords);
    TextureHandle TRITONAPI GetBreakingWaveMap() const {
        return breakingWaveMap;
    }
    Matrix4 TRITONAPI GetBreakingWaveMapMatrix() const {
        return breakingWaveMapMatrix;
    }

    void GetUpRightInVectors(const Vector3& position, Vector3& up, Vector3& right, Vector3& in) const;

    TextureHandle GetHeightMapAndMatrix(Matrix4& heightMapMatrix, const Camera* camera) const;

    /** In OpenGL, get the capabilities of the device; used only internally */
    const BindlessCapsGL* GetBindlessCaps(void) const;

    /** In OpenGL, get the OpenGL device; used only internally */
    const OpenGLDevice* GetOpenGLDevice(void) const;
protected:

    void TRITONAPI DestroyAllCameras(void);

    float adjustDirectionForLeftHanded(float radians);

    CoordinateSystem coordinateSystem;

    void *device;

    Renderer renderer;

    ResourceLoader *resourceLoader;

    RandomNumberGenerator *randomNumberGenerator, *defaultRandomNumberGenerator;

    TRITON_VECTOR(WindFetch) windFetches;

    TRITON_VECTOR(Ocean *) oceans;

    TRITON_VECTOR(SwellDescription) swells;

    bool confusedSeas;

    Camera* camera;

    TRITON_SET(Camera*) cameras;

    double units;

    Vector3 lightDirection, lightColor, ambientColor;

    double seaLevel;

	bool hdr, useOpenMP;

    double aboveWaterVisibility, belowWaterVisibility;

    float sunIntensity;

    Vector3 aboveWaterFogColor, belowWaterFogColor;

    TextureHandle  envMap, breakingWaveMap, depthMap;

    GETUSERHEIGHTPROC userHeightCB;

    float heightMapRange, heightMapOffset;

    Matrix3 envMapMatrix;
    
    Matrix4 breakingWaveMapMatrix;

    typedef TRITON_MAP(const Camera*, HeightMapContainer) MapCameraToHeightMapConfig;
    MapCameraToHeightMapConfig mapCameraToHeightMapConfig;
    Mutex* heightMapMutex;

    TextureHandle planarReflectionMap;

    Matrix3 planarReflectionMapMatrix;

    float  planarReflectionDisplacementScale;

    Frustum *frustum;

    BreakingWavesParameters breakingWavesParameters;

    bool doHeightMapCopy;

    float douglasShortWL, douglasAvgWL, douglasLongWL;
    float douglasLowHeight, douglasModerateHeight, douglasHighHeight;

    float zoomLevel;

    char *userDefinedVertString;
    char *userDefinedFragString;

    double blendDepth;
    bool hasBlendDepth;

    Vector3 currentDirection;
    double currentSpeed;

    float transparency;

    TRITON_SET(Stream*) streams;
    void DestroyAllOpenGLStreams();
    void DestroyOpenGLDevice();
#endif // SWIG

};
}

#pragma pack(pop)

#endif
