# Fast Realistic Rendering (FRR) Lab Project - 1

## Table of contents
* [General info](#general-info)
* [How to run](#how-to-run)
* [Basic Vizualization](#basic-vizualization) 
* [Physically Based Rendering](#pbr)
* [Image Based Lighting](#ibl) 

## General info
Introductory OpenGL / C++ project in Physically Based Rendering for the FRR course.

## How to run
(Linux only, due to Qt dependencies) Use QtCreator 5, open ViewerPBS23.pro, compile & run.

## Basic Vizualization
Basic vizualization of a sphere at the center of a cubemap.
- Phong lighting on a sphere (colored or textured sphere)
- Cubemap / Skybox
- Reflection

Scene cubemap downscaled   |  Reflection
:-------------------------:|:-------------------------:
![](repo_images/1-skybox-little.png)  |  ![](repo_images/1-reflection.png)

## Physically Based Rendering
Bi-directional Reflectance Distribution Function (BRDF)
- Using the Schlick approximation of the Fresnel factor 
- Using the GGX/Trowbridge-Reitz function as Normal Distribution
- Using the Schlick-GGX model as Geometry Shadowing Function
- Using the Cook-Torrance equation to compute the specular part
- Changing the roughness and metalness (using global values from the UI or via textures/maps)
- (Limitations corrections) HDR Tone Mapping, Gamma Correction

Using global values        |  Using simple pbs material
:-------------------------:|:-------------------------:
![](repo_images/2-pbs-showcase.gif)  |  ![](repo_images/2-simple-pbs-material.png)

## Image Based Lighting
Environment mapping for more physically accurate vizualization.

Diffuse Irradiance:
- Using another Schlick approximation of the Fresnel factor that takes roughness into account
- Using a pre-filtered environment map (given, already convoluted)

Specular Radiance:
- Using OpenGL's built-in Mipmaps to get the different Levels Of Detail (LODs)
- Using the Epic Games' pre-computed BRDF integration map
- (Limitations corrections) HDR Tone Mapping, Gamma Correction

Image based lighting using global values|
:-------------------------:|
![](repo_images/3-ibl-showcase.gif)|
