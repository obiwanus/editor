#ifndef ED_MODEL_H
#define ED_MODEL_H

struct Vertex {
  int index;
  int vt_index;
  int vn_index;
};

struct Triangle {
  Vertex vertices[3];
};

struct Quad {
  Vertex vertices[4];
};

struct Image {
  int width;
  int height;
  int bytes_per_pixel;
  u32 *data;

  u32 color(int, int, r32);
};

struct Entity {
  v3 position = {{0, 0, 0}};
  v3 up = {{0, 1.0f, 0}};
  v3 direction = {{1.0f, 0, 0}};

  basis3 get_basis();
  m4x4 transform_to_entity_space();
};

struct AABBox {
  v3 min;
  v3 max;
};

struct Model : Entity {
  v3 *vertices;
  v3 *vns;
  v2 *vts;
  Triangle *triangles;
  Quad *quads;
  Image texture;
  v3 old_direction;

  r32 scale = 1.0f;
  bool display = true;
  bool debug = false;

  AABBox aabb;

  void read_texture(char *);
  void update_aabb();
  void destroy();
};

struct Ray {
  v3 origin;
  v3 direction;

  v3 point_at(r32 t);
  r32 hits_triangle(v3 vertices[3]);
};

enum Camera_Position {
  Camera_Position_User = 0,
  Camera_Position_Front,
  Camera_Position_Left,
  Camera_Position_Top,

  Camera_Position__COUNT,
};

struct Camera : Entity {
  // View frustum
  r32 near = -1.0f;
  r32 far = -100.0f;
  r32 top;
  r32 right;
  v2i viewport;

  bool ortho_projection = false;
  Camera_Position position_type;

  v3 pivot = {};
  v3 old_pivot = {};
  v3 old_position = {};
  v3 old_up = {};
  basis3 old_basis = {};

  void adjust_frustum(int, int);
  void look_at(v3);
  m4x4 projection_matrix();
  m4x4 rotation_matrix(v2);
  r32 distance_to_pivot();
  Ray get_ray_through_pixel(v2i);
};

struct Plane {
  v3 normal;
  v3 point;
};


#endif  // ED_MODEL_H
