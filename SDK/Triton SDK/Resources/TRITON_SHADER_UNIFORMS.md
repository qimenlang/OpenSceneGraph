# Triton Shader `trit_` 变量参考

本文档基于仓库内 `SDK/Triton SDK/Resources/` 下的 GLSL 源码整理，说明 Triton 海洋着色器中所有 `trit_` 前缀的 uniform、UBO 块及结构体字段的含义与用途。

**命名约定**：`trit_` 为 Triton 引擎注入的 uniform 前缀，避免与用户或其它引擎变量冲突。由 Triton 运行时（C++）每帧写入，一般无需在应用侧手动设置，除非通过 `Ocean::GetShaderObject()` 自行 `glUniform*`（例如示例中的 `trit_userRotorCenter`）。

**源码入口**：

| 文件 | 用途 |
|------|------|
| `projgrid-flat-common.glsl` | 平面地球（FLAT_ZUP / FLAT_YUP）海洋主 uniform |
| `projgrid-ellipsoid-common.glsl` | 椭球地球（WGS84 / SPHERICAL）海洋主 uniform |
| `particle-common.glsl` | 浪花粒子 |
| `decal-common.glsl` | 水面贴花 |
| `godrays-common.glsl` | 水下上帝光（**无** `trit_` 前缀，见文末） |
| `ocean-frag-flat-fft.glsl` / `ocean-frag-ellipsoid-fft.glsl` | 片段着色器与用户扩展 uniform |

---

## 1. 坐标系：`trit_basis`、`trit_invBasis`、`trit_east`、`trit_north`

Triton 在着色器内部使用一套 **“海面局部坐标”**：**Z 轴为竖直向上（海面法线/高度方向）**，**XY 为水平面**，用于采样 FFT 位移贴图、尾迹、下洗等。

应用侧世界坐标（OSG 等）可能与该约定不一致（例如 Y-up），因此需要变换矩阵/基向量。

### 1.1 `trit_basis`（`mat3`）

- **含义**：将向量从 **世界/应用坐标** 变换到 **Triton 海面局部坐标（Z-up）**。
- **用法**：`vec3 local = trit_basis * worldVec;`
- **典型用途**（`projgrid-flat-fft.glsl`）：
  - 注释 `// Transform so z is up`
  - `localVWorld = trit_basis * vWorld`：得到水平坐标，用于 `texCoords = localVWorld.xy / trit_textureSize` 采样 `trit_displacementMap`
  - `localVLocal.z`：在此坐标系下叠加波高、Kelvin 尾迹、圆形波（旋翼下洗）等
  - 片段着色器：`length((trit_basis * V).xy)` 计算水平距离；`(trit_basis * V).z` 为相对海面高度
- **平面模式与椭球模式均有**（`projgrid-flat-common.glsl`、`projgrid-ellipsoid-common.glsl`）。

### 1.2 `trit_invBasis`（`mat3`）

- **含义**：`trit_basis` 的逆矩阵，从 **海面局部 Z-up** 变回 **世界坐标**。
- **用法**：`vec3 world = trit_invBasis * localVec;`
- **仅平面地球着色器**（`projgrid-flat-common.glsl`）；椭球 common 中 **未声明** `trit_invBasis`。
- **典型用途**：
  - 顶点：`vLocal = trit_invBasis * localVLocal` 写回裁剪空间前的世界顶点
  - 求世界“上”方向：`trit_invBasis * vec3(0,0,1)` 用于与海平面求交
  - 片段：法线、反射、折射在局部 Z-up 中计算后，用 `trit_invBasis` 转回世界空间光照

### 1.3 `trit_east`、`trit_north`（`vec3`，仅椭球模式）

- **含义**：在 **参考点** `trit_referenceLocation` 处、**世界空间** 中的单位方向向量，分别指向 **东** 与 **北**（切平面切向）。
- **仅** `projgrid-ellipsoid-common.glsl`（WGS84_ZUP、SPHERICAL 等）。
- **典型用途**（`projgrid-ellipsoid-fft.glsl`）：
  ```glsl
  // Transform position on the ellipsoid into a planar reference,
  // x east, y north, z up
  vec3 planar = computeArcLengths(localPos.xyz, trit_north, trit_east);
  // planar.x ≈ 东向弧长, planar.y ≈ 北向弧长
  texCoords = planar.xy / trit_textureSize;
  localPos.xyz += disp.x * trit_east + disp.y * trit_north;
  ```
- **与 `trit_basis` 的关系**：
  - **平面模式**：用 `trit_basis` / `trit_invBasis` 在“相机附近切平面”上建立 Z-up 局部系。
  - **椭球模式**：用 `trit_east` / `trit_north` 将椭球面上的点映射到 **东-北-上** 平面坐标，再采样 FFT；高度沿椭球法线（`up = normalize(worldPos)`），不单独使用 `trit_invBasis`。

### 1.4 `trit_northPole`（`vec3`，仅椭球片段着色器）

- **含义**：世界空间中北极方向（椭球 Z 轴方向）。
- **用途**：在 `ocean-frag-ellipsoid-fft.glsl` 中与局部 `up` 叉乘得到 `localEast`，用于高海拔法线混合等。

### 1.5 小结对照

| 变量 | 坐标系 | 模式 | 作用 |
|------|--------|------|------|
| `trit_basis` | 世界 → 海面局部 Z-up | 平面 + 椭球 | 水平面 XY、竖直 Z 分离 |
| `trit_invBasis` | 海面局部 → 世界 | **仅平面** | 顶点/法线/反射回世界 |
| `trit_east` | 世界东向单位向量 | **仅椭球** | 平面坐标 X、位移东向分量 |
| `trit_north` | 世界北向单位向量 | **仅椭球** | 平面坐标 Y、位移北向分量 |
| `trit_northPole` | 世界北极方向 | **仅椭球 FS** | 极点附近辅助切向 |

---

## 2. UBO 块（`USE_UBO` 时）

| 块名 | 内容 |
|------|------|
| `trit_ShaderParameters_UBO` | 矩阵、光照、雾、环境贴图参数等标量/向量 uniform |
| `trit_TextureHandles_UBO` | 采样器（`BINDLESS_TEXTURES` 时） |
| `trit_WakeParameters` | 尾迹/螺旋桨流/圆形波/背风衰减数组与计数 |

非 UBO 路径下，同名变量以普通 `uniform` 声明（见 common 文件 `#else` 分支）。

---

## 3. 海洋着色器 — `trit_ShaderParameters_UBO` 成员

以下按功能分类；**“平面”** 表示 `projgrid-flat-common.glsl`，**“椭球”** 表示 `projgrid-ellipsoid-common.glsl`。未标注则两者共有。

### 3.1 变换与相机

| 变量 | 类型 | 含义 |
|------|------|------|
| `trit_modelMatrix` | `mat4` | 海洋网格模型矩阵 |
| `trit_modelview` | `mat4` | 模型 × 视图 |
| `trit_projection` | `mat4` | 投影矩阵 |
| `trit_invModelviewProj` | `mat4` | 逆 (投影×模型视图)，屏幕射线反投影到世界/海面 |
| `trit_zNearFar` | `vec2` | 近/远裁剪面，用于投影网格与海平面求交 |
| `trit_depthOffset` | `float` | 深度偏移，减轻 z-fighting |
| `trit_cameraPos` | `vec3` / `dvec3` | 相机世界位置（椭球可 `DOUBLE_PRECISION`） |
| `trit_gridScale` | `mat4` | 投影网格顶点缩放 |
| `trit_plane` | `vec4` | 海平面方程（平面模式）；椭球中亦有，与平面高度相关 |
| `trit_bypassOverridePosition` | `bool` | 是否跳过 `overridePosition()` 用户钩子 |

### 3.2 椭球地球专用

| 变量 | 类型 | 含义 |
|------|------|------|
| `trit_radii` | `vec3` | 椭球半轴 (rx, ry, rz) |
| `trit_oneOverRadii` | `vec3` | 半轴倒数，射线-椭球求交 |
| `trit_north` | `vec3` | 参考点处北向（见 §1.3） |
| `trit_east` | `vec3` | 参考点处东向（见 §1.3） |
| `trit_referenceLocation` | `vec3` | 局部原点对应的地球表面参考点（世界坐标） |
| `trit_planarHeight` | `float` | 平面近似高度相关 |
| `trit_planarAdjust` | `float` | 平面近似修正 |

### 3.3 FFT 海面与纹理尺度

| 变量 | 类型 | 含义 |
|------|------|------|
| `trit_textureSize` | `vec2` | FFT 位移/坡度纹理在世界水平面上的覆盖尺度（米），用于 `texCoords = pos.xy / trit_textureSize` |
| `trit_foamScale` | `vec2` | 泡沫纹理 UV 尺度 |
| `trit_time` | `float` | 仿真时间（秒），动画与相位 |
| `trit_seaLevel` | `float` | 海平面高度（世界竖直轴） |
| `trit_invDampingDistance` | `float` | 网格边缘波高衰减距离倒数 |

### 3.4 雾与水深

| 变量 | 类型 | 含义 |
|------|------|------|
| `trit_fogDensity` | `float` | 水上雾密度 |
| `trit_fogDensityBelow` | `float` | 水下雾密度 |
| `trit_fogColor` | `vec3` | 雾颜色 |
| `trit_floorPlanePoint` | `vec3` | 海底/地板平面一点 |
| `trit_floorPlaneNormal` | `vec3` | 地板法线 |
| `trit_hasDepthMap` | `bool` | 是否使用深度图做水下地形 |
| `trit_heightMapRangeOffset` | `vec2` | 高度图缩放/偏移 (scale, offset) |
| `trit_heightMapMatrix` | `mat4` | 世界坐标 → 高度图 UV |
| `trit_hasHeightMap` | `bool` | 是否绑定高度图 |
| `trit_hasUserHeightMap` | `bool` | 是否使用用户高度图回调 |

### 3.5 光照与外观（片段着色器为主）

| 变量 | 类型 | 含义 |
|------|------|------|
| `trit_L` | `vec3` | 主光源方向（世界空间，指向光源） |
| `trit_lightColor` | `vec3` | 直射光颜色 |
| `trit_ambientColor` | `vec3` | 环境光 |
| `trit_refractColor` | `vec3` | 折射/水体底色 |
| `trit_shininess` | `float` | 高光指数 |
| `trit_sunIntensity` | `float` | 太阳强度缩放 |
| `trit_reflectivityScale` | `float` | 菲涅尔反射率缩放 |
| `trit_transparency` | `float` | 水面透明度 |
| `trit_underwater` | `bool` | 相机是否在水下 |
| `trit_doubleRefractionColor` | `vec3` | 双重折射颜色 |
| `trit_doubleRefractionIntensity` | `float` | 双重折射强度 |
| `trit_oneOverGamma` | `float` | Gamma 校正 |
| `trit_depthOnly` | `bool` | 仅写深度 pass |

### 3.6 环境贴图与平面反射

| 变量 | 类型 | 含义 |
|------|------|------|
| `trit_hasEnvMap` | `bool` | 是否使用立方体环境贴图 |
| `trit_cubeMap` | `samplerCube` | 环境立方体贴图 |
| `trit_cubeMapMatrix` | `mat3` | 世界反射方向 → 立方体采样方向（如 Y-up 天空盒对齐） |
| `trit_hasPlanarReflectionMap` | `bool` | 是否使用平面反射贴图 |
| `trit_planarReflectionMap` | `sampler2D` | 平面反射纹理 |
| `trit_planarReflectionMapMatrix` | `mat3` | 反射 UV 变换 |
| `trit_planarReflectionDisplacementScale` | `float` | 反射 UV 法线扰动强度 |
| `trit_planarReflectionBlend` | `float` | 平面反射与环境贴图混合权重 |

### 3.7 噪声与泡沫

| 变量 | 类型 | 含义 |
|------|------|------|
| `trit_noiseAmplitude` | `float` | 法线噪声幅度 |
| `trit_invNoiseDistance` | `float` | 噪声随距离衰减 |
| `trit_foamBlend` | `float` | 泡沫混合强度 |
| `trit_textureLODBias` | `float` | 坡度/泡沫纹理 LOD 偏移 |

### 3.8 背风衰减（`LEEWARD_DAMPENING`）

| 变量 | 类型 | 含义 |
|------|------|------|
| `trit_windDir` | `vec3` | 风向（世界空间） |

### 3.9 碎浪（`BREAKING_WAVES`）

| 变量 | 类型 | 含义 |
|------|------|------|
| `trit_kexp` | `float` | 碎浪陡峭度参数 k |
| `trit_breakerWavelength` | `float` | 碎浪波长 |
| `trit_breakerWavelengthVariance` | `float` | 波长随机方差 |
| `trit_breakerDirection` | `vec3` | 碎浪传播方向 |
| `trit_breakerAmplitude` | `float` | 碎浪振幅 |
| `trit_breakerPhaseConstant` | `float` | 相位常数 |
| `trit_surgeDepth` | `float` | 涌浪区水深（近岸衰减） |
| `trit_steepnessVariance` | `float` | 陡峭度方差 |
| `trit_breakerDepthFalloff` | `float` | 随深度衰减 |
| `trit_breakingWaveMapMatrix` | `mat4` | 碎浪贴图矩阵（`BREAKING_WAVES_MAP`） |
| `trit_hasBreakingWaveMap` | `bool` | 是否有碎浪贴图 |

### 3.10 高海拔（`HIGHALT`）

| 变量 | 类型 | 含义 |
|------|------|------|
| `trit_invZoom` | `float` | 相机缩放倒数，远距法线混合 |

---

## 4. 纹理采样器 — `trit_TextureHandles_UBO`

| 变量 | 含义 |
|------|------|
| `trit_heightMap` | 地形高度图 |
| `trit_depthMap` | 场景深度图（水下地形） |
| `trit_displacementMap` | FFT 位移图（RGB：水平位移 + 波高） |
| `trit_slopeFoamMap` | 坡度与泡沫数据 |
| `trit_displacementTexture` | 附加位移（船体尾迹等） |
| `trit_hullWakeTexture` | 船体尾迹纹理 |
| `trit_breakingWaveMap` | 碎浪贴图（可选） |
| `trit_lightFoamTex` | 亮泡沫纹理 |
| `trit_noiseTex` | 法线噪声 |
| `trit_washTex` | 螺旋桨/尾迹泡沫洗纹理 |
| `trit_breakerTex` | 碎浪纹理 |

---

## 5. 尾迹与下洗 — `trit_WakeParameters`

### 5.1 控制量

| 变量 | 类型 | 含义 |
|------|------|------|
| `trit_doWakes` | `bool` | 是否启用尾迹/下洗计算 |
| `trit_washLength` | `float` | 螺旋桨洗最大长度 |
| `trit_numPropWashes` | `int` | 有效螺旋桨流数量 |
| `trit_numKelvinWakes` | `int` | 有效 Kelvin 尾迹数量 |
| `trit_numCircularWaves` | `int` | 有效圆形波数量（**含 RotorWash 下洗**） |
| `trit_numLeewardDampeners` | `int` | 背风衰减体数量（`LEEWARD_DAMPENING`） |
| `trit_leewardDampeningStrength` | `float` | 背风衰减强度 |

### 5.2 结构体 `trit_CircularWave`（数组 `trit_circularWaves[]`）

| 字段 | 含义 |
|------|------|
| `amplitude` | 波幅 |
| `radius` | 影响半径 |
| `k` | 波数 |
| `halfWavelength` | 半波长 |
| `position` | 中心位置（海面局部/平面坐标，与 `applyCircularWaves` 输入一致） |

由 `Triton::RotorWash` 等 API 填充，用于旋翼下洗等圆形波纹高度偏移。

### 5.3 结构体 `trit_KelvinWake`（数组 `trit_wakes[]`）

| 字段 | 含义 |
|------|------|
| `amplitude` | 尾迹振幅 |
| `position` | 尾迹位置 |
| `shipPosition` | 船位 |
| `foamAmount` | 泡沫量 |
| `hullWakeWidth` | 船体尾迹宽度 |
| `hullWakeLengthReciprocal` | 船体尾迹长度倒数 |

### 5.4 结构体 `trit_PropWash`（数组 `trit_washes[]`，`PROPELLER_WASH`）

| 字段 | 含义 |
|------|------|
| `deltaPos` | 流段方向向量 |
| `washWidth` | 洗宽度 |
| `propPosition` | 螺旋桨/源位置 |
| `distFromSource` | 距源距离 |
| `washLength` | 洗长度 |
| `alphaStart` / `alphaEnd` | 沿流方向透明度起止 |

### 5.5 结构体 `trit_LeewardDampener`（数组 `trit_leewardDampeners[]`）

| 字段 | 含义 |
|------|------|
| `bowPos` | 船首 |
| `sternPos` | 船尾 |
| `velocityDampening` | 背风侧波高衰减系数 |

数组上限由编译宏 `MAX_PROP_WASHES`、`MAX_KELVIN_WAKES`、`MAX_CIRCULAR_WAVES`、`MAX_LEEWARD_DAMPENERS` 决定（在 Triton 构建/配置中注入，非 GLSL 内写死常数）。

---

## 6. 粒子着色器 — `particle-common.glsl`

| 变量 | 含义 |
|------|------|
| `trit_mvProj` | 模型视图 × 投影 |
| `trit_modelview` | 模型视图 |
| `trit_cameraPos` | 相机位置 |
| `trit_refOffset` | 粒子相对参考偏移 |
| `trit_hasHeightMap` / `trit_hasUserHeightMap` | 高度图标志 |
| `trit_heightMapRangeOffset` / `trit_heightMapMatrix` | 高度图采样 |
| `trit_invSizeFactor` | 粒子尺寸倒数 |
| `trit_seaLevel` | 海平面 |
| `trit_fogDensity` / `trit_fogColor` | 雾 |
| `trit_lightColor` | 光色 |
| `trit_transparency` | 透明度（非实例化路径） |
| `trit_time` | 时间 |
| `trit_g` | 重力加速度向量 |
| `trit_heightMap` | 高度图采样器 |
| `trit_particleTexture` | 粒子纹理 |

---

## 7. 贴花着色器 — `decal-common.glsl`

| 变量 | 含义 |
|------|------|
| `trit_modelview` | 模型视图 |
| `trit_inverseView` | 逆视图 |
| `trit_viewport` / `trit_inverseViewport` | 视口与倒数 |
| `trit_projMatrix` / `trit_inverseProjection` | 投影与逆 |
| `trit_up` | 上方向 |
| `trit_depthRange` | 深度范围 |
| `trit_depthOffset` | 深度偏移 |
| `trit_fogDensity` / `trit_fogColor` | 雾 |
| `trit_mvProj` | 当前贴花 MV×P |
| `trit_decalMatrix` | 贴花变换 |
| `trit_alpha` | 透明度 |
| `trit_lightColor` | 光色 |
| `trit_colorUVOffset` | 颜色 UV 偏移 |
| `trit_depthTexture` | 场景深度 |
| `trit_decalTexture` | 贴花纹理 |

---

## 8. 用户/示例扩展 uniform

| 变量 | 文件 | 含义 |
|------|------|------|
| `trit_userRotorCenter` | `ocean-frag-flat-fft.glsl`、`ocean-frag-ellipsoid-fft.glsl` | 旋翼/下洗中心世界坐标；用于涡旋法线等视觉效果，需应用侧 `glUniform3f`（见 `TritonOcean` 示例 `applyUserRotorCenterUniform`） |

> 历史变量 `trit_userRotorRadius`、`trit_userRotorAmplitude`、`trit_userRotorWaveCount`、`trit_userRotorEnabled` 及顶点函数 `user_rotor_displace()` 已从 `projgrid-flat-fft*.glsl` 移除；下洗高度改由 `trit_circularWaves` + `RotorWash` 驱动。

---

## 9. 上帝光着色器（无 `trit_` 前缀）

`godrays-common.glsl` 中 UBO 名为 `trit_ShaderParameters_UBO`，但成员为 **`basis` / `invBasis` / `mvProj` 等**（无 `trit_` 前缀），语义与 `trit_basis` / `trit_invBasis` 类似，用于上帝光体积采样时的局部 Z-up 坐标。

---

## 10. 用户可插拔函数（非 uniform）

定义于 `user-functions.glsl`、`user-vert-functions.glsl`，由海洋着色器 `#include` 调用，例如：

- `user_intercept`、`overridePosition`、`user_get_depth`
- `user_lighting`、`user_fog`、`user_tonemap`、`user_reflection_adjust`
- `writeFragmentData`

这些 **不是** `trit_` uniform，但可读写上述 uniform 与 varying。

---

## 11. 编译宏对 uniform 可见性的影响

| 宏 | 影响 |
|----|------|
| `USE_UBO` | uniform 并入 std140 UBO |
| `BINDLESS_TEXTURES` | 立方体/平面反射等移入 bindless UBO |
| `PROPELLER_WASH` | `trit_washes[]` 等 |
| `LEEWARD_DAMPENING` | `trit_windDir`、背风数组 |
| `BREAKING_WAVES` / `BREAKING_WAVES_MAP` | 碎浪相关 uniform |
| `HIGHALT` | `trit_invZoom` |
| `DOUBLE_PRECISION` | `trit_cameraPos` 为 `dvec3`（椭球） |
| `OCEAN_SHADER` | 在 common 中定义，标识海洋着色器 |

---

## 12. 与 `CoordinateSystem` 的对应关系

| `Environment::Initialize` | 着色器 common | 水平/竖直变换 |
|---------------------------|---------------|----------------|
| `FLAT_ZUP` / `FLAT_YUP` | `projgrid-flat-common.glsl` | `trit_basis` / `trit_invBasis` |
| `WGS84_*` / `SPHERICAL_*` | `projgrid-ellipsoid-common.glsl` | `trit_east` / `trit_north` + `trit_basis` |

`examples/TritonOcean` 使用 **FLAT_ZUP**，因此主要关注 **`trit_basis` / `trit_invBasis`**，一般不出现 `trit_east` / `trit_north`。

---

*文档版本：与当前仓库 `SDK/Triton SDK/Resources` 同步。若升级 Triton SDK，请以 common 文件为准核对。*
