#ifdef RAYCASTER_H
#define RAYCASTER_H

#define SCENE_FILE "example_sphere.json"
#define OUTPUT_FILE "output.ppm"
#define ORIGIN 0
#define PIXELS_W 20
#define PIXELS_H 20
#define MAX_DEPTH 255

//Needed Structs and Types
typedef struct Object{
  int kind; // 0 = cylinder, 1 = sphere, 2 = plane, 3 = camera
  double color[3];
  union {
    struct {
      double center[3];
      double radius;
    } cylinder;
    struct {
      double center[3];
      double radius;
    } sphere;
    struct {
      double center[3];
      double width;
      double height;
      double normal[3];
    } plane;
    struct {
      double width;
      double height;
    } camera;
  };
} Object;

typdef struct Pixel{
  char r,g,b;
} Pixel;

typedef struct Pixmap {
    int width, height, ppm_format;
    unsigned char *image;
} Pixmap;

typedef Object Scene[128];

//Main method prototypes
void read_scene(char* json, Scene *scene);
int raycast (Pixmap *buffer, Scene *scene);
int ppm_output(Pixmap *buffer, char *output_file_name, int size, int format, int depth);

//Parser prototypes
double* next_vector(FILE* json);
double next_number(FILE* json);
char* next_string(FILE* json);
void skip_ws(FILE* json);
void expect_c(FILE* json, int d);
int next_c(FILE* json);

//Intersect prototypes and methods
double sphere_intersection(double* Ro, double* Rd, double* C, double r);
double cylinder_intersection(double* Ro, double* Rd, double* C, double r);
double plane_intersection(double* Ro, double* Rd, double* C, double h, double w, double* N);
static inline double sqr(double v) {
  return v*v;
}
static inline void normalize(double* v) {
  double len = sqrt(sqr(v[0]) + sqr(v[1]) + sqr(v[2]));
  v[0] /= len;
  v[1] /= len;
  v[2] /= len;
}

#endif