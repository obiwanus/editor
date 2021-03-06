
inline u32 get_rgb_u32(v3 color) {
  assert(color.r >= 0 && color.r <= 1);
  assert(color.g >= 0 && color.g <= 1);
  assert(color.b >= 0 && color.b <= 1);

  u32 result = 0x00000000;
  u8 R = (u8)(color.r * 255);
  u8 G = (u8)(color.g * 255);
  u8 B = (u8)(color.b * 255);
  result = R << 16 | G << 8 | B;
  return result;
}

void draw_pixel(Pixel_Buffer *buffer, int x, int y, u32 color) {
  // Origin in bottom left
  y = buffer->height - y - 1;  // y should never be equal buffer->height

  u32 *pixel = (u32 *)buffer->memory + x + y * buffer->width;
  *pixel = color;
}

void draw_pixel(Area *area, int x, int y, u32 color) {
  // Take area's origin into account
  int X = x + area->left;

  // Origin in bottom left.
  // y should never be equal buffer->height
  int Y = area->buffer->height - (y + area->bottom) - 1;

  u32 *pixel = (u32 *)area->buffer->memory + X + Y * area->buffer->width;
  *pixel = color;
}

void draw_line(Pixel_Buffer *buffer, v2i A, v2i B, u32 color, int width = 1) {
  bool swapped = false;
  if (abs(B.x - A.x) < abs(B.y - A.y)) {
    swap(A.x, A.y);
    swap(B.x, B.y);
    swapped = true;
  }
  if (B.x < A.x) {
    swap(A, B);
  }
  int dy = B.y - A.y;
  int dx = B.x - A.x;
  int sign = (dy >= 0) ? 1 : -1;
  int error = sign * dy - dx;
  int error_step = sign * dy;
  int y = A.y;
  for (int x = A.x; x <= B.x; ++x) {
    for (int i = 0; i < width; ++i) {
      int X = x, Y = y + i;
      if (swapped) {
        X = y + i;
        Y = x;
      }
      if (X >= 0 && X < buffer->width && Y >= 0 && Y < buffer->height) {
        draw_pixel(buffer, X, Y, color);
      }
    }
    error += error_step;
    if (error > 0) {
      error -= dx;
      y += sign;
    }
  }
}

void draw_line(Area *area, v2i A, v2i B, u32 color, int width = 1) {
  int area_width = area->get_width();
  int area_height = area->get_height();

  // Cull
  if ((A.x < 0 && B.x < 0) || (A.y < 0 && B.y < 0) ||
      (A.x > area_width && B.x > area_width) ||
      (A.y > area_height && B.y > area_height)) {
    return;  // line is outside area
  }

  // TODO: bugs
  // Either culling or clipping below should be modified

  // Clip against area bounds
  if (A.x > B.x) swap(A, B);
  if (A.x < 0) A = lerp(A, B, (r32)(-A.x) / (B.x - A.x));
  if (B.x > area_width) B = lerp(A, B, (r32)(area_width - A.x) / (B.x - A.x));

  if (A.y > B.y) swap(A, B);
  if (A.y < 0) A = lerp(A, B, (r32)(-A.y) / (B.y - A.y));
  if (B.y > area_height) B = lerp(A, B, (r32)(area_height - A.y) / (B.y - A.y));

  // Put into the area
  v2i offset = V2i(area->left, area->bottom);
  A += offset;
  B += offset;

  draw_line(area->buffer, A, B, color, width);
}

void draw_line(Area *area, v3 Af, v3 Bf, u32 color, r32 *z_buffer) {
  int area_width = area->get_width();
  int area_height = area->get_height();

  // Super unoptimized, as everything related to drawing so far
  bool swapped = false;
  if (abs(Bf.x - Af.x) < abs(Bf.y - Af.y)) {
    swap(Af.x, Af.y);
    swap(Bf.x, Bf.y);
    swapped = true;
  }
  if (Bf.x < Af.x) {
    swap(Af, Bf);
  }
  v3i A = V3i(Af);
  v3i B = V3i(Bf);
  int dy = B.y - A.y;
  int dx = B.x - A.x;
  int sign = (dy >= 0) ? 1 : -1;
  int error = sign * dy - dx;
  int error_step = sign * dy;
  int y = A.y;

  for (int x = A.x; x <= B.x; ++x) {
    int X = x, Y = y;
    if (swapped) swap(X, Y);  // unswap for drawing
    if (X >= 0 && X < area_width && Y >= 0 && Y < area_height) {
      r32 t = (r32)(x - A.x) / dx;
      r32 z = (1.0f - t) * Af.z + t * Bf.z;

      X = X + area->left;
      Y = area->buffer->height - (Y + area->bottom) - 1;

      int index = area->buffer->width * Y + X;
      if (z_buffer[index] < z) {
        z_buffer[index] = z;
        ((u32 *)area->buffer->memory)[index] = color;
      }
    }
    error += error_step;
    if (error > 0) {
      error -= dx;
      y += sign;
    }
  }
}

int orient2d(v2i p, v2i a, v2i b) {
  return (a.x - p.x) * (b.y - p.y) - (a.y - p.y) * (b.x - p.x);
}

bool is_top_left(v2i vert0, v2i vert1) {
  v2i edge = vert1 - vert0;
  if (edge.y < 0) return true;                 // edge goes down => it's left
  if (edge.y == 0 && edge.x < 0) return true;  // edge is top
  return false;
}

struct Triangle_Edge {
  v4 step_x;
  v4 step_y;

  v4 init(v2, v2, v2);
  void adjust_step(v4);
};

v4 Triangle_Edge::init(v2 vert0, v2 vert1, v2 origin) {
  r32 A = vert0.y - vert1.y;
  r32 B = vert1.x - vert0.x;
  r32 C = vert0.x * vert1.y - vert0.y * vert1.x;

  this->step_x = v4(A * 4);
  this->step_y = v4(B * 1);

  // x, y values for initial pixel block
  v4 x = v4(origin.x) + v4(0, 1, 2, 3);
  v4 y = v4(origin.y);

  v4 w_row = v4(A) * x + v4(B) * y + v4(C);
  return w_row;
}

void Triangle_Edge::adjust_step(v4 inv_denom) {
  this->step_x *= inv_denom;
  this->step_y *= inv_denom;
}

void triangle_rasterize_simd(Area *area, v3 verts[], v3 vns[], r32 *z_buffer,
                             v3 light_dir, bool outline = false) {
  TIMED_BLOCK();

  Pixel_Buffer *buffer = area->buffer;

  v2 vert0 = V2(verts[0]);
  v2 vert1 = V2(verts[1]);
  v2 vert2 = V2(verts[2]);

  // Compute BB and align to integer grid
  r32 min_x = floor_r32(min3(vert0.x, vert1.x, vert2.x));
  r32 min_y = floor_r32(min3(vert0.y, vert1.y, vert2.y));
  r32 max_x = floor_r32(max3(vert0.x, vert1.x, vert2.x));
  r32 max_y = floor_r32(max3(vert0.y, vert1.y, vert2.y));

  // Clip against area bounds
  min_x = max(min_x, 0.0f);
  min_y = max(min_y, 0.0f);
  max_x = min(max_x, (r32)(area->get_width() - 1));
  max_y = min(max_y, (r32)(area->get_height() - 1));

  // Triangle setup
  v2 origin = V2(min_x, min_y);
  Triangle_Edge e01, e12, e20;
  v4 w0_row = e12.init(vert1, vert2, origin);
  v4 w1_row = e20.init(vert2, vert0, origin);
  v4 w2_row = e01.init(vert0, vert1, origin);
  v4 z0 = v4(verts[0].z);
  v4 z1 = v4(verts[1].z);
  v4 z2 = v4(verts[2].z);

  // Normalise barycentric coordinates
  v4 inv_denom = v4(1.0f) / (w0_row + w1_row + w2_row);
  w0_row *= inv_denom;
  w1_row *= inv_denom;
  w2_row *= inv_denom;
  e01.adjust_step(inv_denom);
  e12.adjust_step(inv_denom);
  e20.adjust_step(inv_denom);

  // Calculate intensity at vertices for Gouraud shading
  light_dir = light_dir.normalized();
  v4 in[3];
  for (int i = 0; i < 3; ++i) {
    in[i] = v4(-vns[i].normalized() * light_dir);
  }

  // Real pixel start and end coords
  v2i p_min = V2i(area->left + (int)min_x,
                  buffer->height - (int)max_y - area->bottom - 1);
  v2i p_max = V2i(area->left + (int)max_x,
                  buffer->height - (int)min_y - area->bottom - 1);

  v4 zero = v4::zero();

  v4i buffer_width_wide = v4i(buffer->width);
  v4i x_step_wide = v4i(4);
  v4 max_intensity = v4(220.0f);
  v4 min_intensity = v4(40.0f);

  int pitch = buffer->width;
  u32 *pixel_row = (u32 *)buffer->memory + p_max.y * pitch;
  r32 *z_buffer_row = z_buffer + p_max.y * pitch;

  TIME_BEGIN(rasterization);
  // Rasterize
  for (int y = p_max.y; y >= p_min.y; y -= 1) {
    // Barycentric coordinates at start of row
    v4 w0 = w0_row;
    v4 w1 = w1_row;
    v4 w2 = w2_row;
    v4i x_wide = v4i(p_min.x) + v4i(0, 1, 2, 3);

    for (int x = p_min.x; x <= p_max.x; x += 4) {
      // If point is on or inside all edges for any pixels, render those pixels
      v4i mask =
          float2bits(v4_and(cmpge(w0, zero), cmpge(w1, zero), cmpge(w2, zero)));
      mask &= cmplt(x_wide, buffer_width_wide);

      if (mask_not_zero(mask)) {
        v4 intensity = w0 * in[0] + w1 * in[1] + w2 * in[2];
        intensity = v4_and(intensity, cmpge(intensity, zero));
        v4i grey_ch = ftoi(v4_lerp(intensity, min_intensity, max_intensity));
        v4i grey = (shiftl<24>(grey_ch) | shiftl<16>(grey_ch) |
                    shiftl<8>(grey_ch) | grey_ch);

        v4 z_values = w0 * z0 + w1 * z1 + w2 * z2;
        v4 z_buffer_values = v4::loadu(z_buffer_row + x);
        v4 z_mask = v4_and(bits2float(mask), cmpge(z_values, z_buffer_values));
        v4 new_z_values =
            v4_or(v4_and(z_mask, z_values), v4_andnot(z_mask, z_buffer_values));
        new_z_values.storeu(z_buffer_row + x);
        mask &= float2bits(z_mask);

        u32 *pixel = pixel_row + x;
        v4i original_color = v4i::loadu(pixel);
        v4i masked_out = (mask & grey) | andnot(mask, original_color);
        masked_out.storeu(pixel);
      }

      // One step to the right
      w0 += e12.step_x;
      w1 += e20.step_x;
      w2 += e01.step_x;
      x_wide += x_step_wide;
    }

    // One row step up
    w0_row += e12.step_y;
    w1_row += e20.step_y;
    w2_row += e01.step_y;
    pixel_row -= pitch;
    z_buffer_row -= pitch;
  }
  TIME_END(rasterization, (p_max.x - p_min.x + 1) * (p_max.y - p_min.y + 1));
}

void triangle_shaded(Area *area, v3 verts[], v3 vns[], r32 *z_buffer,
                     v3 light_dir, bool outline = false) {
  TIMED_BLOCK();
  int area_width = area->get_width();
  int area_height = area->get_height();

  // clang-format off
  v2i t0 = V2i(verts[0]); r32 z0 = verts[0].z;
  v2i t1 = V2i(verts[1]); r32 z1 = verts[1].z;
  v2i t2 = V2i(verts[2]); r32 z2 = verts[2].z;

  if (t0.y == t1.y && t1.y == t2.y) return;
  if (t0.x == t1.x && t1.x == t2.x) return;

  // Calculate intensity at vertices for Gouraud shading
  light_dir = light_dir.normalized();
  r32 in[3];
  for (int i = 0; i < 3; ++i) {
    in[i] = -vns[i].normalized() * light_dir;
  }

  if (t0.y > t1.y) { swap(t0, t1); swap(z0, z1); swap(in[0], in[1]); }
  if (t0.y > t2.y) { swap(t0, t2); swap(z0, z2); swap(in[0], in[2]); }
  if (t1.y > t2.y) { swap(t1, t2); swap(z1, z2); swap(in[1], in[2]); }

  int total_height = t2.y - t0.y;
  for (int y = t0.y; y <= t2.y; y++) {
    if (y < 0 || y >= area_height) continue;
    bool second_half = y > t1.y || t1.y == t0.y;
    int segment_height = second_half ? (t2.y - t1.y) : (t1.y - t0.y);
    r32 dy_total = (r32)(y - t0.y) / total_height;
    r32 dy_segment =
        (r32)(second_half ? (y - t1.y) : (y - t0.y)) / segment_height;
    v2i A = lerp(t0, t2, dy_total);
    r32 A_z = lerp(z0, z2, dy_total);
    r32 A_in = lerp(in[0], in[2], dy_total);
    v2i B;
    r32 B_z;
    r32 B_in;
    if (!second_half) {
      B = lerp(t0, t1, dy_segment);
      B_z = lerp(z0, z1, dy_segment);
      B_in = lerp(in[0], in[1], dy_segment);
    } else {
      B = lerp(t1, t2, dy_segment);
      B_z = lerp(z1, z2, dy_segment);
      B_in = lerp(in[1], in[2], dy_segment);
    }
    if (A.x > B.x) { swap(A, B); swap(A_z, B_z); swap(A_in, B_in); };

    for (int x = A.x; x <= B.x; x++) {
      if (x < 0 || x >= area_width) continue;
      r32 t = (A.x == B.x) ? 1.0f : (r32)(x - A.x) / (B.x - A.x);
      r32 z = lerp(A_z, B_z, t);
      int index = area->buffer->width * (y + area->bottom) + (x + area->left);
      if (z_buffer[index] < z) {
        z_buffer[index] = z;
        r32 intensity = lerp(A_in, B_in, t);
        if (intensity < 0) intensity = 0;
        intensity = lerp(0.2f, 1.0f, intensity);
        const r32 grey = 0.7f;
        u32 color = get_rgb_u32(V3(grey, grey, grey) * intensity);
        if (outline && (x == A.x || x == B.x || y == t0.y || y == t2.y)) {
          color = 0x00FFAA40;
        }
        draw_pixel(area, x, y, color);
      }
    }
  }
  // clang-format on
}

inline u32 blend_color(u32 color_bg, u8 alpha_src, u32 color_fg) {
  if (alpha_src == 0xFF) {
    // Just draw foreground
    return color_fg;
  } else if (alpha_src == 0) {
    // Don't draw
    return color_bg;
  } else {
    // Blend and draw
    int alpha = alpha_src + 1;
    int alpha_inv = 256 - alpha_src;
    // Background
    u8 R_bg = (u8)((color_bg & 0x00FF0000) >> 16);
    u8 G_bg = (u8)((color_bg & 0x0000FF00) >> 8);
    u8 B_bg = (u8)((color_bg & 0x000000FF) >> 0);
    // Foreground
    u8 R_fg = (u8)((color_fg & 0x00FF0000) >> 16);
    u8 G_fg = (u8)((color_fg & 0x0000FF00) >> 8);
    u8 B_fg = (u8)((color_fg & 0x000000FF) >> 0);
    // Blend
    u8 R = (u8)((alpha * R_fg + alpha_inv * R_bg) >> 8);
    u8 G = (u8)((alpha * G_fg + alpha_inv * G_bg) >> 8);
    u8 B = (u8)((alpha * B_fg + alpha_inv * B_bg) >> 8);
    u32 blend = R << 16 | G << 8 | B << 0;
    return blend;
  }
}

void draw_rect(Area *area, Rect rect, u32 color) {
  // Draw rect in area

  int area_width = area->get_width();
  int area_height = area->get_height();

  if (rect.left < 0) rect.left = 0;
  if (rect.bottom < 0) rect.bottom = 0;
  if (rect.right > area_width) rect.right = area_width;
  if (rect.top > area_height) rect.top = area_height;

  for (int y = rect.bottom; y < rect.top; y++) {
    for (int x = rect.left; x < rect.right; x++) {
      // Don't care about performance (yet)
      draw_pixel(area, x, y, color);
    }
  }
}

void draw_rect(Pixel_Buffer *buffer, Rect rect, u32 color) {
  // Draw rect in buffer directly

  if (rect.left < 0) rect.left = 0;
  if (rect.bottom < 0) rect.bottom = 0;
  if (rect.right > buffer->width) rect.right = buffer->width;
  if (rect.top > buffer->width) rect.top = buffer->width;

  for (int y = rect.bottom; y < rect.top; y++) {
    for (int x = rect.left; x < rect.right; x++) {
      // Don't care about performance (yet)
      draw_pixel(buffer, x, y, color);
    }
  }
}

void draw_sprite(Area *area, Image *image, Rect src, Rect dst) {
  assert(src.get_width() == dst.get_width());
  assert(src.get_height() == dst.get_height());

  for (int y = 0; y < src.get_height(); ++y) {
    for (int x = 0; x < src.get_width(); ++x) {
      u32 *src_pixel = image->data +
                       image->width * (image->height - y + src.bottom) +
                       (x + src.left);
      u8 alpha = (u8)((0xFF000000 & *src_pixel) >> 24);
      if (alpha == 0xFF) {
        draw_pixel(area, x + dst.left, y + dst.bottom, *src_pixel);
      }
    }
  }
}

int get_string_len(char *string) {
  int length = 0;
  int ch = 0;
  while (string[ch] != '\0') {
    // We only measure length, height doesn't count
    if (string[ch] == '\n') {
      length = 0;
      ++ch;
      continue;
    }
    ED_Font_Codepoint *cp =
        g_font.codepoints + (string[ch] - g_font.first_char);

    int advance = 0, kern_advance = 0;
    stbtt_GetGlyphHMetrics(&g_font.info, cp->glyph, &advance, 0);
    if (string[ch + 1] != '\0') {
      ED_Font_Codepoint *next =
          g_font.codepoints + (*string - g_font.first_char);
      kern_advance =
          stbtt_GetGlyphKernAdvance(&g_font.info, cp->glyph, next->glyph);
    }
    length += (int)((advance + kern_advance) * g_font.scale);

    ++ch;
  }
  return length;
}

void draw_string(Area *area, v2i position, char *string, u32 text_color,
                 bool aligh_top = true, bool aligh_right = false) {
  int buffer_width = area->buffer->width;
  int buffer_height = area->buffer->height;

  if (!aligh_top) {
    // Convert position to top-left
    position.y = buffer_height - position.y - 1 - area->bottom;
  } else {
    position.y = buffer_height - area->top + position.y;
  }
  if (!aligh_right) {
    position.x += area->left;
  } else {
    position.x = area->right - position.x - get_string_len(string);
  }

  v2i start = V2i(position.x + 2, position.y + g_font.baseline);
  int ch = 0;
  while (string[ch] != '\0') {
    if (string[ch] == '\n') {
      start.x = position.x + 2;
      start.y += g_font.line_height;
      continue;
    }
    assert(string[ch] - g_font.first_char >= 0);
    assert(g_font.last_char - string[ch] >= 0);

    ED_Font_Codepoint *codepoint =
        g_font.codepoints + (string[ch] - g_font.first_char);
    u8 *char_bitmap = codepoint->bitmap;

    int min_y = max(buffer_height - area->top, (int)start.y + codepoint->y0);
    int max_y =
        min(buffer_height - area->bottom - 1, (int)start.y + codepoint->y1);
    int min_x = max(area->left, (int)start.x + codepoint->x0);
    int max_x = min(area->right, (int)start.x + codepoint->x1);
    for (int y = min_y; y < max_y; ++y) {
      for (int x = min_x; x < max_x; ++x) {
        u8 alpha_src =
            char_bitmap[(x - min_x) + (y - min_y) * codepoint->width];
        u32 *dst_pixel = (u32 *)area->buffer->memory + x + y * buffer_width;
        *dst_pixel = blend_color(*dst_pixel, alpha_src, text_color);
      }
    }
    if (string[ch + 1] != '\0') {
      ED_Font_Codepoint *next_codepoint =
          g_font.codepoints + (string[ch + 1] - g_font.first_char);
      int advance, kern_advance;
      stbtt_GetGlyphHMetrics(&g_font.info, codepoint->glyph, &advance, 0);
      kern_advance = stbtt_GetGlyphKernAdvance(&g_font.info, codepoint->glyph,
                                               next_codepoint->glyph);
      start.x += (int)((advance + kern_advance) * g_font.scale);
    }
    ++ch;
  }
}
