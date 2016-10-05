/*********************************************************************
| Title: raycaster.c
| Author: JT Barrett
|
|
\*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include "raycaster.h"

int main(int argc, char** argv){
  

  //Create the scene of objects and parse the json file into it
  Object *scene = malloc(sizeof(Object)*128);
  int num_objects = read_scene(SCENE_FILE, scene);
  printf("\n%d\n", num_objects);

  //Take the scene and do the raycasting to generate a pixel buffer
  Pixbuff pixbuffer;
  raycast(pixbuffer, scene, num_objects);

  for (int i = 0; i < PIXELS_M*PIXELS_N*3; i++)
    printf("%d ", pixbuffer[i]);

  //Take the pixel buffer and write it as a valid ppm file.
  //ppm_output(&buffer, OUTPUT_FILE, sizeof(buffer), 3, 255);
  printf("\n%d\n %lf %lf", scene[0].kind, scene[0].camera.width, scene[0].camera.height);

  for (int i = 0; i < 3; i++){
    printf("\n|%lf|", scene[1].sphere.center[i]);
  }
  printf("\nmain exiting\n");
  return 0;
}


/**********************
|Main Methods
\*********************/
int read_scene(char* json_input, Scene scene){
  int c;
  FILE* json = fopen(json_input, "r");

  if (json == NULL) {
    fprintf(stderr, "Error: Could not open file \"%s\"\n", json);
    exit(1);
  }

  skip_ws(json);

  // Find the beginning of the list
  expect_c(json, '[');

  skip_ws(json);

  // Find the objects

  int obj_num = 0;
  while (1) {
    int c = fgetc(json);
    if (c == ']') {
      fprintf(stderr, "Error: This is the worst scene file EVER.\n");
      fclose(json);
      return -1;
    }
    if (c == '{') {
      skip_ws(json);

      // Parse the object
      char* key = next_string(json);
      if (strcmp(key, "type") != 0) {
        fprintf(stderr, "Error: Expected \"type\" key\n");
        exit(1);
      }

      skip_ws(json);

      expect_c(json, ':');

      skip_ws(json);

      char* value = next_string(json);

      if (strcmp(value, "camera") == 0) {
        scene[obj_num].kind = 3;
        printf("||Camera Recognized||\n");
      }
      else if (strcmp(value, "sphere") == 0) {
        scene[obj_num].kind = 1;
        printf("||Sphere Recognized||\n");
      }
      else if (strcmp(value, "plane") == 0) {
        scene[obj_num].kind = 2;
        printf("||Plane Recognized||\n");
      }
      else {
        fprintf(stderr, "Error: Unknown type, \"%s\"\n", value);
        exit(1);
      }

      skip_ws(json);

      while (1) {
        // , }
        c = next_c(json);
        if (c == '}') {
          // stop parsing this object
          break;
        }
        else if (c == ',') {
          // read another field
          skip_ws(json);
          char* key = next_string(json);
          skip_ws(json);
          expect_c(json, ':');
          skip_ws(json);

          if (scene[obj_num].kind == 3){
            if (strcmp(key, "width") == 0){
              double value = next_number(json);              
              memcpy(&scene[obj_num].camera.width, &value, sizeof(double));
              printf("width%lf\n", value);   
            }else if (strcmp(key, "height") == 0){
              double value = next_number(json);
              scene[obj_num].camera.height = value;
              memcpy(&scene[obj_num].camera.height, &value, sizeof(double));
              printf("width%lf\n", value);
            }else{
              fprintf(stderr, "Error: Unrecognized field \"%s\" for 'camera'.\n.", key);
              exit(1);
            }
          }
          else if (scene[obj_num].kind == 1){
            if (strcmp(key, "color") == 0){
              double* value = next_vector(json);
              memcpy(&scene[obj_num].color, value, sizeof(double)*3);
            }
            else if (strcmp(key, "radius") == 0)
              scene[obj_num].sphere.radius = next_number(json);
            else if (strcmp(key, "position") == 0){
              double* value = next_vector(json);
              memcpy(&scene[obj_num].sphere.center, value, sizeof(double)*3);
            }
            else{
              fprintf(stderr, "Error: Unrecognized field \"%s\" for 'sphere'.\n.", key);
              exit(1);
            }
          }
          else if (scene[obj_num].kind == 2){
            if (strcmp(key, "width") == 0)
              scene[obj_num].plane.width = next_number(json);
            else if (strcmp(key, "height") == 0)
              scene[obj_num].plane.height = next_number(json);
            else if (strcmp(key, "color") == 0){
              double* value = next_vector(json);
              memcpy(&scene[obj_num].color, value, sizeof(double)*3);
            }
            else if (strcmp(key, "position") == 0){
              double* value = next_vector(json);
              memcpy(&scene[obj_num].plane.center, value, sizeof(double)*3);
            }
            else if (strcmp(key, "normal") == 0){
              double* value = next_vector(json);
              memcpy(&scene[obj_num].plane.normal, value, sizeof(double)*3);
            }
            else{
              fprintf(stderr, "Error: Unrecognized field \"%s\" for 'plane'.\n.", key);
              exit(1);
            }
          }
        }
        skip_ws(json);
      }

      printf("--New Object Loaded--\n");
      obj_num++;
      /*else {
        fprintf(stderr, "Error: Unexpected value\n");
        exit(1);
      }*/
    }
    skip_ws(json);
    c = next_c(json);
    if (c == ',') {
      // noop
      skip_ws(json);
    }
    else if (c == ']') {
      fclose(json);
      return obj_num;
    }
    else {
      fprintf(stderr, "Error: Expecting ',' or ']'.\n");
      exit(1);
    }
  }
  return obj_num;
}

int raycast(Pixbuff buffer, Scene scene, int num_objects){

  int zero = 0;

  double cx = 0;
  double cy = 0;
  double h = scene[0].camera.height;
  double w = scene[0].camera.width;

  int M = PIXELS_M;
  int N = PIXELS_N;

  double pixheight = h / M;
  double pixwidth = w / N;
  int current_pixel = 0;
  for (int y = 0; y < M; y += 1) {
    for (int x = 0; x < N; x += 1) {
      double Ro[3] = { 0, 0, 0 };
      // Rd = normalize(P - Ro)
      double Rd[3] = {
        cx - (w / 2) + pixwidth * (x + 0.5),
        cy - (h / 2) + pixheight * (y + 0.5),
        1
      };
      normalize(Rd);

      double best_t = INFINITY;
      int best_obj = 0;
      for (int i = 1; i < num_objects; i += 1) {
        double t = 0;

        switch (scene[i].kind) {
        case 0:
          t = cylinder_intersection(Ro, Rd,
            scene[i].cylinder.center,
            scene[i].cylinder.radius);
          break;
        case 1:
          t = sphere_intersection(Ro, Rd,
            scene[i].sphere.center,
            scene[i].sphere.radius);
          break;
        default:
          // Horrible error
          exit(1);
        }

        if (t > 0 && t < best_t){
          best_t = t;
          best_obj = i;
        }
      }

      Pixel pix;

      if (best_t > 0 && best_t != INFINITY) {
        pix.r = (int)floor(scene[best_obj].color[0]*255.0);
        pix.b = (int)floor(scene[best_obj].color[1]*255.0);
        pix.g = (int)floor(scene[best_obj].color[2]*255.0);
        //printf("#");
      }
      else {
        pix.r = 0;
        pix.b = 0;
        pix.g = 0;
        //printf(".");
      }

      int e = (int)floor(scene[best_obj].color[0]*255.0);
      printf("\nPixel: %d %d %d\n", pix.r, pix.b, pix.g);
      buffer[current_pixel++] = pix.r;
      buffer[current_pixel++] = pix.b;
      buffer[current_pixel++] = pix.g;

    }
    printf("\n");
  }

  return 0;
}

int ppm_output(Pixbuff buffer, char *output_file_name, int size, int format, int depth){
  FILE *output_file;
  int i, j, num_pix;

  output_file = fopen(output_file_name, "w");
  if (!output_file)
  {
    fprintf(stderr, "\nError: Could not open file for write.");
    fclose(output_file);
    return -1;
  }
  else
  {
    fprintf(output_file, "P%d\n%d %d\n%d\n", format, PIXELS_M, PIXELS_N, depth);
    // Print out to the outfile in P6 format
    if (format == 6)
    {
      num_pix = fwrite((void *)buffer, 1, (size_t)size, output_file);
    }
    // Print out to the outfile in P3 format
    else if (format == 3)
    {   //printf("doing stuff!");
      for (i = 0; i<PIXELS_N; i++)
      {
        for (j = 0; j<PIXELS_M; j++)
        {
          fprintf(output_file, "%d ", buffer[i*PIXELS_M * 3 + 3 * j]);
          fprintf(output_file, "%d ", buffer[i*PIXELS_M * 3 + 3 * j + 1]);
          fprintf(output_file, "%d ", buffer[i*PIXELS_M * 3 + 3 * j + 2]);
        }
        // Add a new line character after each line pixels have been added to the outfile
        fprintf(output_file, "\n");
      }
    }
  }

  fclose(output_file);
  return 0;
}


/**********************
|Parser Methods
\*********************/
double* next_vector(FILE* json){
  double* v = malloc(3 * sizeof(double));
  expect_c(json, '[');
  skip_ws(json);
  v[0] = next_number(json);
  skip_ws(json);
  expect_c(json, ',');
  skip_ws(json);
  v[1] = next_number(json);
  skip_ws(json);
  expect_c(json, ',');
  skip_ws(json);
  v[2] = next_number(json);
  skip_ws(json);
  expect_c(json, ']');
  return v;
}
double next_number(FILE* json){
  double value;
  fscanf(json, "%lf", &value);
  // Error check this..
  return value;
}
char* next_string(FILE* json){
  char buffer[129];
  int c = next_c(json);
  if (c != '"') {
    fprintf(stderr, "Error: Expected string");
    exit(1);
  }
  c = next_c(json);
  int i = 0;
  while (c != '"') {
    if (i >= 128) {
      fprintf(stderr, "Error: Strings longer than 128 characters in length are not supported.\n");
      exit(1);
    }
    if (c == '\\') {
      fprintf(stderr, "Error: Strings with escape codes are not supported.\n");
      exit(1);
    }
    if (c < 32 || c > 126) {
      fprintf(stderr, "Error: Strings may contain only ascii characters.\n");
      exit(1);
    }
    buffer[i] = c;
    i += 1;
    c = next_c(json);
  }
  buffer[i] = 0;
  return strdup(buffer);
}
void skip_ws(FILE* json){
  int c = next_c(json);
  while (isspace(c)) {
    c = next_c(json);
  }
  ungetc(c, json);
}
void expect_c(FILE* json, int d){
  int c = next_c(json);
  if (c == d) return;
  fprintf(stderr, "Error: Expected '%c'");
  exit(1);
}
int next_c(FILE* json){
  int c = fgetc(json);
#ifdef DEBUG
  printf("next_c: '%c'\n", c);
#endif
  if (c == EOF) {
    fprintf(stderr, "Error: Unexpected end of file");
    exit(1);
  }
  return c;
}


/**********************
|Intersect Methods
\*********************/
double sphere_intersection(double* Ro, double* Rd, double* C, double r){
  double a = (sqr(Rd[0]) + sqr(Rd[1]) + sqr(Rd[2]));
  double b = (2 * (Ro[0] * Rd[0] - Rd[0] * C[0] + Ro[1] * Rd[1] - Rd[1] * C[1] + Ro[2] * Rd[2] - Rd[2] * C[2]));
  double c = sqr(Ro[0]) - 2 * Ro[0] * C[0] + sqr(C[0]) + sqr(Ro[1]) - 2 * Ro[1] * C[1] + sqr(C[1]) + sqr(Ro[2]) - 2 * Ro[2] * C[2] + sqr(C[2]) - sqr(r);

  double det = sqr(b) - 4 * a * c;
  if (det <= 0) return -1;

  det = sqrt(det);

  double t0 = (-b - det) / (2 * a);
  if (t0 > 0) return t0;

  double t1 = (-b + det) / (2 * a);
  if (t1 > 0) return t1;

  return -1;
}

double cylinder_intersection(double* Ro, double* Rd,
           double* C, double r) {
  // Step 1. Find the equation for the object you are
  // interested in..  (e.g., cylinder)
  //
  // x^2 + z^2 = r^2
  //
  // Step 2. Parameterize the equation with a center point
  // if needed
  //
  // (x-Cx)^2 + (z-Cz)^2 = r^2
  //
  // Step 3. Substitute the eq for a ray into our object
  // equation.
  //
  // (Rox + t*Rdx - Cx)^2 + (Roz + t*Rdz - Cz)^2 - r^2 = 0
  //
  // Step 4. Solve for t.
  //
  // Step 4a. Rewrite the equation (flatten).
  //
  // -r^2 +
  // t^2 * Rdx^2 +
  // t^2 * Rdz^2 +
  // 2*t * Rox * Rdx -
  // 2*t * Rdx * Cx +
  // 2*t * Roz * Rdz -
  // 2*t * Rdz * Cz +
  // Rox^2 -
  // 2*Rox*Cx +
  // Cx^2 +
  // Roz^2 -
  // 2*Roz*Cz +
  // Cz^2 = 0
  //
  // Steb 4b. Rewrite the equation in terms of t.
  //
  // t^2 * (Rdx^2 + Rdz^2) +
  // t * (2 * (Rox * Rdx - Rdx * Cx + Roz * Rdz - Rdz * Cz)) +
  // Rox^2 - 2*Rox*Cx + Cx^2 + Roz^2 - 2*Roz*Cz + Cz^2 - r^2 = 0
  //
  // Use the quadratic equation to solve for t..
  double a = (sqr(Rd[0]) + sqr(Rd[2]));
  double b = (2 * (Ro[0] * Rd[0] - Rd[0] * C[0] + Ro[2] * Rd[2] - Rd[2] * C[2]));
  double c = sqr(Ro[0]) - 2*Ro[0]*C[0] + sqr(C[0]) + sqr(Ro[2]) - 2*Ro[2]*C[2] + sqr(C[2]) - sqr(r);

  double det = sqr(b) - 4 * a * c;
  if (det < 0) return -1;

  det = sqrt(det);
  
  double t0 = (-b - det) / (2*a);
  if (t0 > 0) return t0;

  double t1 = (-b + det) / (2*a);
  if (t1 > 0) return t1;

  return -1;
}

double plane_intersection(double* Ro, double* Rd, double* C, double h, double w, double* N){
  return 0.0;
}

