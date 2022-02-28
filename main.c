#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <GL/glut.h>

/* Animation parameters */
static float animation_parameter;
static int animation_active;

/* Callback functions. */
static void on_keyboard(unsigned char key, int x, int y);
static void on_reshape(int width, int height);
static void on_timer(int value);
static void on_display(void);

/* Drawing functions */
static void draw_ship(void);
static void draw_tunnel(void);
static void draw_obs(GLint num);
static void draw_hud(int run);
static void print_hud_text(int x, int y, char *string);

#define MAX_STR 100

/* HUD */
#define HUD_RUN 1
#define HUD_END 0

/* Views */
#define VIEW_DEF 0
#define VIEW_BACK 1
#define VIEW_TOP 2

/* Obstacle positions */
#define MAX_OBS_NUMBER 10
static float obs_xpos[MAX_OBS_NUMBER];
static float obs_ypos[MAX_OBS_NUMBER];
static float obs_zpos[MAX_OBS_NUMBER];

/* Custom transformation matrixes for drawing ship */
GLfloat skewer_1[4][4] = {
	{1, 0, -0.8	, 0},
	{0, 1, 0, 0.5},
	{0, 0, 1, 0.5},
	{0, 0, 0, 1},
};
GLfloat skewer_2[4][4] = {
	{1, 0, 0, 0},
	{0, 1, 0, 0.9},
	{0, 0, 1, -0.1},
	{0, 0.15, 0, 1},
};

/* Current ship position */
GLfloat ship_xpos = 0;
GLfloat ship_ypos = 1;

/* Position to go to */
GLint goto_xpos = 0;
GLint goto_ypos = 1;

/* Speed */
GLfloat speed = 1.3; /* Current speed */
GLfloat speed_inc = 0.2; /* Speed increment */
GLfloat mov_speed = 0.5; /* Ship speed */

GLint view = 0;

GLint obs_num = 0;
GLint wall_freq = 25; /* Number of regular obstacles between two walls */
GLint wall = -1; /* Holds index of a wall obstacle */
GLint lives = 3;
GLint invincibility = 0; /* Counter for time passed with invincibility */

GLint closest_obj = 0; /* Holds position of the closest obstacle */
GLint new_obj_pos = 0; /* Holds next free obstacle index */

GLint score = 0;
char *score_text[MAX_STR];

/* Lights */
/* - End light */
GLfloat light_pos0[] = {0, -5, 3, 0};
GLfloat light_amb0[] = { 0.3, 0.3, 0.3, 1 };
GLfloat light_dif0[] = { 0.7, 0.7, 0.7, 1 };
GLfloat light_spe0[] = { 0.8, 0.8, 0.8, 1 };

/* - Game light */
GLfloat light_pos1[] = { 2, 3, -2, 0 };
GLfloat light_amb1[] = { 0.3, 0.3, 0.3, 1 };
GLfloat light_dif1[] = { 0.7, 0.7, 0.7, 1 };
GLfloat light_spe1[] = { 0.8, 0.8, 0.8, 1 };

/* - Engine light */
GLfloat light_pos2[] = { 0, 0, 3, 0 };
GLfloat light_amb2[] = { 0, 0, 0, 1 };
GLfloat light_dif2[] = { 1, 0, 0, 1 };
GLfloat light_spe2[] = { 1, 0, 0, 1 };

/* Current viewport size */
GLint viewport[4];

int main(int argc, char **argv){
	/* GLUT init */
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);

	/* Window init */
	glutInitWindowSize(1024, 768);
	glutInitWindowPosition(200, 200);
	glutCreateWindow(argv[0]);

	/* Event functions */
	glutKeyboardFunc(on_keyboard);
	glutReshapeFunc(on_reshape);
	glutDisplayFunc(on_display);

	animation_parameter = 0;
	animation_active = 0;

	glGetIntegerv(GL_VIEWPORT, viewport);

	int i;
	srand(time(NULL));

	/* Object position initialization */
	for (i = 0; i < MAX_OBS_NUMBER; i++) {
		obs_zpos[i] = -1;
	}

	/* OpenGL init */
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glClearColor(0, 0, 0, 0);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glEnable(GL_LIGHTING);
	glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	/* Lights init */
	glLightfv(GL_LIGHT0, GL_POSITION, light_pos0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_amb0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_dif0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_spe0);

	glEnable(GL_LIGHT1);
	glLightfv(GL_LIGHT1, GL_POSITION, light_pos1);
	glLightfv(GL_LIGHT1, GL_AMBIENT, light_amb1);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light_dif1);
	glLightfv(GL_LIGHT1, GL_SPECULAR, light_spe1);

	glLightfv(GL_LIGHT2, GL_POSITION, light_pos2);
	glLightfv(GL_LIGHT2, GL_AMBIENT, light_amb2);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, light_dif2);
	glLightfv(GL_LIGHT2, GL_SPECULAR, light_spe2);

	glutMainLoop();

	return 0;
}

static void on_reshape(int width, int height){
	glViewport(0, 0, width, height);
	glGetIntegerv(GL_VIEWPORT, viewport);

	/* Projection parameters */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, (float) width / height, 1, 1500);
}

static void on_display(void){
	int i;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	/* Eye positioning */
	if(lives == 0)
		gluLookAt(0, 0, 2, 0, 0, 0, 0, 1, 0);
	else{
		switch(view){
			case VIEW_DEF:
				gluLookAt(0.7, 3, 9, -1, 1, -8, 0, 1, 0);
				break;
			case VIEW_BACK:
				gluLookAt(0, 3, 10, 0, 3, 0, 0, 1, 0);
				break;
			case VIEW_TOP:
				gluLookAt(0, 15, 0, 0, 0, 0, 0, 0, -1);
				break;
		}
	}

	if(lives > 0){
		draw_hud(HUD_RUN);
		draw_tunnel();

		/* Drawing obstacles */
		for(i=MAX_OBS_NUMBER-1; i >= 0; i--){
			if(obs_zpos[i] >= 0){
				draw_obs(i);
			}
		}

		/* Ship positioning and drawing*/
		glPushMatrix();
			glTranslatef(ship_xpos, ship_ypos, 0);
			glRotatef( sin(animation_parameter / 10.0f) * 30.0f, 0, 0, 1);

			/* Engine light enabled only for drawing ship so that only rear is lit */
			glLightfv(GL_LIGHT2, GL_POSITION, light_pos2);
			glEnable(GL_LIGHT2);
			draw_ship();
			glDisable(GL_LIGHT2);

		glPopMatrix();
	}
	else{
		glEnable(GL_LIGHT0);
		glDisable(GL_LIGHT1);
		glDisable(GL_LIGHT2);
		draw_hud(HUD_END);
	}

	glutSwapBuffers();
}

static void on_keyboard(unsigned char key, int x, int y){
	switch (key) {
		case 27:
			/* Exit */
			exit(0);
			break;

		case 'g':
		case 'G':
			/* Start */
			if (!animation_active) {
				glutTimerFunc(50, on_timer, 0);
				animation_active = 1;
			}
			break;

		case 'h':
		case 'H':
			/* Stop */
			animation_active = 0;
			break;

		/* Views */
		case 'n':
		case 'N':
			/* Default */
			view = VIEW_DEF;
			break;

		case 'b':
		case 'B':
			/* Back */
			view = VIEW_BACK;
			break;

		case 'm':
		case 'M':
			/* Top */
			view = VIEW_TOP;
			break;

		/* Steering */
		case 'w':
		case 'W':
			if(goto_ypos < 5) goto_ypos += 2;
			break;
		case 's':
		case 'S':
			if(goto_ypos > 1) goto_ypos -= 2;
			break;
		case 'a':
		case 'A':
			if(goto_xpos > -2) goto_xpos -= 2;
			break;
		case 'd':
		case 'D':
			if(goto_xpos < 2) goto_xpos += 2;
			break;
	}
}

static void on_timer(int value){
	int i;

	if (value != 0)
		return;

	animation_parameter++;
	glutPostRedisplay();

	if(lives > 0){
		/* Generating new obstacles */
		if((int)animation_parameter % (int)(25.0 / speed) == 0){
			if(obs_num == wall_freq){
				obs_num = 0;
				wall = new_obj_pos;
			}
			else
				obs_num++;

			obs_zpos[new_obj_pos] = 120;
			/* Random x and y coordinates */
			obs_xpos[new_obj_pos] = (rand() % 3 - 1) * 2;
			obs_ypos[new_obj_pos] = rand() % 3 * 2 + 1;

			new_obj_pos++;
			if(new_obj_pos == MAX_OBS_NUMBER)
				new_obj_pos = 0;
		}

		/* Moving obstacles */
		for (i = 0; i < MAX_OBS_NUMBER; i++){
			if (obs_zpos[i] >= 0) {
				obs_zpos[i] -= speed;
				if (obs_zpos[i] < 0){
					obs_zpos[i] = -1;
					if(i == wall)
						wall = -1;
				}
			}
		}

		/* Moving ship to destination */
		if(ship_xpos != goto_xpos){
			if(fabs(ship_xpos - goto_xpos) < 0.01)
				ship_xpos = goto_xpos;
			else
				ship_xpos += (ship_xpos < goto_xpos) ? mov_speed : -mov_speed;
		}
		if(ship_ypos != goto_ypos){
			if(fabs(ship_ypos - goto_ypos) < 0.01)
				ship_ypos = goto_ypos;
			else
				ship_ypos += (ship_ypos < goto_ypos) ? mov_speed : -mov_speed;
		}

		/* Counting invincibility time */
		if(invincibility > 0)
			invincibility += 1;
		if(invincibility == 150)
			invincibility = 0;

		/* Detecting collisions */
		/* Checking only the object closest to the ship */
		if(obs_zpos[closest_obj] >= 5 && obs_zpos[closest_obj] <= 7.6){
			if(closest_obj == wall){
				if(!(fabs(ship_xpos - obs_xpos[closest_obj]) < 1) ||
					!(fabs(ship_ypos - obs_ypos[closest_obj]) < 1)){
						/* Wall hit */
						if(invincibility == 0){
							invincibility = 1;
							lives--;
							if(lives == 0)
								score = animation_parameter;
						}

						obs_zpos[closest_obj] = -1;
						wall = -1;
				}
				/* Increase speed after every wall */
				if(speed < 2.5)
					speed += speed_inc;
			}
			else{
				if(fabs(ship_xpos - obs_xpos[closest_obj]) < 1 &&
					fabs(ship_ypos - obs_ypos[closest_obj]) < 1){
						/* Object hit */
						if(invincibility == 0){
							invincibility = 1;
							lives--;
							if(lives == 0)
								score = animation_parameter;
						}

						obs_zpos[closest_obj] = -1;
				}
			}

			closest_obj++;
			if(closest_obj == MAX_OBS_NUMBER)
				closest_obj = 0;
		}
	}

	/* Reinitializing timer */
	if (animation_active)
		glutTimerFunc(20, on_timer, 0);
}

static void draw_ship(void){
	if(invincibility > 0 && invincibility % 5 > 2)
		return;

	glPushMatrix();

	/* Tip */
	glColor3f(0.5, 0.4, 0.8);
	glPushMatrix();
		glTranslatef(0, -0.2, -0.6);
		glScalef(0.5, 0.5, 1.7);
		/* Bases */
		glBegin(GL_POLYGON);
			glNormal3f(0, 0, 1);
			glVertex3f(-1, 0, 0);
			glVertex3f(-0.5, 1, 0);
			glVertex3f(0.5, 1, 0);
			glVertex3f(1, 0, 0);
		glEnd();
		glBegin(GL_POLYGON);
			glNormal3f(0, 0, -1);
			glVertex3f(-0.7, 0, -1);
			glVertex3f(-0.2, 0.7, -1);
			glVertex3f(0.2, 0.7, -1);
			glVertex3f(0.7, 0, -1);
		glEnd();
		/* Sides */
		glBegin(GL_POLYGON);
			glNormal3f(-1, 1, 0);
			glVertex3f(-1, 0, 0);
			glVertex3f(-0.7, 0, -1);
			glVertex3f(-0.2, 0.7, -1);
			glVertex3f(-0.5, 1, 0);
		glEnd();
		glBegin(GL_POLYGON);
			glNormal3f(0, 1, 0);
			glVertex3f(-0.2, 0.7, -1);
			glVertex3f(-0.5, 1, 0);
			glVertex3f(0.5, 1, 0);
			glVertex3f(0.2, 0.7, -1);
		glEnd();
		glBegin(GL_POLYGON);
			glNormal3f(1, 1, 0);
			glVertex3f(0.2, 0.7, -1);
			glVertex3f(0.5, 1, 0);
			glVertex3f(1, 0, 0);
			glVertex3f(0.7, 0, -1);
		glEnd();
		glBegin(GL_POLYGON);
			glNormal3f(0, -1, 0);
			glVertex3f(1, 0, 0);
			glVertex3f(-1, 0, 0);
			glVertex3f(-0.7, 0, -1);
			glVertex3f(0.7, 0, -1);
		glEnd();
	glPopMatrix();

	/* Body */
	glColor3f(0.5, 0.4, 0.8);
	glPushMatrix();
		glMultMatrixf(*skewer_2);
		glScalef(0.9, 0.5, 1.3);
		glutSolidCube(1.2);
	glPopMatrix();

	/* Wings */
	glPushMatrix();
		glColor3f(0.2, 0.3, 0.3);
		glTranslatef(-0.5, 0, 1);
		glScalef(1, 0.5, 1);

		/* - Left wing */
		glPushMatrix();
			glMultMatrixf(*skewer_1);
			glutSolidCube(0.7);
		glPopMatrix();
		
		/* - Right wing */
		skewer_1[0][2] *= -1;
		glTranslatef(1, 0, 0);
		glPushMatrix();
			glMultMatrixf(*skewer_1);
			glutSolidCube(0.7);
		glPopMatrix();
		skewer_1[0][2] *= -1;
	glPopMatrix();

	/* Engine */
	glPushMatrix();
		glColor3f(1, 0, 0);
		glTranslatef(0, 0, 1);

		/* - Left bar */
		glBegin(GL_POLYGON);
			glVertex3f(-0.1, -0.2, 0.2);
			glVertex3f(-0.05, -0.2, 0.2);
			glVertex3f(-0.1, 0.3, -0.27);
			glVertex3f(-0.2, 0.3, -0.27);
		glEnd();
		/* - Right bar */
		glBegin(GL_POLYGON);
			glVertex3f(0.1, -0.2, 0.2);
			glVertex3f(0.05, -0.2, 0.2);
			glVertex3f(0.1, 0.3, -0.27);
			glVertex3f(0.2, 0.3, -0.27);
		glEnd();
		/* - Middle bar */
		glBegin(GL_POLYGON);
			glVertex3f(0.02, -0.2, 0.2);
			glVertex3f(-0.02, -0.2, 0.2);
			glVertex3f(-0.05, 0.3, -0.27);
			glVertex3f(0.05, 0.3, -0.27);
		glEnd();
	glPopMatrix();

	glPopMatrix();
}

static void draw_tunnel(void){
	/* Left side */
	glColor3f(0.1, 0.1, 0.1);
	glBegin(GL_POLYGON);
		glVertex3f(-3, 0, 4);
		glVertex3f(-3, 6, 4);
		glColor3f(0, 0, 0);
		glVertex3f(-3, 6, -40);
		glVertex3f(-3, 0, -40);
	glEnd();

	/* Right side */
	glEnable(GL_LIGHT2);
	glColor3f(0.1, 0.1, 0.1);
	glBegin(GL_POLYGON);
		glVertex3f(3, 0, 4);
		glVertex3f(3, 6, 4);
		glColor3f(0, 0, 0);
		glVertex3f(3, 6, -40);
		glVertex3f(3, 0, -40);
	glEnd();

	/* Bottom */
	glColor3f(0.2, 0.2, 0.2);
	glBegin(GL_POLYGON);
		glVertex3f(3, 0, 4);
		glVertex3f(-3, 0, 4);
		glColor3f(0, 0, 0);
		glVertex3f(-3, 0, -40);
		glVertex3f(3, 0, -40);
	glEnd();
	glDisable(GL_LIGHT2);
}

static void draw_obs(GLint i){
	GLfloat z = -obs_zpos[i] + 5;
	if(i == wall){
		/* Temporarily changing depth buffer to read-only in order to properly blend walls.
		 Blending would otherwise depend only upon the drawing order of obstacles and cause 
		 some objects to be hidden behind walls */
		glDepthMask(GL_FALSE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_LIGHTING);
		/* Wall */
		glColor4f(0.7, 0.7, 0.7, 0.2);
		glBegin(GL_POLYGON);
			glVertex3f(-3, 6, z);
			glVertex3f(3, 6, z);
			glVertex3f(3, 0, z);
			glVertex3f(-3, 0, z);
		glEnd();

		/* Door */
		glColor4f(0.1, 0.9, 0.3, 0.3);
		glBegin(GL_POLYGON);
			glVertex3f(obs_xpos[i]+1, obs_ypos[i]-1, z);
			glVertex3f(obs_xpos[i]+1, obs_ypos[i]+1, z);
			glVertex3f(obs_xpos[i]-1, obs_ypos[i]+1, z);
			glVertex3f(obs_xpos[i]-1, obs_ypos[i]-1, z);
		glEnd();

		glEnable(GL_LIGHTING);
		glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
		glDepthMask(GL_TRUE);
	}
	else{
		/* Basic obstacle */
		glColor3f(1, 0, 0);
		glPushMatrix();
			glTranslatef(obs_xpos[i], obs_ypos[i], z);
			glRotatef(90, 0, 0, 1);
			glScalef(1, 1, 5);
			glutSolidSphere(0.4, 3, 2);
		glPopMatrix();
	}
}


static void draw_hud(int run){
	if(run == HUD_RUN){
		/* Draw lives if game runs */
		glColor3f(0.2, 0.9, 0.3);
		glPushMatrix();
			glDisable(GL_LIGHTING);
			glTranslatef(2.5, 1, 4);
			int i;
			for(i=0; i<lives; i++){
				glutSolidSphere(0.1, 16, 16);
				glTranslatef(-0.25, 0, 0);
			}
			glEnable(GL_LIGHTING);
		glPopMatrix();
	}
	else{
		/* Game over */
		int i;

		/* Spinner */
		glColor3f(0.35, 0.2, 0.6);
		glPushMatrix();
			glutSolidSphere(0.2, 32, 32);
			glTranslatef(0, 0, -2);
			glRotatef(animation_parameter, 0, 0, 1);
			for(i=0; i<360; i+=20){
				glPushMatrix();
					glRotatef(i, 0, 0, 1);
					glTranslatef(1, 0, 0);
					glutSolidCube(0.15);
				glPopMatrix();
			}
		glPopMatrix();

		/* Text */
		glColor3f(0.2, 0.9, 0.3);
		print_hud_text(50, 50, "GAME OVER");
		sprintf(score_text, "%s%d", "Your score is: ", (int)score);
		print_hud_text(50, 70, score_text);
	}
}

static void print_hud_text(int x, int y, char *string){
	glPushMatrix ();
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();

			glLoadIdentity();

			gluOrtho2D(0, viewport[2], viewport[3], 0);
			
			glDisable(GL_LIGHTING);
			glDepthFunc(GL_ALWAYS);

			/* X and Y represent percentages of a window size */
			GLfloat raster_xpos = x/100.0 * viewport[2] - glutBitmapLength(GLUT_BITMAP_HELVETICA_18, string)/2;
			GLfloat raster_ypos = y/100.0 * viewport[3] + 6;
			glRasterPos2f(raster_xpos, raster_ypos);

			int i;
			for(i = 0; i < strlen(string); i++)
				glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, string[i]);

			glDepthFunc(GL_LESS);
			glEnable(GL_LIGHTING);

		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}