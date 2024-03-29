#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define  GLEW_STATIC
#include <GL/glew.h>
#include <pthread.h>

#include <GLFW/glfw3.h>

#include <GL/gl.h>

#define VAL 255



typedef struct {
  double x;
  double y;
}coords;

typedef struct {
  uint8_t r; 
  uint8_t g;
  uint8_t b;
}rgb_t;

coords screen;
rgb_t **tex_array = 0;
rgb_t *image;
int gwin;
int width = 640;
int height = 480;
int tex_w, tex_h;
double scale = 1./256;
double cx = -.6, cy = 0;
int color_rotate = 0;
int saturation = 1;
int invert = 0;
int max_iter = 256;
int dump = 1;
int global_iterator = 0;
int conversion_iterator_x = 0;
int conversion_iterator_y = 0;
GLuint texture;
GLFWwindow* window;

GLFWwindow* init_glfw();
void set_texture(GLuint tex); 
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void render(GLuint tex);
void screen_dump();
void keypress(unsigned char key, int x, int y);
void hsv_to_rgb(int hue, int min, int max, rgb_t *p);
void calc_mandel(rgb_t* px);
void alloc_texture();
void set_texture();
void mouseclick(int button, int state, int x, int y);
void resize(int w, int h);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void window_size_callback(GLFWwindow* window, int w, int h);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

int main(int c, char **v)
{
   
  GLFWwindow* win = init_glfw();
  glfwSetWindowPos(win, 1000, 500);

  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);

  //printf("keys:\n\tr: color rotation\n\tc: monochrome\n\ts: screen dump\n\t"
  //		"<, >: decrease/increase max iteration\n\tq: quit\n\tmouse buttons to zoom\n");

  glGenTextures(1, &texture);
  set_texture(texture);

  /* Loop until the user closes the window */
       while (!glfwWindowShouldClose(win))
    {

        render(texture);  

        //Draw the shape
        //glDrawArrays(GL_TRIANGLES, 0, 3);

        /* Swap front and back buffers */
        glfwSwapBuffers(win);

        /* Poll for and process events */
        glfwPollEvents();

        if(glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS){
           glfwSetWindowShouldClose(win, GL_TRUE);
        }

        /*
        if(glfwGetKey(win, ) == ) GLFW_PRESS{

        }
        */
    }

	return 0;
}


void set_texture(GLuint tex)
{
  printf("Allocating space\n");
	alloc_texture();
  printf("Calculating mandel... %d\n", global_iterator);
  ++global_iterator;
	calc_mandel(image);
  printf("mandel calculation complete\n");

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex);
 
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_w, tex_h,
		0, GL_RGB, GL_UNSIGNED_BYTE, tex_array[0]);
	
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  printf("Rendering to screen...\n");
	render(tex);
}

void alloc_texture()
{
	int i;
  int ow = tex_w;
  int oh = tex_h;
 
	for (tex_w = 1; tex_w < width;  tex_w <<= 1);
	for (tex_h = 1; tex_h < height; tex_h <<= 1);
 
	if (tex_h != oh || tex_w != ow){
		tex_array = realloc(tex_array, tex_h * tex_w * 3 + tex_h * sizeof(rgb_t*));
  }

	for (tex_array[0] = (rgb_t *)(tex_array + tex_h), i = 1; i < tex_h; i++){
		tex_array[i] = tex_array[i - 1] + tex_w;
  }
}


void render(GLuint tex)
{
	double	x = (double)width /tex_w,
		y = (double)height/tex_h;
 
	glClear(GL_COLOR_BUFFER_BIT);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
 
	glBindTexture(GL_TEXTURE_2D, tex);
 
	glBegin(GL_QUADS);
 
	glTexCoord2f(0, 0); glVertex2i(0, 0);
	glTexCoord2f(x, 0); glVertex2i(width, 0);
	glTexCoord2f(x, y); glVertex2i(width, height);
	glTexCoord2f(0, y); glVertex2i(0, height);
 
	glEnd();
 
	glFlush();
	glFinish();
}

GLFWwindow* init_glfw()
{

    /* Initialize the library */
    if (!glfwInit()){
        return NULL;
    }
   
    /*
     * Configure window options here if you so desire
     *
     * i.e.
     */
     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
     //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
     //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
     glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
     glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
     

    //the fourth parameter of glfwCreateWindow should be NULL for windowed mode and 
    //glfGetPrimaryMonitor() for full screen mode
    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(width, height, "Mandelbrot", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return NULL;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);

    /*
     * Initialize glew here
     */

    glewExperimental = GL_TRUE;
    glewInit();

    glViewport(0, 0, width, height);
  	glOrtho(0, width, 0, height, -1, 1);

  return window;

}


void calc_mandel(rgb_t* px)
{
	int i, j, iter, min, max;
	double x, y, zx, zy, zx2, zy2;

  min = max_iter; 
  max = 0;

  for (i = 0; i < height; i++) {
		px = tex_array[i];
		y = (i - height/2) * scale + cy;
		for (j = 0; j  < width; j++, px++) {
			x = (j - width/2) * scale + cx;
			iter = 0;
			zx = hypot(x - .25, y);

			if (x < zx - 2 * zx * zx + .25){
        iter = max_iter;
      }
			if ((x + 1)*(x + 1) + y * y < 1/16){
        iter = max_iter;
      }

			zx = zy = zx2 = zy2 = 0;
			for (; iter < max_iter && zx2 + zy2 < 4; iter++) {
				zy = 2 * zx * zy + y;
				zx = zx2 - zy2 + x;
				zx2 = zx * zx;
				zy2 = zy * zy;
			}

			if (iter < min){
        min = iter;
      }
			if (iter > max){ 
        max = iter;
      }
			*(unsigned short *)px = iter;
		}
	}
 
  printf("Converting hsv to rbg... ");
	for (i = 0; i < height; i++){
    ++conversion_iterator_y;
		for (j = 0, px = tex_array[i]; j  < width; j++, px++){
			hsv_to_rgb(*(unsigned short*)px, min, max, px);
      ++conversion_iterator_x;
    }
  }
	printf("done!\n");
 
  printf("converstion_iterator_x: %d\n", conversion_iterator_x);
  printf("converstion_iterator_y: %d\n", conversion_iterator_y);
}

void hsv_to_rgb(int hue, int min, int max, rgb_t *p)
{

	if (min == max){
    max = min + 1;
  }
	if (invert){
    hue = max - (hue - min);
  }
	if (!saturation) {
		p->r = p->g = p->b = 255 * (max - hue) / (max - min);
    return;
	}
	double h = fmod(color_rotate + 1e-4 + 4.0 * (hue - min) / (max - min), 6);
	double c = VAL * saturation;
	double X = c * (1 - fabs(fmod(h, 2) - 1));
 
	p->r = p->g = p->b = 0;
 
	switch((int)h) {
	case 0: p->r = c; p->g = X; break;
	case 1:	p->r = X; p->g = c; break;
	case 2: p->g = c; p->b = X; break;
	case 3: p->g = X; p->b = c; break;
	case 4: p->r = X; p->b = c; break;
	default:p->r = c; p->b = X; break;
	}
}

 


void window_size_callback(GLFWwindow* window, int w, int h)
{
	printf("resize %d %d\n", w, h);
	width = w;
	height = h;
 
	glViewport(0, 0, width, height);
	glOrtho(0, width, 0, height, -1, 1);
	set_texture(texture);
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
	  printf("resize %d %d\n", width, height);
    glViewport(0, 0, width, height);
  	glOrtho(0, width, 0, height, -1, 1);
    set_texture(texture);
    
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{

  double x = screen.x;
  double y = screen.y;

	switch(button) {
	case GLFW_MOUSE_BUTTON_LEFT: // zoom in
    printf("zoom: in\n"); 
		if (scale > fabs(x) * 1e-16 && scale > fabs(y) * 1e-16)
			scale /= 4;
		break;
	case GLFW_MOUSE_BUTTON_RIGHT: // zoom out 
    printf("zoom: out\n"); 
    scale *= 4;
		break;
	// any other button recenters 
	}
	set_texture(texture);
}
 
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	cx += (xpos - width / 2) * scale;
	cy -= (ypos - height/ 2) * scale;
  
  screen.x = xpos;
  screen.y = ypos;

  printf("cursor xpos: %2f\n", xpos);
  printf("cursor ypos: %2f\n", ypos);

}



/*
void screen_dump()
{
	char fn[100];
	int i;
	sprintf(fn, "screen%03d.ppm", dump++);
	FILE *fp = fopen(fn, "w");
	fprintf(fp, "P6\n%d %d\n255\n", width, height);
	for (i = height - 1; i >= 0; i--)
		fwrite(tex_array[i], 1, width * 3, fp);
	fclose(fp);
	printf("%s written\n", fn);
}
*/

/*
void keypress(unsigned char key, int x, int y)
{
	switch(key) {
	case 'q':	glFinish();
			//glutDestroyWindow(gwin);
			return;
	case 27:	scale = 1./256; cx = -.6; cy = 0; break;
 
	case 'r':	color_rotate = (color_rotate + 1) % 6;
			break;
 
	case '>': case '.':
			max_iter += 128;
			if (max_iter > 1 << 15) max_iter = 1 << 15;
			printf("max iter: %d\n", max_iter);
			break;
 
	case '<': case ',':
			max_iter -= 128;
			if (max_iter < 128) max_iter = 128;
			printf("max iter: %d\n", max_iter);
			break;
 
	case 'c':	saturation = 1 - saturation;
			break;
 
	case 's':	screen_dump(); return;
	case 'z':	max_iter = 4096; break;
	case 'x':	max_iter = 128; break;
	case ' ':	invert = !invert;
	}
	set_texture();
}
*/


