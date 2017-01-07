
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
      int index = area->buffer->width * Y + X;
      if (z_buffer[index] < z) {
        z_buffer[index] = z;
        draw_pixel(area, X, Y, color);
      }
    }
    error += error_step;
    if (error > 0) {
      error -= dx;
      y += sign;
    }
  }
}

// void triangle_wireframe(Area *area, v3 verts[], u32 color) {
//   v2i vert0 = V2i(verts[0]);
//   v2i vert1 = V2i(verts[1]);
//   v2i vert2 = V2i(verts[2]);
//   draw_line(area, vert0, vert1, color);
//   draw_line(area, vert0, vert2, color);
//   draw_line(area, vert1, vert2, color);
// }

void triangle_filled(Area *area, v3 verts[], u32 color, r32 *z_buffer) {
  int area_width = area->get_width();
  int area_height = area->get_height();

  // clang-format off
  v2i t0 = V2i(verts[0]); r32 z0 = verts[0].z;
  v2i t1 = V2i(verts[1]); r32 z1 = verts[1].z;
  v2i t2 = V2i(verts[2]); r32 z2 = verts[2].z;

  if (t0.y == t1.y && t1.y == t2.y) return;
  if (t0.x == t1.x && t1.x == t2.x) return;

  if (t0.y > t1.y) { swap(t0, t1); swap(z0, z1); }
  if (t0.y > t2.y) { swap(t0, t2); swap(z0, z2); }
  if (t1.y > t2.y) { swap(t1, t2); swap(z1, z2); }

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
    v2i B;
    r32 B_z;
    if (!second_half) {
      B = lerp(t0, t1, dy_segment);
      B_z = lerp(z0, z1, dy_segment);
    } else {
      B = lerp(t1, t2, dy_segment);
      B_z = lerp(z1, z2, dy_segment);
    }
    if (A.x > B.x) { swap(A, B); swap(A_z, B_z); };
    for (int x = A.x; x <= B.x; x++) {
      if (x < 0 || x >= area_width) continue;
      r32 t = (A.x == B.x) ? 1.0f : (r32)(x - A.x) / (B.x - A.x);
      r32 z = lerp(A_z, B_z, t);
      int index = area->buffer->width * y + x;
      if (z_buffer[index] < z) {
        z_buffer[index] = z;
        draw_pixel(area, x, y, color);
      }
    }
  }
  // clang-format on
}

void triangle_shaded(Area *area, v3 verts[], v3 vns[], r32 *z_buffer,
                     v3 light_dir, bool outline = false) {
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
      int index = area->buffer->width * y + x;
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

void draw_string(Area *area, int string_x, int string_y, const char *string,
                 u32 text_color) {
  int area_width = area->get_width();
  int area_height = area->get_height();

  char c;
  v2i start = V2i(string_x, string_y + g_font.baseline);
  while ((c = *string++) != '\0') {
    if (c == '\n') {
      start.x = string_x;
      start.y += g_font.line_height;
      continue;
    }

    ED_Font_Codepoint *codepoint = g_font.codepoints + (c - g_font.first_char);
    u8 *char_bitmap = codepoint->bitmap;

    // TODO: this can be optimized significantly
    for (int x = 0; x < codepoint->width; x++) {
      int X = start.x + x;
      if (X < 0 || X > area_width) continue;
      for (int y = 0; y < codepoint->height; y++) {
        int Y = start.y + y - codepoint->height;
        if (Y < 0 || Y > area_height) continue;
        u8 alpha_src = char_bitmap[x + y * codepoint->width];
        u32 *pixel = (u32 *)area->buffer->memory + X + Y * area->buffer->width;
        if (alpha_src == 0xFF) {
          // Just draw foreground
          *pixel = text_color;
        } else if (alpha_src == 0) {
          // Don't draw
        } else {
          // Blend and draw
          int alpha = alpha_src + 1;
          int alpha_inv = 256 - alpha_src;
          // Background
          u32 bg = *pixel;
          u8 R_bg = (u8)((bg & 0x00FF0000) >> 16);
          u8 G_bg = (u8)((bg & 0x0000FF00) >> 8);
          u8 B_bg = (u8)((bg & 0x000000FF) >> 0);
          // Foreground
          u8 R_fg = (u8)((text_color & 0x00FF0000) >> 16);
          u8 G_fg = (u8)((text_color & 0x0000FF00) >> 8);
          u8 B_fg = (u8)((text_color & 0x000000FF) >> 0);
          // Blend
          u8 R = (u8)((alpha * R_fg + alpha_inv * R_bg) >> 8);
          u8 G = (u8)((alpha * G_fg + alpha_inv * G_bg) >> 8);
          u8 B = (u8)((alpha * B_fg + alpha_inv * B_bg) >> 8);
          *pixel = R << 16 | G << 8 | B << 0;
        }
      }
    }
    if (*string != '\0') {
      ED_Font_Codepoint *next_codepoint =
          g_font.codepoints + (*string - g_font.first_char);
      int advance, kern_advance;
      stbtt_GetGlyphHMetrics(&g_font.info, codepoint->glyph, &advance, 0);
      stbtt_GetCodepointHMetrics(&g_font.info, '6', &advance, 0);
      kern_advance = stbtt_GetGlyphKernAdvance(&g_font.info, codepoint->glyph,
                                               next_codepoint->glyph);
      start.x += (int)((advance + kern_advance) * g_font.scale);
    }
  }
}
