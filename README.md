# Vireo RHI Samples

Samples for the Vireo Rendering Hardware Interface.

https://github.com/HenriMichelon/vireo_rhi

The samples demonstrate the following features, in order of complexity:

- Triangle, the classic "Hello Triangle" with an RGB gradient:
  - Submit queues
  - Swap chains
  - Vertex buffers
  - Command allocators
  - Command lists
  - Graphic pipelines
  - Pipeline resources
  - Vertex layouts
  - Shader modules
  - Image barriers
  - Fences
  - Rendering session
  - Rendering commands : set viewports/scissors, bind pipeline, bind vertex buffer, draw
  - Slang example for a vertex-colored triangle
- Triangle texture, the "Hello Triangle" with a texture:
  - Images
  - Samplers
  - Descriptor layouts
  - Samplers descriptor layouts
  - Descriptor sets
  - Binding descriptor sets
  - Slang example for a texture-filled triangle
- Triangle with texture and buffers, more triangles with transparency and a shader-based material :
  - Uniform buffers
  - Color blending for transparency
  - Push constants
  - Multiple pipelines
  - Slang example using a uniform buffer and push constants
- MSAA, the anti-aliased "Hello Triangle" :
  - Custom render targets
  - MSAA support
- Compute, a nice wave effect using a compute shader :
  - Read/Write images
  - Compute pipeline
  - Images copying
  - Slang example for a compute shader
- Cube, two textured rotating cubes with a camera, a light and a sky :
  - Index buffer and indexed drawing
  - Depth buffer and depth pre-pass
  - Forward rendering with one color pass
  - Cubemap and skybox
  - Semaphore synchronization
  - Dynamic uniform buffers for models & materials data
  - Post-processing examples for SMAA, FXAA, gamma correction, and a voronoi effect
  - Slang examples for an MVP vertex shader, a Phong fragment shader, a skybox, and the post-processing effects
- Deferred, same as Cube with :
  - Deferred rendering (Gbuffers, deferred lighting and weighted, blended order-independent transparency)
  - Stencil buffer to reduce skybox and gbuffers workload
  - Push constants used in replacement of dynamic uniform buffers
  - Slang examples for the gbuffers pass, the lighting pass, the order-independent transparency pass.