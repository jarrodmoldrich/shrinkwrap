# shrinkwrap![shrinkwrap logo](shrinkwraplogo.png "shrinkwrap")                                                                       

`shrinkwrap` is a tool for creating triangle meshes from the alpha mask of a sprite sheet.  The final geometry is economised to have as few polygons as possible and can be either a composite of two meshes for partial alpha and full alpha, or the full alpha separately.

Using more complex geometry to reduce on unnecessary blending has significant performance benefits on older GPUs, iPhone4 and earlier iDevices in particular.  The gains will be mostly apparent for games that use semi-transparent 2D sprites with large fully-transparent and/or fully opaque regions where the game is GPU-bound, and in particular bottlenecked by pixel-blending.  You can determine whether your game will benefit by turning alpha blending off for all your sprites and comparing the average duration of a frame.

This project is unfinished, but prematurely exposed to github to show the work in progress.

# Explanation

A shrink wrap entails processing a bitmap and creating two non-overlapping geometries that represent the full and partial alpha sections respectively. By making a geometry that has fully opaque areas separated, alpha blending can be avoided for those pixels.  By reducing the partial-alpha section to exclude fully transparent sections, alpha blending can be further reduced.

Reducing alpha blending has GPU performance ramifications on older Apple mobile devices.  The PowerVR chipset in particular has high throughput for polygons but large penalties for pixel blending.  Pixel blending also loses the benefit of occlusion culling gained from its tile-based renderer.  The performance gains should also translate to GPUs that benefit from reduced blending and increased z-occlusion on other platforms.

`shrinkwrap` in its current form takes one png file and one xml-formatted sprite sheet as an input.  To see the format of the sprite sheet, refer to `data/test.xml`.  The output is either an amalgamation of several CSV tables in text, or of two tables in binary format.  The format is explained later in the document.
                                                                          
The basic procedure for `shrinkwrap` goes thusly:
   
1. Reclassify sprite pixels into transparent, semi-transparent and opaque alpha states.
2. Optionally filter the triaged states according to supplied bleed factor.
3. Generate curves from the borders between the three states.
4. Economise points on curves according to a supplied tolerance factor.
5. Build monotonic polygons from curves and triangulate.

A number of functions are supplied to transform the bitmap into geometries.  Some are optional, but this order should be followed:

1. `generate_typemap`  
Convert bitmap into pixels with 3 alpha states - none/partial/full.  

2. `reduce_dither` (optional)  
Reduce high-frequency alpha state changes to reduce complexity.  

3. `dilate_alpha` (optional)  
Bleed semi-alpha state to surrounding pixels to avoid artefacts.  
              
4. `build_curves`  
Use X-axis scan-line edge detection to generate curves.  

5. `smooth_curves` (optional)  
Reduce complexity of curves by removing superfluous points.  

6. `triangulate`  
Iterate through all curves and generate triangles for final geometry.  

7. `set_texture_coordinates`  
Assign texture UV coordinates to geometry

# Future
                                              
Outline generation, optimisation and triangulation are very simplistic and work by scanning down the x-axis and creating monotonic polygons.  This was to keep mesh topology simple to process and to simplify the problem of optimisation.  More sophisticated geometry creation and optimisation could be used.

One alternative would be to start with a partial-alpha supporting quad and use recursively divided convex hulls to determine the shapes of no-alpha and full-alpha regions.  Stitch the shapes together and use conventional polygon optimisation and triangulation techniques from other open source libraries.

Another alternative would be to use a stochastic method of generating polygons and evolve to good fit by mutating and selecting points sets incrementally.  The fitness function would be the fewest points and triangles possible with the least error for the alpha states compared to the original image.  This could be an optimisation process applied after the original method, to then find a 'better fit'.

# Code conventions

* Line width: 120
* Tabs: 8 spaces
* Identifiers
  * Struct names: `snake_case`  
  * Enum names: `snake_case`
  * Variable names: `lowercase`  
  * Enum entries: `SNAKE_UPPER_CASE`  
  * Macros: `SNAKE_UPPER_CASE`
  * No type prefixes or suffixes
* Code comments: //
* Braces: K&R style

# License

`shrinkwrap` - a tool for converting bitmap alpha into geometry

Copyright (C) 2014  Jarrod Moldrich

`shrinkwrap` is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

`shrinkwrap` is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License along with this `shrinkwrap`.  If not, see <http://www.gnu.org/licenses/>.
