/*
Copyright 2013 Cameron MacFarland
 
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <pebble.h>
 
void graphics_draw_arc(GContext *ctx, GPoint p, int radius, int thickness, int start, int end) {
  start = start % 360;
  end = end % 360;
 
  while (start < 0) start += 360;
  while (end < 0) end += 360;
 
  if (end == 0) end = 360;
  
  float sslope = (float)cos_lookup(start * TRIG_MAX_ANGLE / 360) / (float)sin_lookup(start * TRIG_MAX_ANGLE / 360);
  float eslope = (float)cos_lookup(end * TRIG_MAX_ANGLE / 360) / (float)sin_lookup(end * TRIG_MAX_ANGLE / 360);
 
  if (end == 360) eslope = -1000000;
 
  int ir2 = (radius - thickness) * (radius - thickness);
  int or2 = radius * radius;
 
  for (int x = -radius; x <= radius; x++)
    for (int y = -radius; y <= radius; y++)
    {
      int x2 = x * x;
      int y2 = y * y;
 
      if (
        (x2 + y2 < or2 && x2 + y2 >= ir2) &&
        (
          (y > 0 && start < 180 && x <= y * sslope) ||
          (y < 0 && start > 180 && x >= y * sslope) ||
          (y < 0 && start <= 180) ||
          (y == 0 && start <= 180 && x < 0) ||
          (y == 0 && start == 0 && x > 0)
        ) &&
        (
          (y > 0 && end < 180 && x >= y * eslope) ||
          (y < 0 && end > 180 && x <= y * eslope) ||
          (y > 0 && end >= 180) ||
          (y == 0 && end >= 180 && x < 0) ||
          (y == 0 && start == 0 && x > 0)
        )
      )
        graphics_draw_pixel(ctx, GPoint(p.x + x, p.y + y));
    }
}