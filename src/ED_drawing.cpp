
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

inline void draw_pixel(Pixel_Buffer *buffer, v2i point, u32 color,
                       bool top_left = false) {
  int x = point.x;
  int y = point.y;

  if (!top_left) {
    y = buffer->height - y;  // Origin in bottom-left
  }
  u32 *pixel = (u32 *)buffer->memory + x + y * buffer->width;
  *pixel = color;
}

void draw_line(Pixel_Buffer *buffer, v2i A, v2i B, u32 color, int width = 1,
               bool top_left = false) {
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
        draw_pixel(buffer, V2i(X, Y), color, top_left);
      }
    }
    error += error_step;
    if (error > 0) {
      error -= dx;
      y += sign;
    }
  }
}

void draw_ui_line(Pixel_Buffer *buffer, v2i A, v2i B, u32 color) {
  // Draw line with the top left corner as the origin
  draw_line(buffer, A, B, color, 1, true);
}

void draw_line(Pixel_Buffer *buffer, v3 Af, v3 Bf, u32 color, r32 *z_buffer) {
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
    if (X >= 0 && X < buffer->width && Y >= 0 && Y < buffer->height) {
      r32 t = (r32)(x - A.x) / dx;
      r32 z = (1.0f - t) * Af.z + t * Bf.z;
      int index = buffer->width * Y + X;
      if (z_buffer[index] < z) {
        z_buffer[index] = z;
        draw_pixel(buffer, V2i(X, Y), color, false);
      }
    }
    error += error_step;
    if (error > 0) {
      error -= dx;
      y += sign;
    }
  }
}

void triangle_wireframe(Pixel_Buffer *buffer, v3 verts[], u32 color) {
  v2i vert0 = V2i(verts[0]);
  v2i vert1 = V2i(verts[1]);
  v2i vert2 = V2i(verts[2]);
  draw_line(buffer, vert0, vert1, color);
  draw_line(buffer, vert0, vert2, color);
  draw_line(buffer, vert1, vert2, color);
}

void triangle_filled(Pixel_Buffer *buffer, v3 verts[], u32 color,
                     r32 *z_buffer) {
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
    if (y < 0 || y >= buffer->height) continue;
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
      if (x < 0 || x >= buffer->width) continue;
      r32 t = (A.x == B.x) ? 1.0f : (r32)(x - A.x) / (B.x - A.x);
      r32 z = lerp(A_z, B_z, t);
      int index = buffer->width * y + x;
      if (z_buffer[index] < z) {
        z_buffer[index] = z;
        draw_pixel(buffer, V2i(x, y), color, false);
      }
    }
  }
  // clang-format on
}

void triangle_shaded(Pixel_Buffer *buffer, v3 verts[], v3 vns[], r32 *z_buffer,
                     v3 light_dir, bool outline = false) {
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
    if (y < 0 || y >= buffer->height) continue;
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
      if (x < 0 || x >= buffer->width) continue;
      r32 t = (A.x == B.x) ? 1.0f : (r32)(x - A.x) / (B.x - A.x);
      r32 z = lerp(A_z, B_z, t);
      int index = buffer->width * y + x;
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
        draw_pixel(buffer, V2i(x, y), color, false);
      }
    }
  }
  // clang-format on
}

void triangle_textured(Pixel_Buffer *buffer, v3i verts[], v2 vts[], v3 vns[],
                       Image texture, r32 *z_buffer) {
  v3i t0 = verts[0];
  v3i t1 = verts[1];
  v3i t2 = verts[2];

  v2 tex0 = vts[0];
  v2 tex1 = vts[1];
  v2 tex2 = vts[2];

  v3 n0 = vns[0];
  v3 n1 = vns[1];
  v3 n2 = vns[2];

  if (t0.y == t1.y && t1.y == t2.y) return;
  if (t0.x == t1.x && t1.x == t2.x) return;

  if (t0.y > t1.y) {
    swap(t0, t1);
    swap(tex0, tex1);
    swap(n0, n1);
  }
  if (t0.y > t2.y) {
    swap(t0, t2);
    swap(tex0, tex2);
    swap(n0, n2);
  }
  if (t1.y > t2.y) {
    swap(t1, t2);
    swap(tex1, tex2);
    swap(n1, n2);
  }

  int total_height = t2.y - t0.y;
  for (int y = t0.y; y <= t2.y; y++) {
    if (y < 0 || y >= buffer->height) continue;
    bool second_half = y > t1.y || t1.y == t0.y;
    int segment_height = second_half ? (t2.y - t1.y + 1) : (t1.y - t0.y + 1);
    r32 dy_total = (r32)(y - t0.y) / total_height;
    r32 dy_segment =
        (r32)(second_half ? (y - t1.y) : (y - t0.y)) / segment_height;
    v3i A = t0 + V3i(dy_total * V3(t2 - t0));  // TODO: try lerp
    v3i B;
    v2 Atex = tex0 + dy_total * (tex2 - tex0);
    v2 Btex;
    v3 An = n0 + dy_total * (n2 - n0);
    v3 Bn;
    if (!second_half) {
      B = t0 + V3i(dy_segment * V3(t1 - t0));
      Btex = tex0 + dy_segment * (tex1 - tex0);
      Bn = n0 + dy_segment * (n1 - n0);
    } else {
      B = t1 + V3i(dy_segment * V3(t2 - t1));
      Btex = tex1 + dy_segment * (tex2 - tex1);
      Bn = n1 + dy_segment * (n2 - n1);
    }
    if (A.x > B.x) {
      swap(A, B);
      swap(Atex, Btex);
      swap(An, Bn);
    };
    for (int x = A.x; x <= B.x; x++) {
      if (x < 0 || x >= buffer->width) continue;
      r32 t = (A.x == B.x) ? 1.0f : (r32)(x - A.x) / (B.x - A.x);
      r32 z = (r32)lerp(A.z, B.z, t);
      int index = buffer->width * y + x;
      if (z_buffer[index] < z) {
        z_buffer[index] = z;
        v3 n = lerp(An, Bn, t).normalized();
        r32 intensity = n * V3(0, 0, 1);
        if (intensity > 0) {
          // // Get color from texture
          v2 texel = lerp(Atex, Btex, t);
          v2i tex_coords =
              V2i(texture.width * texel.u, texture.height * texel.v);
          u32 color = texture.color(tex_coords.x,
                                    (texture.height - tex_coords.y), intensity);
          draw_pixel(buffer, V2i(x, y), color);
        }
      }
    }
  }
}

// void draw_ui_line(Pixel_Buffer *buffer, v2 A, v2 B, u32 color) {
//   v2i a = {(int)A.x, (int)A.y};
//   v2i b = {(int)B.x, (int)B.y};
//   draw_ui_line(buffer, a, b, color);
// }

void draw_rect(Pixel_Buffer *buffer, Rect rect, u32 color,
               bool top_left = true) {
  if (rect.left < 0) rect.left = 0;
  if (rect.top < 0) rect.top = 0;
  if (rect.right > buffer->width) rect.right = buffer->width;
  if (rect.bottom > buffer->height) rect.bottom = buffer->height;

  for (int x = rect.left; x < rect.right; x++) {
    for (int y = rect.top; y < rect.bottom; y++) {
      // Don't care about performance (yet)
      draw_pixel(buffer, V2i(x, y), color, top_left);
    }
  }
}

void draw_string(Pixel_Buffer *buffer, int x, int y, const char *string,
                 u32 color) {
  char c;
  while ((c = *string++) != '\0') {
  }
}
