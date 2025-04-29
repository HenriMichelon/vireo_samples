# Vireo RHI Samples

Samples for the Vireo Rendering Hardware Interface.

https://github.com/HenriMichelon/vireo_rhi

The samples demonstrate the following features, in order of complexity:

- Triangle, the classic "Hello Triangle" :
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
- Triangle texture:
  - Images
  - Samplers
  - Descriptor layouts
  - Samplers descriptor layouts
  - Descriptor sets
  - Binding descriptor sets
  - Slang example for a texture-filled triangle
- Triangle with texture and buffers :
  - Uniform buffers
  - Push constants
  - Multiple pipelines
  - Slang example using a uniform buffer and push constants
- MSAA :
  - Custom render targets
  - MSAA support
- Compute :
  - Read/Write images
  - Compute pipeline
  - Images copying
  - Slang example for a compute shader
- Cube :
  - Index buffers
  - Indexed drawing
  - Depth buffers
  - Depth pre-pass with semaphore synchronization
  - Cubemaps
  - MSAA for color and depth buffers
  - Slang example for an MVP vertex shader, a skybox and a post-processing effect
