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

struct Fan {
  static const int kMaxNumVertices = 8;
  int num_vertices;
  Vertex vertices[kMaxNumVertices];
};

struct Triangle_Hit {
  r32 at;
  r32 barycentric[3];
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
  Image texture;
  v3 old_position;
  v3 old_direction;
  static const int kMaxNameLength = 100;
  char name[kMaxNameLength + 1];

  r32 scale = 1.0f;
  bool display = true;
  bool debug = false;

  AABBox aabb;
  m4x4 TransformMatrix;
  bool transform_calculated = false;

  void read_texture(char *);
  void update_aabb(bool);
  void set_defaults();
  void destroy();
  m4x4 get_transform_matrix();
};

struct Ray {
  v3 origin;
  v3 direction;

  v3 get_point_at(r32 t);
  Triangle_Hit hits_triangle(v3 vertices[3]);
  bool hits_aabb(AABBox);
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
