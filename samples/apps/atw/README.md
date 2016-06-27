
These samples implement the simplest form of time warp transform for virtual reality.
This transform corrects for optical aberration of the lenses used in a virtual
reality headset, and only rotates the stereoscopic images based on the very latest
head orientation to reduce the motion-to-photon delay (or end-to-end latency).

These samples can be used to test whether or not a particular combination of hardware,
operating system and graphics driver is capable of rendering stereoscopic pairs of
images, while asynchronously (and ideally concurrently) warping the latest pair of
images onto the display, synchronized with the display refresh without dropping any
frames. Under high system load, the rendering of the stereoscopic images is allowed
to drop frames, but the asynchronous time warp must be able to warp the latest
stereoscopic images onto the display, synchronized with the display refresh
*without ever* dropping any frames.

source file   |  description
--------------|--------------------------------------------------------
atw_cpu_dsp.c |	 Asynchronous Time Warp implementation for a CPU or DSP
atw_opengl.c  |  Asynchronous Time Warp implementation using OpenGL
atw_vulkan.c  |  Asynchronous Time Warp implementation using Vulkan