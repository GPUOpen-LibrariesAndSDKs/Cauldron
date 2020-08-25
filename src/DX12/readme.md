The DX12 backend
================

Here lives all the DX12 specific code. It should be as similar as possible to the VK directory.

### Directory structure

* **base**: All the basic code to manage and load textures/buffers/command buffers
* **GLTF**: Code to render the scene using different techniques
    * GLTFBBoxPass: Renders just the bounding boxes of the objects
    * GLTFBDepthPass: Creates optimized geometry, descriptor sets and pipelines for a depth pass. It also renders the pass and supports skinning.
    * GLTFPbrPass: Same as above but for the forward PBR pass. (Credits go to the glTF-WebGL-PBR github project for its fantastic PBR shader.)
    * GLTFMotionVectorPass: Generates a small gbuffer with depth, normals and motion vectors. This is needed for the TAA pass.
    * TexturesAndBuffers: It loads the texture and buffers from disk and uploads into video memory it also uploads the skinning matrices buffer.
* **PostProc**
    * PostProcCS/PostProcPS: Takes a shader and some inputs and draws a full screen quad (used by the effects below)
    * Bloom: Combines the DownsamplePS and the Blur passes to create a Bloom effect. (All the credits go to Jorge Jimenez!)
    * BlurPS: Takes a mip chain and blurs every mip element
    * BakeSkinning: Takes a vertex buffer and the skinning matrices and computes the skinned vertices into a UAV
    * DownSamplePS: Takes a render target and generates a mip chain  
    * ShadowResolvePass: Takes an atlas of 4 shadowmaps and creates a shadow mask
    * Sharpen: Sharpens the render target, needed after applying TAA.
    * SkyDome: Loads a diffuse+LUT and a specular skydome cubemap. This is used for Image Based Lighting (IBL) and also for drawing a skydome.
    * SkydomeProc: Draws a skydome procedurally using the Preetham Model. (Credits go to Simon Wallner and Martin Upitis.) TODO: Generate a diffuse cubemap for IBL.
    * TAA: Applies TAA, needs motion vectors, depth and normals.
    * ToneMapping: Implements a number of tonemapping methods.
* **Shaders**
    * All the shaders used by the above classes
* **Widgets**
    * Axis: Renders an axis
    * WireFrameBox: Renders a box in wireframe
    * WireFrameSphere: Renders a sphere in wireframe
    * CheckerBoardFloor: As stated.

Note: All the code that may be common with VK is in [src/common](src/common)
