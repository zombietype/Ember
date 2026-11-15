Ember
======

Ember is a Direct3D 12 based rendering engine used as a learning project and a playground.

**Before we start:**
[Framegraph](./framegraph) is an external project by Dawid Kurek (skaarj1989). 
It has been minimally modified to fix a pass culling bug and set up `setPreExecCallback`. (To be upstreamed).

Screenshots
------

### Simple Scene in Sponza
![Scene in Sponza](Misc/Main.png)

### GPU Driven - Mesh Shading based rendering

Ember uses a single dispatch to amplification shader that handles frustum culling (see note),
meshlet culling and cone-culling.

![Meshlet view of the scene](Misc/Meshlet.png)

Note:  
This was done for learning the capabilities of amplification shaders.  
There are performance issues with culling entire meshlets in the amplification shader, 
as such, this step will be moved to a dedicated compute shader.

### Raytraced and Probe Reflections

Fully raytraced reflections for smooth surfaces with a fallback to probe based reflections (work in progress)
for rougher surfaces.

![Image of DamagedHelmet GLB sample in sponza with raytraced reflections](Misc/Raytraced_Reflection.png)

### Atmospheric Scattering

Implementation of Hillaire, S. (2020), A Scalable and Production Ready Sky and Atmosphere Rendering Technique. Computer Graphics Forum, 39: 13-22. https://doi.org/10.1111/cgf.14050

Currently missing aerial perspective view (as we do not have terrain to test it against).

![Image of an evening sky](Misc/Evening.png)

### Screen Space Ambient Occlusion

A basic hemispherical SSAO is currently integrated in the engine, with GTAO and raytraced AO as the next steps.

![SSAO in Ember](Misc/SSAO.png)

### Other Features

- Dynamic Shadows for all lights:
    - Shadow maps with percent closer filtering (PCF)
    - Raytraced shadows
- Cascaded Shadow Maps:
    - Directional lights support CSM for improved shadow quality
- Deferred Rendering:
    - Ember can use both forward and deferred rendering and switch between them.

### Planned

- Reflection Probe Blending
- Light probes for Precomputed Radiance Transfer.