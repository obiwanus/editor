
void *Program_Memory::allocate(size_t size) {
  // Deallocation is not intended (yet)
  void *result;
  if (this->allocated + size > MAX_INTERNAL_MEMORY_SIZE) {
    return NULL;
  }
  result = this->free_memory;
  this->free_memory = (void *)((u8 *)this->free_memory + size);
  return result;
}

bool is_number_sym(int ch) {
  return ('0' <= ch && ch <= '9') || ch == '-' || ch == '.';
}

void Program_State::read_wavefront_obj_file(char *filename) {
  FILE *f = fopen(filename, "rb");
  if (f == NULL) {
    printf("Can't open model file %s\n", filename);
    exit(1);
  }

  int num_models = 0;

  Model model = {};
  model.set_defaults();
  sprintf(model.name, "Model %d", num_models + 1);

  // Where indices start for each model
  int v_start = 0;
  int vn_start = 0;
  int vt_start = 0;

  const int kBufSize = 300;
  char string[kBufSize];
  while (fgets(string, kBufSize, f) != NULL) {
    if (string[0] == 'o' && string[1] == ' ') {
      if (model.triangles != NULL) {
        // Push the model
        sb_push(this->models, model);
        ++num_models;

        // Update indices for the next model
        v_start += model.vertices ? sb_count(model.vertices) : 0;
        vn_start += model.vns ? sb_count(model.vns) : 0;
        vt_start += model.vts ? sb_count(model.vts) : 0;

        // Start a new one
        model.set_defaults();
      }
      // Set model name
      strncpy(model.name, string + 2, model.kMaxNameLength);
      model.name[strlen(model.name) - 1] = '\0';
    } else if (string[0] == 'f' && string[1] == ' ') {
      // Face string - parse by hand
      char number_string[kBufSize / 2];
      const int kMaxIndices = 30;
      int indices[kMaxIndices];
      int num_indices = 0;
      int ch = 0;
      while (string[2 + ch] != '\n') {
        int num_symbols = 0;
        char c;
        while (is_number_sym(c = string[2 + ch])) {
          number_string[num_symbols++] = c;
          ++ch;
        }
        number_string[num_symbols] = '\0';
        if (c != '\n') ++ch;

        int parsed_index = -1;
        if (num_symbols > 0) {
          parsed_index = atoi(number_string) - 1;  // start from 0

          // Substract the previous model indices
          if (num_indices % 3 == 0) {
            parsed_index -= v_start;
          } else if (num_indices % 3 == 1) {
            parsed_index -= vt_start;
          } else if (num_indices % 3 == 2) {
            parsed_index -= vn_start;
          } else {
            INVALID_CODE_PATH;
          }
        }
        indices[num_indices++] = parsed_index;
        assert(num_indices <= kMaxIndices);
      }
      if (num_indices >= 9 && (num_indices % 3) == 0) {
        Fan fan;
        fan.num_vertices = num_indices / 3;
        for (int i = 0; i < fan.num_vertices; ++i) {
          fan.vertices[i].index = indices[3 * i];
          fan.vertices[i].vt_index = indices[3 * i + 1];
          fan.vertices[i].vn_index = indices[3 * i + 2];
        }
        int triangle_count = fan.num_vertices - 2;
        for (int i = 0; i < triangle_count; ++i) {
          Triangle triangle;
          triangle.vertices[0] = fan.vertices[0];
          triangle.vertices[1] = fan.vertices[1 + i];
          triangle.vertices[2] = fan.vertices[2 + i];

          sb_push(model.triangles, triangle);
        }
      } else {
        printf("Unknown face definition in file %s, line \"%s\"\n", filename,
               string);
        exit(1);
      }
    } else if (string[0] == 'v' && string[1] == ' ') {
      // Vertex
      v3 vertex;
      sscanf(string + 2, "%f %f %f", &vertex.x, &vertex.y, &vertex.z);
      sb_push(model.vertices, vertex);
    } else if (string[0] == 'v' && string[1] == 't' && string[2] == ' ') {
      // Texture vertex
      v2 vt;  // only expecting 2d textures
      sscanf(string + 3, "%f %f", &vt.x, &vt.y);
      sb_push(model.vts, vt);
    } else if (string[0] == 'v' && string[1] == 'n' && string[2] == ' ') {
      // Normal
      v3 vn;
      sscanf(string + 3, "%f %f %f", &vn.x, &vn.y, &vn.z);
      sb_push(model.vns, vn);
    }
  }

  if (model.vertices != NULL) {
    // sb_push(this->models, model);
    {
      assert(this->models == NULL);
      // push model
      int *p = (int *)realloc(0, sizeof(Model) + sizeof(int) * 2);
      if (p) {
        p[1] = 1;
        p[0] = 1;
        Model *result = (Model *)(p + 2);
        *result = model;
        this->models = result;
      } else {
        assert(!"vasia");
      }
    }
    num_models++;
  }

  // Find AABB and reposition the models
  for (int i = 0; i < sb_count(this->models); ++i) {
    Model *m = this->models + i;
    m->position = V3(0, 0, 0);
    m->update_aabb(false);  // not rotated
    m->position = (m->aabb.min + m->aabb.max) * 0.5f;
    for (int j = 0; j < sb_count(m->vertices); ++j) {
      m->vertices[j] -= m->position;
    }
  }

  fclose(f);
}

void Program_State::init(Program_Memory *memory, Pixel_Buffer *buffer,
                         Raytrace_Work_Queue *queue) {
  Program_State *state = this;

  g_FPS.value = 0;
  g_FPS.min = 100;
  g_FPS.max = 0;
  g_FPS.frame_count = 10;

  state->kWindowWidth = 1500;
  state->kWindowHeight = 1000;

  state->UI = (User_Interface *)malloc(sizeof(*state->UI));
  memset(state->UI, 0, sizeof(*state->UI));
  state->UI->memory = memory;
  state->UI->buffer = buffer;

  state->raytrace_queue = queue;
  for (int i = 0; i < g_kNumThreads; ++i) {
    state->raytrace_queue->entries_in_progress[i] = -1;
  }

  // Allocate memory for the main buffer
  buffer->allocate();
  buffer->width = state->kWindowWidth;
  buffer->height = state->kWindowHeight;

  g_font.load_from_file("../src/ui/fonts/Ubuntu-R.ttf", 16);

  state->icons.load_from_file("../assets/icons.png");

  // Create main area
  sb_reserve(state->UI->areas, 10);  // reserve memory for 10 area pointers
  state->UI->create_area(
      NULL, {0, state->kWindowHeight, state->kWindowWidth, 0}, false);

  state->UI->cursor = V3(0, 0, 0);

  // For some reason these pointers are not initialized by default
  // on windows using C++11 struct initializers
  state->models = NULL;
  state->selected_model = NULL;

  this->read_wavefront_obj_file("../models/african_head/african_head.wobj");
  // this->read_wavefront_obj_file("../models/teapot/teapot.wobj");
  // this->read_wavefront_obj_file("../models/cube/cube.wobj");
  // this->read_wavefront_obj_file("../models/test.wobj");
  // this->read_wavefront_obj_file("../models/culdesac/geometricCuldesac.wobj");

  Model *model = &state->models[0];
  model->read_texture("../models/african_head/african_head_diffuse.jpg");
  // model->read_texture("../models/cube/cube.png");

}

void ED_Font::load_from_file(char *filename, int char_height) {
  // Load font into buffer
  {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
      printf("Can't open file '%s'\n", filename);
      exit(1);
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    this->ttf_raw_data = (u8 *)malloc(size * sizeof(*this->ttf_raw_data));
    assert(this->ttf_raw_data != NULL);
    size_t result = fread(this->ttf_raw_data, 1, size, file);

    fclose(file);
  }

  int result =
      stbtt_InitFont(&this->info, this->ttf_raw_data,
                     stbtt_GetFontOffsetForIndex(this->ttf_raw_data, 0));
  if (!result) {
    printf("Can't init font\n");
    exit(1);
  }

  this->scale = stbtt_ScaleForPixelHeight(&this->info, (r32)char_height);
  int ascent, descent, line_gap;
  stbtt_GetFontVMetrics(&this->info, &ascent, &descent, &line_gap);
  this->baseline = (int)(ascent * this->scale);
  this->line_height = (int)((line_gap + ascent - descent) * this->scale);

  int alphabet_size = this->last_char - this->first_char + 1;

  // Fill the char metadata and calculate the
  int buffer_size = 0;
  for (int i = 0; i < alphabet_size; ++i) {
    ED_Font_Codepoint *codepoint = this->codepoints + i;
    codepoint->glyph = stbtt_FindGlyphIndex(&this->info, this->first_char + i);
    int x0, y0, x1, y1;
    stbtt_GetGlyphBitmapBox(&this->info, codepoint->glyph, this->scale,
                            this->scale, &x0, &y0, &x1, &y1);
    codepoint->x0 = x0;
    codepoint->y0 = y0;
    codepoint->x1 = x1;
    codepoint->y1 = y1;
    codepoint->width = x1 - x0;
    codepoint->height = y1 - y0;
    buffer_size += codepoint->width * codepoint->height;
  }
  this->bitmap = (u8 *)malloc(buffer_size * sizeof(*this->bitmap));

  // Fill the char bitmaps
  u8 *char_bitmap = this->bitmap;
  for (int i = 0; i < alphabet_size; ++i) {
    ED_Font_Codepoint *codepoint = this->codepoints + i;
    codepoint->bitmap = char_bitmap;
    stbtt_MakeCodepointBitmap(&this->info, codepoint->bitmap, codepoint->width,
                              codepoint->height, codepoint->width, this->scale,
                              this->scale, this->first_char + i);
    // Advance to the next char
    char_bitmap += codepoint->width * codepoint->height;
  }
}

Update_Result update_and_render(Program_Memory *program_memory,
                                Program_State *state, User_Input *input) {
  Update_Result result = {};

  // Project mouse pointer into main area
  Area *main_area = state->UI->areas[0];
  input->mouse.y = main_area->get_height() - input->mouse.y;

  // Remember where dragging starts
  for (int i = 0; i < 3; ++i) {
    if (input->button_went_down((Input_Button)i)) {
      input->mouse_positions[i] = input->mouse;
    }
  }

  result = state->UI->update_and_draw(input, state);

  return result;
}

bool Rect::contains(v2i point) {
  bool result = (this->left <= point.x) && (point.x <= this->right) &&
                (this->bottom <= point.y) && (point.y <= this->top);
  return result;
}

v2i Rect::projected(v2i point) {
  v2i result = point;

  result.x -= this->left;
  result.y -= this->bottom;

  return result;
}

inline int Rect::get_width() {
  int result = this->right - this->left;
  assert(result >= 0);
  return result;
}

inline int Rect::get_height() {
  int result = this->top - this->bottom;
  assert(result >= 0);
  return result;
}

int Rect::get_area() {
  // Physical area
  return this->get_width() * this->get_height();
}

void Pixel_Buffer::allocate() {
  *this = {};
  this->max_width = 3000;
  this->max_height = 3000;
  this->memory = malloc(this->max_width * this->max_height * sizeof(u32));
}

Rect Pixel_Buffer::get_rect() {
  Rect result = {0, 0, this->width, this->height};
  return result;
}

bool User_Input::button_was_down(Input_Button button) {
  if (this->old == NULL) return false;
  return this->old->button_is_down(button);
}

bool User_Input::button_is_down(Input_Button button) {
  assert(button < IB__COUNT);
  return this->buttons[button];
}

bool User_Input::button_went_down(Input_Button button) {
  return this->button_is_down(button) && !this->button_was_down(button);
}

bool User_Input::button_went_up(Input_Button button) {
  return !this->button_is_down(button) && this->button_was_down(button);
}

bool User_Input::key_went_down(int key) {
  return this->button_went_down(IB_key) && this->symbol == key;
}

u32 Image::color(int x, int y, r32 intensity = 1.0f) {
  u32 result;

  // TODO: fix the images
  u32 raw_pixel = *(this->data + this->width * y + x);
  u32 R = (u32)(intensity * ((0x000000FF & raw_pixel) >> 0));
  u32 G = (u32)(intensity * ((0x0000FF00 & raw_pixel) >> 8));
  u32 B = (u32)(intensity * ((0x00FF0000 & raw_pixel) >> 16));
  u32 A = (0xFF000000 & raw_pixel) >> 24;
  result = (A << 24 | R << 16 | G << 8 | B << 0);
  return result;
}


void Image::load_from_file(char *filename) {
  this->data = (u32 *)stbi_load(filename, &this->width, &this->height,
                                &this->bytes_per_pixel, 4);
  if (this->bytes_per_pixel < 3 || this->bytes_per_pixel > 4) {
    printf("Image format not supported: %s\n", filename);
    exit(1);
  }
  if (this->data == NULL) {
    printf("Can't read file %s\n", filename);
    exit(1);
  }
}
