# A 3D graphics testbed, inspired by Blender

This is a project I built for self education, with the goals being: 

1) Use no libraries and no engines
2) Use software rendering (no GPU) 
3) Use SIMD and multithreading
4) Code as quickly as possible, without thinking too much about design

(click below to see a quick demo)

[![Quick demo](https://img.youtube.com/vi/cqdyQezP9Ck/0.jpg)](https://www.youtube.com/watch?v=cqdyQezP9Ck)

This is done trivially if one uses a math library and a graphics API such as OpenGL, but in this project I did everything the hard way, essentially inventing how to implement all the transforms, how to move the camera around, how to rasterize triangles using SIMD and subpixel precision etc., so it was surprisingly a lot of work and a lot of bug fighting - see the process here http://ivan.ivanovs.info/process.html

The project is on hold. It is possible that I return to it some time in the future, but this time, without the restrictions. It will most likely require a redesign (a concequence of the goal 4 above)
