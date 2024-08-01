#include <windows.h>  
#include <GL\gl.h>
#include <GL\glu.h>
#include "cviogl.h"

#define DETAIL_LVL	20   // Use values in range 10-100

extern int 			mainPanel; 
extern int			OGLControlID;
extern float 		pitch;
extern float 		yaw;
extern float 		roll;
GLUquadricObj   	*object;   

void InitOGLControl(void);
void RenderImage(int fastFlag);
void DrawMissileBody(int fastFlag);
void DrawClosedCylinder(GLUquadricObj *object, float radius, float height);
void LogMessage(const char *format, ...); 

//----------------------------------------------------------------------------
//  Initializes the OGL control and sets the rendering properties appropriately
//----------------------------------------------------------------------------        
void InitOGLControl(void)
{
	// Setup lighting for system
    OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_LIGHTING_ENABLE, 1);
    OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_LIGHT_SELECT,    1);
    OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_LIGHT_ENABLE,    1);
    OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_LIGHT_DISTANCE,  2.0);
    OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_LIGHT_LATITUDE, 30.0);
	OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_VIEW_AUTO_DISTANCE, OGLVAL_FALSE);
	OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_VIEW_DISTANCE, 2.0);
	OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_PROJECTION_TYPE, OGLVAL_ORTHOGRAPHIC);
	
	// Setup plot axes, grids, scaling, and plot area
	OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_ZNAME, "z-axis");
	OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_XNAME, "x-axis");
	OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_YNAME, "y-axis");
	OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_ZNAME_POINT_SIZE, 20);
	OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_XNAME_POINT_SIZE, 20); 
	OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_YNAME_POINT_SIZE, 20);
	OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_ZNAME_COLOR, OGLVAL_YELLOW); 
	OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_XNAME_COLOR, OGLVAL_YELLOW);
	OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_YNAME_COLOR, OGLVAL_YELLOW);
	OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_YZ_GRID_VISIBLE, 1);
	OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_XY_GRID_VISIBLE, 1);
	OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_XZ_GRID_VISIBLE, 1);
	OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_YZ_GRID_COLOR, OGLVAL_LT_GRAY); 
	OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_XY_GRID_COLOR, OGLVAL_LT_GRAY);
	OGLSetCtrlAttribute (mainPanel, OGLControlID, OGLATTR_XZ_GRID_COLOR, OGLVAL_LT_GRAY);
 
	LogMessage("OGL control initialized successfully."); 
}

//----------------------------------------------------------------------------
//  Renders the image to the OGL control.
//----------------------------------------------------------------------------
void RenderImage(int fastFlag)
{
	GLfloat specularLight0[] = {1.0f, 1.0f, 1.0f, 1.0f};

	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	{
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		{
			glEnable(GL_DEPTH_TEST);
			glShadeModel(GL_SMOOTH);

			glEnable(GL_COLOR_MATERIAL);
			glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularLight0);
			glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 128);

			DrawMissileBody(fastFlag);
		}
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
	}
	glPopMatrix();
	glPopAttrib();
	glFlush();
}

//----------------------------------------------------------------------------
//  Draws the OGL missile in its entirety
//----------------------------------------------------------------------------
void DrawMissileBody(int fastFlag) 
{
	// Initial position
	glRotatef(90.0, 1.0, 0.0, 0.0);
	glRotatef(90.0, 0.0, 1.0, 0.0);
	
	// Apply yaw, pitch, and roll rotations to the entire plane
	glRotatef(pitch, 1.0, 0.0, 0.0); // Pitch
	glRotatef(yaw, 0.0, 1.0, 0.0);   // Yaw
	glRotatef(roll, 0.0, 0.0, 1.0);  // Roll
	
	object = gluNewQuadric(); 

	glColor3f(0.9f, 0.9f, 0.0f);

	// Draw in "Line" mode for speed
	if (fastFlag)
		gluQuadricDrawStyle(object,GLU_LINE);

	// Draw the main body
	glPushMatrix();
	{
		glTranslatef(0.0, 0.0, -0.04);
		glScalef(1.0, 1.0, 5.0);
		gluSphere(object, 0.05, DETAIL_LVL, DETAIL_LVL);
	}
	glPopMatrix();

	// Draw the wings
	glPushMatrix();
	{
		glTranslatef(-0.3, 0.0, 0.0);
		glRotatef(90.0, 0.0, 1.0, 0.0);
		glScalef(5.0, 1.0, 1.0);
		DrawClosedCylinder(object, 0.01, 0.6);
	}
	glPopMatrix();

	// Draw the vertical fin
	glPushMatrix();
	{
		glTranslatef(0.0, 0.0, 0.2);
		glRotatef(-90.0, 1.0, 0.0, 0.0);
		glScalef(1.0, 5.0, 1.0);
		DrawClosedCylinder(object, 0.01, 0.1);
	}
	glPopMatrix();

	// Draw the horizontal fin
	glPushMatrix();
	{
		glTranslatef(-0.125, 0.0, 0.2);
		glRotatef(90.0, 0.0, 1.0, 0.0);
		glScalef(5.0, 1.0, 1.0);
		DrawClosedCylinder(object, 0.01, 0.25);
	}
	glPopMatrix();

	gluDeleteQuadric(object);
}

//----------------------------------------------------------------------------
//  Auxiliary function to draw a closed cylinder
//----------------------------------------------------------------------------
void DrawClosedCylinder(GLUquadricObj *object, float radius, float height)
{
	glEnable(GL_NORMALIZE);
	glPushMatrix();
	{
		// Draw the main body (cylinder)
		gluCylinder(object, radius, radius, height, DETAIL_LVL, DETAIL_LVL);
		// Draw the bottom cap as a scaled sphere
		glScalef(1.0, 1.0, 0.05);
		gluSphere(object, radius, DETAIL_LVL, DETAIL_LVL);
	}
	glPopMatrix();

	glPushMatrix();
	{
		// Draw the top cap as a scaled sphere
		glTranslatef(0.0, 0.0, height); // Move to the top of the cylinder
		glScalef(1.0, 1.0, 0.05);
		gluSphere(object, radius, DETAIL_LVL, DETAIL_LVL);
	}
	glPopMatrix();
	glDisable(GL_NORMALIZE);
}

//----------------------------------------------------------------------------
//  Required by CVI for image refreshes and paints
//----------------------------------------------------------------------------
int CVICALLBACK OGLCallback (int panel, int control, int event,
        void *callbackData, int eventData1, int eventData2)
{
    switch (event) 
	{
        case OGLEVENT_REFRESH:
            // Render the image when REFRESH event is received
            RenderImage(eventData1);
            break;
    }
    return 0;
}
