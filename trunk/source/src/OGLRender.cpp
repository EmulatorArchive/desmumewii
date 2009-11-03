/*
	Copyright (C) 2006 yopyop
	Copyright (C) 2006-2007 shash

    This file is part of DeSmuME

    DeSmuME is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DeSmuME is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DeSmuME; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

//problem - alpha-on-alpha texture rendering might work but the dest alpha buffer isnt tracked correctly
//due to zeromus not having any idea how to set dest alpha blending in opengl.
//so, it doesnt composite to 2d correctly.
//(re: new super mario brothers renders the stormclouds at the beginning)

#include "OGLRender.h"
#include "debug.h"

bool (*oglrender_init)() = 0;
bool (*oglrender_beginOpenGL)() = 0;
void (*oglrender_endOpenGL)() = 0;

//Stuff GX will need

static void *frameBuffer[2] = { NULL, NULL};
GXRModeObj *rmode;

/** Open GL Stuff From Profetylen**/

// Static variables
static bool glHasBegun=false;

/*
	The following 2 functions is not finsished.
*/
void glBegin()
{
	glHasBegun=true;
}

void glEnd()
{
	glHasBegun=false;
}


// Shader stuff *************************************************************************************

// OpenGL enumerators
GLenum GL_VERTEX_SHADER=0;
GLenum GL_FRAGMENT_SHADER_=1;

GLenum SHADER_OBJECT=2;
GLenum PROGRAM_OBJECT=3;

GLenum GL_SHADER_TYPE=4;
GLenum GL_DELETE_STATUS=5;
GLenum GL_COMPILE_STATUS_=6;
GLenum GL_INFO_LOG_LENGTH_=7;
GLenum GL_SHADER_SOURCE_LENGTH=8;

//GLenum GL_DELETE_STATUS=9;
GLenum GL_LINK_STATUS_=10;
GLenum GL_VALIDATE_STATUS=11;
//GLenum GL_INFO_LOG_LENGTH=12;
GLenum GL_ATTACHED_SHADERS=13;
GLenum GL_ACTIVE_ATTRIBUTES=14;
GLenum GL_ACTIVE_ATTRIBUTE_MAX_LENGTH=15;
GLenum GL_ACTIVE_UNIFORMS=16;
GLenum GL_ACTIVE_UNIFORM_MAX_LENGTH=17;

/*
	"error: expected unqualified-id before numeric constant" was given with the
	name GL_FRAGMENT_SHADER, GL_COMPILE_STATUS and GL_INFO_LOG_LENGTH, GLenum GL_LINK_STATUS, so I added
	a "_" after each of those enums.
*/


/*
	Creates a shader object with a specified type.
*/
GLuint glCreateShader(GLenum type)
{
	GLuint returnValue=0;
	bool createObject=true;
	if ((type!=GL_VERTEX_SHADER)&&(type!=GL_FRAGMENT_SHADER))
	{
		// This line should be replaced by a line that generates error: GL_INVALID_ENUM
		createObject=false;
	}
	if (glHasBegun)
	{
		// This line should be replaced by a line that generates error: GL_INVALID_OPERATION
		createObject=false;
	}
	if (createObject)
	{
		Shader* shader = new Shader(type);
		if (shader!=NULL)
		{
			returnValue=(GLuint) shader;
		}
	}
	return returnValue;
}

/*
	Deletes a shader object if it isn't attached to a program (The latter is not implemeted yet).
*/
void glDeleteShader(GLuint shader)
{
	// Abort if it's a null shader object
	if (shader==0)
	{
		return;
	}
	Shader* shaderObject=(Shader*) shader;
	delete shaderObject;
	return;
}

/*
	Returns to params the given parameter to return from the
	given shader.
*/
void glGetShaderiv(GLuint shader,GLenum pname,GLint* params)
{
	if (glHasBegun)
	{
		// This line should be replaced by a line that generates error: GL_INVALID_OPERATION
		return;
	}
	Shader* shaderObject=(Shader*) shader;
	if (pname==GL_SHADER_TYPE)
	{
		*params=shaderObject->getType();
	} else if (pname==GL_DELETE_STATUS)
	{
		*params=shaderObject->getDeletionFlag();
	} else if (pname==GL_COMPILE_STATUS)
	{
		// Not yet implemented because glCompileShader is not done.
	} else if (pname==GL_INFO_LOG_LENGTH)
	{
		// Not yet implemented because glGetInfoLog is not done.
	}else if (pname==GL_SHADER_SOURCE_LENGTH)
	{
		// Not yet implemented because glShaderSource is not done.
	} else
	{
		// This line should be replaced by a line that generates error: GL_INVALID_ENUM
	}
	// TODO: Add code that checks for object type errors or non openGL values.
	return;
}

/*
	Replaces the source code in the shader object.
	I don't know how to do this at all.
*/
void glShaderSource(GLuint shader,GLsizei count,const GLchar** string,GLint* length)
{
	return;
}

/*
	Compiles the source code in the shader object.
	I don't know how to do this at all.
*/
void glCompileShader(GLuint shader)
{
	return;
}

/*
	Returns the infolog of a shader object to GLchar* infoLog
	I don't know how to do this because glShaderSource(...) and
	glCompileShader(...) isn't done.
*/
void glGetShaderInfoLog(GLuint shader,GLsizei maxLength,GLint* length,GLchar* infoLog)
{
	return;
}

// Program stuff ************************************************************************************

/*
	Links the program with the help of the shaders that are attached
	to it.
	I don't know how to do this at all.
*/
void glLinkProgram(GLuint shader)
{
	return;
}

/*
	Returns the infolog of a shader object to GLchar* infoLog
	I don't know how to do this because glShaderSource(...) and
	glCompileShader(...) isn't done.
*/
void glGetProgramInfoLog(GLuint program,GLsizei maxLength,GLint* length,GLchar* infoLog)
{
	return;
}

/*
	Creates a shader object and returns a reference to it cast to
	a GLuint.
*/
GLuint glCreateProgram()
{
	GLuint returnValue=0;
	bool createObject=true;
	if (glHasBegun)
	{
		// This line should be replaced by a line that generates error: GL_INVALID_OPERATION
		createObject=false;
	}
	if (createObject)
	{
		Program* program = new Program();
		if (program!=NULL)
		{
			returnValue=(GLuint) program;
		}
	}
	return returnValue;
}

/*
	Deletes a program object if it isn't in use (The latter is not implemeted yet).
*/
void glDeleteProgram(GLuint program)
{
	bool deleteObject=true;
	// Abort if it's a null program object
	if (program==0)
	{
		return;
	}
	if (glHasBegun)
	{
		// This line should be replaced by a line that generates error: GL_INVALID_OPERATION
		deleteObject=false;
	}
	if (deleteObject)
	{
		Program* programObject=(Program*) program;
		if (programObject->isDeletable())
		{
			delete programObject;
		} else
		{
			programObject->setDeletionFlag();
		}
	} 
	return;
}

/*
	Attaches a shader to a program.
*/

void glAttachShader(GLuint program,GLuint shader)
{
	Program* programObject=(Program*) program;
	bool perform=true;
	if (glHasBegun)
	{
		// This line should be replaced by a line that generates error: GL_INVALID_OPERATION
		perform=false;
	}
	if (programObject->shaderIsAttached(shader))
	{
		// This line should be replaced by a line that generates error: GL_INVALID_OPERATION
		perform=false;
	}
	// TODO: Add code that checks for object type errors or non openGL values.
	if (perform)
	{
		programObject->attachShader(shader);
	}
}

/*
	Detaches a shader from a program.
*/

void glDetachShader(GLuint program,GLuint shader)
{
	Program* programObject=(Program*) program;
	bool perform=true;
	if (glHasBegun)
	{
		// This line should be replaced by a line that generates error: GL_INVALID_OPERATION
		perform=false;
	}
	if (programObject->shaderIsAttached(shader))
	{
		// This line should be replaced by a line that generates error: GL_INVALID_OPERATION
		perform=false;
	}
	// TODO: Add code that checks for object type errors or non openGL values.
	if (perform)
	{
		programObject->detachShader(shader);
	}
}

/*
	Returns to params the given parameter to return from the
	given program.
*/
void glGetProgramiv(GLuint program,GLenum pname,GLint* params)
{
	if (glHasBegun)
	{
		// This line should be replaced by a line that generates error: GL_INVALID_OPERATION
		return;
	}
	Program* programObject=(Program*) program;
	if (pname==GL_DELETE_STATUS)
	{
		*params=programObject->getDeletionFlag();
	} else if (pname==GL_LINK_STATUS)
	{
		// Not yet implemented.
	} else if (pname==GL_COMPILE_STATUS)
	{
		// Not yet implemented.
	} else if (pname==GL_VALIDATE_STATUS)
	{
		// Not yet implemented.
	}else if (pname==GL_INFO_LOG_LENGTH)
	{
		// Not yet implemented.
	}else if (pname==GL_ATTACHED_SHADERS)
	{
		*params=programObject->getNumberOfAttachedShaders();
	}else if (pname==GL_ACTIVE_ATTRIBUTES)
	{
		// Not yet implemented.
	}else if (pname==GL_ACTIVE_ATTRIBUTE_MAX_LENGTH)
	{
		// Not yet implemented.
	}else if (pname==GL_ACTIVE_UNIFORMS)
	{
		// Not yet implemented.
	}else if (GL_ACTIVE_UNIFORM_MAX_LENGTH)
	{
		// Not yet implemented.
	} else
	{
		// This line should be replaced by a line that generates error: GL_INVALID_ENUM
	}
	// TODO: Add code that checks for object type errors or non openGL values.
	return;
}

/*
	Checks to see whether the executables contained in program
	can execute given the current OpenGL state.
	I don't know how to do this.
*/
void glValidateProgram(GLuint program)
{
	return;
}

/*
	Installs the program object specified by program as part
	of current rendering state.
	I don't know how to do this.
*/
void glUseProgram(GLuint program)
{
	return;
}

/**End OpenGL Stuff from Profetylen**/


static bool BEGINGL() {
	if(oglrender_beginOpenGL){
	//This function is called just before rendering anything
	
	GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
		
	}
	
	else return true;
	
}

static void ENDGL() {
	if(oglrender_endOpenGL){
	    //This function is called after rendering is finished
		
		GX_DrawDone();
		
		VIDEO_Flush();
 
		VIDEO_WaitVSync();
	}
}

//Thanks for the following functions profetlyn
const char* glGetString(GLenum name)
{
	const char* returnString="ERROR";
	if (name==GL_EXTENSIONS)
	{
		returnString="";
	}
	return returnString;
}

// profetlyn: glEnable and glDisable perhaps to be filled with more things to enable / disable
void glEnable(GLenum cap)
{
	if (cap==GL_DEPTH_TEST)
	{
		depthTestEnabled=true;
	}
}

void glDisable(GLenum cap)
{
	if (cap==GL_DEPTH_TEST)
	{
		depthTestEnabled=false;
	}
}

// profetlyn: Sets the function to use in depth comparisions.
void glDepthFunc(GLenum func)
{
	currentDepthFunction=func;
}

// profetlyn: Enables or disables wring do the depth buffer.
void glDepthMask(bool flag)
{
	depthBufferWritingEnabled=flag;
}

/*profetlyn:
	Sets polygon face and polygon mode.
	
	Face specifies wheter only the backs, only the fronts or both of the backs
	and the fronts of the polygons will be drawn. I didn't find a GX constant
	for this so this isn't implemented in the function (at least not yet).
	
	Mode specifies how the polygon is drawn. I set GX_POINTS as default and
	I am not sure it corresponds to GL_POINT. I don't even think so because there
	is also a constant GL_POINTS.
*/
void glPolygonMode(GLenum face,GLenum mode)
{
	currentPolygonMode=mode;
}

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <GL/gl.h>
	#include <GL/glext.h>
#else
#ifdef DESMUME_COCOA
	#include <OpenGL/gl.h>
	#include <OpenGL/glext.h>
#else
//	#include <GL/gl.h> This is replaced by libogc's GX
//	#include <GL/glext.h> This is replaced by libogc's GX
#endif
#endif

#include "types.h"
#include "debug.h"
#include "MMU.h"
#include "bits.h"
#include "matrix.h"
#include "NDSSystem.h"
#include "OGLRender.h"
#include "gfx3d.h"

#include "shaders.h"
#include "texcache.h"



#ifndef CTASSERT
#define	CTASSERT(x)		typedef char __assert ## y[(x) ? 1 : -1]
#endif

static ALIGN(16) u8  GPU_screen3D			[256*192*4];


static const unsigned short map3d_cull[4] = {GX_FRONT_AND_BACK, GX_FRONT, GX_BACK, 0};
static const int texEnv[4] = { GX_MODULATE, GX_DECAL, GX_MODULATE, GX_MODULATE };
static const int depthFunc[2] = { GX_LESS, GX_EQUAL };

//derived values extracted from polyattr etc
static bool wireframe=false, alpha31=false;
static unsigned int polyID=0;
static unsigned int depthFuncMode=0;
static unsigned int envMode=0;
static unsigned int lastEnvMode=0;
static unsigned int cullingMask=0;
static bool alphaDepthWrite;
static unsigned int lightMask=0;
static bool isTranslucent;

static u32 textureFormat=0, texturePalette=0;

//------------------------------------------------------------

#define OGLEXT(x,y) x y = 0;

#ifdef _WIN32
#define INITOGLEXT(x,y) y = (x)wglGetProcAddress(#y);
#elif !defined(DESMUME_COCOA)
//#include <GL/glx.h> this is replaced by libogc's GX
#define INITOGLEXT(x,y) y = (x)glXGetProcAddress((const GLubyte *) #y);
#endif

#ifndef DESMUME_COCOA
//Arikado: I've commented out the below section because I have no idea what it does
// OGLEXT is an extension of OpenGL but I have no idea what the original authors were attempting to use it for

/*OGLEXT(PFNGLCREATESHADERPROC,glCreateShader)
//zero: i dont understand this at all. my glext.h has the wrong thing declared here... so I have to do it myself
typedef void (APIENTRYP X_PFNGLGETSHADERSOURCEPROC) (GLuint shader, GLsizei bufSize, const GLchar **source, GLsizei *length);
OGLEXT(X_PFNGLGETSHADERSOURCEPROC,glShaderSource)
OGLEXT(PFNGLCOMPILESHADERPROC,glCompileShader)
OGLEXT(PFNGLCREATEPROGRAMPROC,glCreateProgram)
OGLEXT(PFNGLATTACHSHADERPROC,glAttachShader)
OGLEXT(PFNGLDETACHSHADERPROC,glDetachShader)
OGLEXT(PFNGLLINKPROGRAMPROC,glLinkProgram)
OGLEXT(PFNGLUSEPROGRAMPROC,glUseProgram)
OGLEXT(PFNGLGETSHADERIVPROC,glGetShaderiv)
OGLEXT(PFNGLGETSHADERINFOLOGPROC,glGetShaderInfoLog)
OGLEXT(PFNGLDELETESHADERPROC,glDeleteShader)
OGLEXT(PFNGLDELETEPROGRAMPROC,glDeleteProgram)
OGLEXT(PFNGLGETPROGRAMIVPROC,glGetProgramiv)
OGLEXT(PFNGLGETPROGRAMINFOLOGPROC,glGetProgramInfoLog)
OGLEXT(PFNGLVALIDATEPROGRAMPROC,glValidateProgram)
OGLEXT(PFNGLBLENDFUNCSEPARATEEXTPROC,glBlendFuncSeparateEXT)
OGLEXT(PFNGLGETUNIFORMLOCATIONPROC,glGetUniformLocation)
OGLEXT(PFNGLUNIFORM1IPROC,glUniform1i)
OGLEXT(PFNGLUNIFORM1IVPROC,glUniform1iv)*/
#endif

#if !defined(GL_VERSION_1_3) || defined(_MSC_VER) || defined(__INTEL_COMPILER)
//OGLEXT(PFNGLACTIVETEXTUREPROC,glActiveTexture)
#endif


//opengl state caching:
//This is of dubious performance assistance, but it is easy to take out so I am leaving it for now.
//every function that is xgl* can be replaced with gl* if we decide to rip this out or if anyone else
//doesnt feel like sticking with it (or if it causes trouble)

static void xglDepthFunc(GLenum func) {
	static GLenum oldfunc = -1;
	if(oldfunc == func) return;
	glDepthFunc(oldfunc=func);
}

static void xglPolygonMode(GLenum face,GLenum mode) {
	static GLenum oldmodes[2] = {-1,-1};
	switch(face) {
		case GX_FRONT: if(oldmodes[0]==mode) return; else glPolygonMode(GX_FRONT,oldmodes[0]=mode); return;
		case GX_BACK: if(oldmodes[1]==mode) return; else glPolygonMode(GX_BACK,oldmodes[1]=mode); return;
		case GX_FRONT_AND_BACK: if(oldmodes[0]==mode && oldmodes[1]==mode) return; else glPolygonMode(GX_FRONT_AND_BACK,oldmodes[0]=oldmodes[1]=mode);
	}
}

#if 0
#ifdef _WIN32
static void xglUseProgram(GLuint program) {
	if(!glUseProgram) return;
	static GLuint oldprogram = -1;
	if(oldprogram==program) return;
	glUseProgram(oldprogram=program);
} 
#else
#if 0 /* not used */
static void xglUseProgram(GLuint program) {
	(void)program;
	return;
}
#endif
#endif
#endif

static void xglDepthMask (GLboolean flag) {
	static GLboolean oldflag = -1;
	if(oldflag==flag) return;
	glDepthMask(oldflag=flag);
}

struct GLCaps {
	u8 caps[0x100];
	GLCaps() {
		memset(caps,0xFF,sizeof(caps));
	}
};
static GLCaps glcaps;

static void _xglEnable(GLenum cap) {
	cap -= 0x0B00;
	if(glcaps.caps[cap] == 0xFF || glcaps.caps[cap] == 0) {
		glEnable(cap+0x0B00);
		glcaps.caps[cap] = 1;
	}
}

static void _xglDisable(GLenum cap) {
	cap -= 0x0B00;
	if(glcaps.caps[cap]) {
		glDisable(cap+0x0B00);
		glcaps.caps[cap] = 0;
	}
}

#define xglEnable(cap) { \
	CTASSERT((cap-0x0B00)<0x100); \
	_xglEnable(cap); }

#define xglDisable(cap) {\
	CTASSERT((cap-0x0B00)<0x100); \
	_xglDisable(cap); }



GLenum			oglTempTextureID[MAX_TEXTURE];
GLenum			oglToonTableTextureID;

#define NOSHADERS(s)					{ hasShaders = false; INFO("Shaders aren't supported on your system, using fixed pipeline\n(%s)\n", s); return; }

#define SHADER_COMPCHECK(s, t)				{ \
	GLint status = GL_TRUE; \
	glGetShaderiv(s, GL_COMPILE_STATUS, &status); \
	if(status != GL_TRUE) \
	{ \
		GLint logSize; \
		GLchar *log; \
		glGetShaderiv(s, GL_INFO_LOG_LENGTH, &logSize); \
		log = new GLchar[logSize]; \
		glGetShaderInfoLog(s, logSize, &logSize, log); \
		INFO("SEVERE : FAILED TO COMPILE GL SHADER : %s\n", log); \
		delete log; \
		if(s)glDeleteShader(s); \
		NOSHADERS("Failed to compile the "t" shader."); \
	} \
}

#define PROGRAM_COMPCHECK(p, s1, s2)	{ \
	GLint status = GL_TRUE; \
	glGetProgramiv(p, GL_LINK_STATUS, &status); \
	if(status != GL_TRUE) \
	{ \
		GLint logSize; \
		GLchar *log; \
		glGetProgramiv(p, GL_INFO_LOG_LENGTH, &logSize); \
		log = new GLchar[logSize]; \
		glGetProgramInfoLog(p, logSize, &logSize, log); \
		INFO("SEVERE : FAILED TO LINK GL SHADER PROGRAM : %s\n", log); \
		delete log; \
		if(s1)glDeleteShader(s1); \
		if(s2)glDeleteShader(s2); \
		NOSHADERS("Failed to link the shader program."); \
	} \
}

bool hasShaders = false;

/* Vertex shader */
GLuint vertexShaderID;
/* Fragment shader */
GLuint fragmentShaderID;
/* Shader program */
GLuint shaderProgram;

static GLuint hasTexLoc;
static GLuint texBlendLoc;
static bool hasTexture = false;

/* Shaders init */

static void createShaders()
{
	hasShaders = true;

#ifdef HAVE_LIBOSMESA
	NOSHADERS("Shaders aren't supported by OSMesa.");
#endif

	/* This check is just plain wrong. */
	/* It will always pass if you've OpenGL 2.0 or later, */
	/* even if your GFX card doesn't support shaders. */
/*	if (glCreateShader == NULL ||  //use ==NULL instead of !func to avoid always true warnings for some systems
		glShaderSource == NULL ||
		glCompileShader == NULL ||
		glCreateProgram == NULL ||
		glAttachShader == NULL ||
		glLinkProgram == NULL ||
		glUseProgram == NULL ||
		glGetShaderInfoLog == NULL)
		NOSHADERS("Shaders aren't supported by your system.");*/

	const char *extString = (const char*)glGetString(GL_EXTENSIONS);
	if ((strstr(extString, "GL_ARB_shader_objects") == NULL) ||
		(strstr(extString, "GL_ARB_vertex_shader") == NULL) ||
		(strstr(extString, "GL_ARB_fragment_shader") == NULL))
		NOSHADERS("Shaders aren't supported by your system.");

	vertexShaderID = glCreateShader(GX_VERTEX_SHADER);
	if(!vertexShaderID)
		NOSHADERS("Failed to create the vertex shader.");

	glShaderSource(vertexShaderID, 1, (const GLchar**)&vertexShader, NULL);
	glCompileShader(vertexShaderID);
	SHADER_COMPCHECK(vertexShaderID, "vertex");

	fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	if(!fragmentShaderID)
		NOSHADERS("Failed to create the fragment shader.");

	glShaderSource(fragmentShaderID, 1, (const GLchar**)&fragmentShader, NULL);
	glCompileShader(fragmentShaderID);
	SHADER_COMPCHECK(fragmentShaderID, "fragment");

	shaderProgram = glCreateProgram();
	if(!shaderProgram)
		NOSHADERS("Failed to create the shader program.");

	glAttachShader(shaderProgram, vertexShaderID);
	glAttachShader(shaderProgram, fragmentShaderID);

	glLinkProgram(shaderProgram);
	PROGRAM_COMPCHECK(shaderProgram, vertexShaderID, fragmentShaderID);

	glValidateProgram(shaderProgram);
	glUseProgram(shaderProgram);

	INFO("Successfully created OpenGL shaders.\n");
}

//=================================================

static void OGLReset()
{
	if(hasShaders)
	{
		glUniform1i(hasTexLoc, 0);
		hasTexture = false;
		glUniform1i(texBlendLoc, 0);
		
	}

	TexCache_Reset();

	for (int i = 0; i < MAX_TEXTURE; i++)
		texcache[i].id=oglTempTextureID[i];

//	memset(GPU_screenStencil,0,sizeof(GPU_screenStencil));
	memset(GPU_screen3D,0,sizeof(GPU_screen3D));
}




static void BindTexture(u32 tx)
{
	glBindTexture(GL_TEXTURE_2D,(GLuint)texcache[tx].id);
	glMatrixMode (GL_TEXTURE);
	glLoadIdentity ();
	glScaled (texcache[tx].invSizeX, texcache[tx].invSizeY, 1.0f);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (BIT16(texcache[tx].frm) ? (BIT18(texcache[tx].frm)?GL_MIRRORED_REPEAT:GL_REPEAT) : GL_CLAMP));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (BIT17(texcache[tx].frm) ? (BIT19(texcache[tx].frm)?GL_MIRRORED_REPEAT:GL_REPEAT) : GL_CLAMP));
}

static void BindTextureData(u32 tx, u8* data)
{
	BindTexture(tx);

#if 0
	for (int i=0; i < texcache[tx].sizeX * texcache[tx].sizeY*4; i++)
		data[i] = 0xFF;
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
		texcache[tx].sizeX, texcache[tx].sizeY, 0, 
		GL_RGBA, GL_UNSIGNED_BYTE, data);
}


static char OGLInit(void)
//This function initializes and creates the variables used for rendering with OpenGL
{
	GLuint loc = 0;

	if(!oglrender_init)
		return 0;
	if(!oglrender_init())
		return 0;

	if(!BEGINGL())
		return 0;

	TexCache_BindTexture = BindTexture;
	TexCache_BindTextureData = BindTextureData;
	glGenTextures (MAX_TEXTURE, &oglTempTextureID[0]);

	glPixelStorei(GL_PACK_ALIGNMENT,8);

	xglEnable		(GL_NORMALIZE);
	xglEnable		(GL_DEPTH_TEST);
	glEnable		(GL_TEXTURE_1D);
	glEnable		(GL_TEXTURE_2D);

	glAlphaFunc		(GL_GREATER, 0);
	xglEnable		(GL_ALPHA_TEST);

	glViewport(0, 0, 256, 192);
	if (glGetError() != GL_NO_ERROR)
		return 0;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifndef DESMUME_COCOA

/*	Arikado: Again, I have no idea what the point of this is

    INITOGLEXT(PFNGLCREATESHADERPROC,glCreateShader)
	INITOGLEXT(X_PFNGLGETSHADERSOURCEPROC,glShaderSource)
	INITOGLEXT(PFNGLCOMPILESHADERPROC,glCompileShader)
	INITOGLEXT(PFNGLCREATEPROGRAMPROC,glCreateProgram)
	INITOGLEXT(PFNGLATTACHSHADERPROC,glAttachShader)
	INITOGLEXT(PFNGLDETACHSHADERPROC,glDetachShader)
	INITOGLEXT(PFNGLLINKPROGRAMPROC,glLinkProgram)
	INITOGLEXT(PFNGLUSEPROGRAMPROC,glUseProgram)
	INITOGLEXT(PFNGLGETSHADERIVPROC,glGetShaderiv)
	INITOGLEXT(PFNGLGETSHADERINFOLOGPROC,glGetShaderInfoLog)
	INITOGLEXT(PFNGLDELETESHADERPROC,glDeleteShader)
	INITOGLEXT(PFNGLDELETEPROGRAMPROC,glDeleteProgram)
	INITOGLEXT(PFNGLGETPROGRAMIVPROC,glGetProgramiv)
	INITOGLEXT(PFNGLGETPROGRAMINFOLOGPROC,glGetProgramInfoLog)
	INITOGLEXT(PFNGLVALIDATEPROGRAMPROC,glValidateProgram) */
#ifdef HAVE_LIBOSMESA
	glBlendFuncSeparateEXT = NULL;
#else
//	INITOGLEXT(PFNGLBLENDFUNCSEPARATEEXTPROC,glBlendFuncSeparateEXT)
#endif
//	INITOGLEXT(PFNGLGETUNIFORMLOCATIONPROC,glGetUniformLocation)
//	INITOGLEXT(PFNGLUNIFORM1IPROC,glUniform1i)
//	INITOGLEXT(PFNGLUNIFORM1IVPROC,glUniform1iv)
#endif
#if !defined(GL_VERSION_1_3) || defined(_MSC_VER) || defined(__INTEL_COMPILER)
//	INITOGLEXT(PFNGLACTIVETEXTUREPROC,glActiveTexture)
#endif

	/* Create the shaders */
	createShaders();

	/* Assign the texture units : 0 for main textures, 1 for toon table */
	/* Also init the locations for some variables in the shaders */
	if(hasShaders)
	{
		loc = glGetUniformLocation(shaderProgram, "tex2d");
		glUniform1i(loc, 0);

		loc = glGetUniformLocation(shaderProgram, "toonTable");
		glUniform1i(loc, 1);

		hasTexLoc = glGetUniformLocation(shaderProgram, "hasTexture");

		texBlendLoc = glGetUniformLocation(shaderProgram, "texBlending");
	}

	//we want to use alpha destination blending so we can track the last-rendered alpha value
	if(glBlendFuncSeparateEXT != NULL)
	{
		glBlendFuncSeparateEXT(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_DST_ALPHA);
	//	glBlendFuncSeparateEXT(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	}

	if(hasShaders)
	{
		glGenTextures (1, &oglToonTableTextureID);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_1D, oglToonTableTextureID);
		glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP); //clamp so that we dont run off the edges due to 1.0 -> [0,31] math
	}

	OGLReset();

	ENDGL();

	return 1;
}

static void OGLClose()
//This functions gets rid of the shaders and textures
{
	if(!BEGINGL())
		return;

	if(hasShaders)
	{
		glUseProgram(0);

		glDetachShader(shaderProgram, vertexShaderID);
		glDetachShader(shaderProgram, fragmentShaderID);

		glDeleteProgram(shaderProgram);
		glDeleteShader(vertexShaderID);
		glDeleteShader(fragmentShaderID);

		hasShaders = false;
	}

	glDeleteTextures(MAX_TEXTURE, &oglTempTextureID[0]);
	glDeleteTextures(1, &oglToonTableTextureID);

	ENDGL();
}

static void setTexture(unsigned int format, unsigned int texpal)
//This functin sets textures to polygons
{
	textureFormat = format;
	texturePalette = texpal;

	u32 textureMode = (unsigned short)((format>>26)&0x07);

	if (format==0)
	{
		if(hasShaders && hasTexture) { glUniform1i(hasTexLoc, 0); hasTexture = false; }
		return;
	}
	if (textureMode==0)
	{
		if(hasShaders && hasTexture) { glUniform1i(hasTexLoc, 0); hasTexture = false; }
		return;
	}

	if(hasShaders)
	{
		if(!hasTexture) { glUniform1i(hasTexLoc, 1); hasTexture = true; }
		glActiveTexture(GL_TEXTURE0);
	}


	TexCache_SetTexture<TexFormat_32bpp>(format, texpal);
}



//controls states:
//glStencilFunc
//glStencilOp
//glColorMask
static u32 stencilStateSet = -1;

static u32 polyalpha=0;

static void BeginRenderPoly()
//This function renders the polygons
{
	bool enableDepthWrite = true;

	xglDepthFunc (depthFuncMode);

	// Cull face
	if (cullingMask != 0xC0)
	{
		xglEnable(GL_CULL_FACE);
		glCullFace(map3d_cull[cullingMask>>6]);
	}
	else
		xglDisable(GL_CULL_FACE);

	if (!wireframe)
	{
		xglPolygonMode (GX_FRONT_AND_BACK, GL_FILL);
	}
	else
	{
		xglPolygonMode (GX_FRONT_AND_BACK, GL_LINE);
	}

	setTexture(textureFormat, texturePalette);

	if(isTranslucent)
		enableDepthWrite = alphaDepthWrite;

	//handle shadow polys
	if(envMode == 3)
	{
		xglEnable(GL_STENCIL_TEST);
		if(polyID == 0) {
			enableDepthWrite = false;
			if(stencilStateSet!=0) {
				stencilStateSet = 0;
				//when the polyID is zero, we are writing the shadow mask.
				//set stencilbuf = 1 where the shadow volume is obstructed by geometry.
				//do not write color or depth information.
				glStencilFunc(GL_ALWAYS,65,255);
				glStencilOp(GL_KEEP,GL_REPLACE,GL_KEEP);
				glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
			}
		} else {
			enableDepthWrite = true;
			if(stencilStateSet!=1) {
				stencilStateSet = 1;
				//when the polyid is nonzero, we are drawing the shadow poly.
				//only draw the shadow poly where the stencilbuf==1.
				//I am not sure whether to update the depth buffer here--so I chose not to.
				glStencilFunc(GL_EQUAL,65,255);
				glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
				glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
			}
		}
	} else {
		xglEnable(GL_STENCIL_TEST);
		if(isTranslucent)
		{
			stencilStateSet = 3;
			glStencilFunc(GL_NOTEQUAL,polyID,255);
			glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
			glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
		}
		else
		if(stencilStateSet!=2) {
			stencilStateSet=2;	
			glStencilFunc(GL_ALWAYS,64,255);
			glStencilOp(GL_REPLACE,GL_REPLACE,GL_REPLACE);
			glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
		}
	}

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texEnv[envMode]);

	if(hasShaders)
	{
		if(envMode != lastEnvMode)
		{
			lastEnvMode = envMode;

			int _envModes[4] = {0, 1, (2 + gfx3d.shading), 0};
			glUniform1i(texBlendLoc, _envModes[envMode]);
		}
	}

	xglDepthMask(enableDepthWrite?GX_TRUE:GX_FALSE);
}

static void InstallPolygonAttrib(unsigned long val)
//Adjust polygons for rendering
{
	// Light enable/disable
	lightMask = (val&0xF);

	// texture environment
	envMode = (val&0x30)>>4;

	// overwrite depth on alpha pass
	alphaDepthWrite = BIT11(val)!=0;

	// depth test function
	depthFuncMode = depthFunc[BIT14(val)];

	// back face culling
	cullingMask = (val&0xC0);

	alpha31 = ((val>>16)&0x1F)==31;
	
	// Alpha value, actually not well handled, 0 should be wireframe
	wireframe = ((val>>16)&0x1F)==0;

	polyalpha = ((val>>16)&0x1F);

	// polyID
	polyID = (val>>24)&0x3F;
}

static void Control()
{
	if(gfx3d.enableTexturing) glEnable (GL_TEXTURE_2D);
	else glDisable (GL_TEXTURE_2D);

	if(gfx3d.enableAlphaTest)
		glAlphaFunc	(GL_GREATER, gfx3d.alphaTestRef/31.f);
	else
		glAlphaFunc	(GL_GREATER, 0);

	if(gfx3d.enableAlphaBlending)
	{
		glEnable		(GX_BLEND);
	}
	else
	{
		glDisable		(GX_BLEND);
	}
}


static void GL_ReadFramebuffer()
{
	if(!BEGINGL()) return; 
	GX_End();
//	glReadPixels(0,0,256,192,GL_STENCIL_INDEX,		GL_UNSIGNED_BYTE,	GPU_screenStencil);
	glReadPixels(0,0,256,192,GL_BGRA,			GL_UNSIGNED_BYTE,	GPU_screen3D);	
	ENDGL();

	//convert the pixels to a different format which is more convenient
	//is it safe to modify the screen buffer? if not, we could make a temp copy
	for(int i=0,y=191;y>=0;y--)
	{
		u16* dst = gfx3d_convertedScreen + (y<<8);
		u8* dstAlpha = gfx3d_convertedAlpha + (y<<8);

		#ifndef NOSSE2
			//I dont know much about this kind of stuff, but this seems to help
			//for some reason I couldnt make the intrinsics work 
			u8* wanx =  (u8*)&((u32*)GPU_screen3D)[i];
			#define ASS(X,Y) __asm { prefetchnta [wanx+32*0x##X##Y] }
			#define PUNK(X) ASS(X,0) ASS(X,1) ASS(X,2) ASS(X,3) ASS(X,4) ASS(X,5) ASS(X,6) ASS(X,7) ASS(X,8) ASS(X,9) ASS(X,A) ASS(X,B) ASS(X,C) ASS(X,D) ASS(X,E) ASS(X,F) 
			PUNK(0); PUNK(1);
		#endif

		for(int x=0;x<256;x++,i++)
		{
			u32 &u32screen3D = ((u32*)GPU_screen3D)[i];
			u32screen3D>>=3;
			u32screen3D &= 0x1F1F1F1F;

			const int t = i<<2;
			const u8 a = GPU_screen3D[t+3];
			const u8 r = GPU_screen3D[t+2];
			const u8 g = GPU_screen3D[t+1];
			const u8 b = GPU_screen3D[t+0];
			dst[x] = R5G5B5TORGB15(r,g,b) | alpha_lookup[a];
			dstAlpha[x] = alpha_5bit_to_4bit[a];
		}
	}
}


static void OGLRender()
//Renders the frame buffer
{
	if(!BEGINGL()) return;

	Control();

	if(hasShaders)
	{
		//TODO - maybe this should only happen if the toon table is stale (for a slight speedup)

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_1D, oglToonTableTextureID);
		
		//generate a 8888 toon table from the ds format one and store it in a texture
		u32 rgbToonTable[32];
		for(int i=0;i<32;i++) rgbToonTable[i] = RGB15TO32(gfx3d.u16ToonTable[i], 255);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbToonTable);
	}

	xglDepthMask(GL_TRUE);

	float clearColor[4] = {
		((float)(gfx3d.clearColor&0x1F))/31.0f,
		((float)((gfx3d.clearColor>>5)&0x1F))/31.0f,
		((float)((gfx3d.clearColor>>10)&0x1F))/31.0f,
		((float)((gfx3d.clearColor>>16)&0x1F))/31.0f,
	};
	glClearColor(clearColor[0],clearColor[1],clearColor[2],clearColor[3]);
	glClearDepth(gfx3d.clearDepth);
	glClearStencil((gfx3d.clearColor >> 24) & 0x3F);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	GX_SetCurrentMtx(GL_PROJECTION);
	glLoadIdentity();


	//render display list
	//TODO - properly doublebuffer the display lists
	{

		u32 lastTextureFormat = 0, lastTexturePalette = 0, lastPolyAttr = 0, lastViewport = 0xFFFFFFFF;
		// int lastProjIndex = -1;

		for(int i=0;i<gfx3d.polylist->count;i++) {
			POLY *poly = &gfx3d.polylist->list[gfx3d.indexlist[i]];
			int type = poly->type;

			//a very macro-level state caching approach:
			//these are the only things which control the GPU rendering state.
			if(i==0 || lastTextureFormat != poly->texParam || lastTexturePalette != poly->texPalette || lastPolyAttr != poly->polyAttr)
			{
				isTranslucent = poly->isTranslucent();
				InstallPolygonAttrib(lastPolyAttr=poly->polyAttr);
				lastTextureFormat = textureFormat = poly->texParam;
				lastTexturePalette = texturePalette = poly->texPalette;
				lastPolyAttr = poly->polyAttr;
				BeginRenderPoly();
			}
			
			//since we havent got the whole pipeline working yet, lets use opengl for the projection
		/*	if(lastProjIndex != poly->projIndex) {
				glMatrixMode(GL_PROJECTION);
				glLoadMatrixf(gfx3d.projlist->projMatrix[poly->projIndex]);
				lastProjIndex = poly->projIndex;
			}*/

		/*	glBegin(type==3?GL_TRIANGLES:GL_QUADS);
			for(int j=0;j<type;j++) {
				VERT* vert = &gfx3d.vertlist->list[poly->vertIndexes[j]];
				u8 color[4] = {
					material_5bit_to_8bit[vert->color[0]],
					material_5bit_to_8bit[vert->color[1]],
					material_5bit_to_8bit[vert->color[2]],
					material_5bit_to_8bit[vert->color[3]]
				};
				
				//float tempCoord[4];
				//Vector4Copy(tempCoord, vert->coord);
				//we havent got the whole pipeline working yet, so we cant do this
				////convert from ds device coords to opengl
				//tempCoord[0] *= 2;
				//tempCoord[1] *= 2;
				//tempCoord[0] -= 1;
				//tempCoord[1] -= 1;

				//todo - edge flag?
				glTexCoord2fv(vert->texcoord);
				glColor4ubv((GLubyte*)color);
				//glVertex4fv(tempCoord);
				glVertex4fv(vert->coord);
			}
			glEnd();*/
		
			if(lastViewport != poly->viewport)
			{
				VIEWPORT viewport;
				viewport.decode(poly->viewport);
				glViewport(viewport.x,viewport.y,viewport.width,viewport.height);
				lastViewport = poly->viewport;
			}

			GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 12);//Begin rendering polygons with GX

			VERT *vert0 = &gfx3d.vertlist->list[poly->vertIndexes[0]];
			u8 alpha =	material_5bit_to_8bit[poly->getAlpha()];
			u8 color0[4] = {
					material_5bit_to_8bit[vert0->color[0]],
					material_5bit_to_8bit[vert0->color[1]],
					material_5bit_to_8bit[vert0->color[2]],
					alpha
				};

			for(int j = 1; j < (type-1); j++)
			{
				VERT *vert1 = &gfx3d.vertlist->list[poly->vertIndexes[j]];
				VERT *vert2 = &gfx3d.vertlist->list[poly->vertIndexes[j+1]];
				
				u8 color1[4] = {
					material_5bit_to_8bit[vert1->color[0]],
					material_5bit_to_8bit[vert1->color[1]],
					material_5bit_to_8bit[vert1->color[2]],
					alpha
				};
				u8 color2[4] = {
					material_5bit_to_8bit[vert2->color[0]],
					material_5bit_to_8bit[vert2->color[1]],
					material_5bit_to_8bit[vert2->color[2]],
					alpha
				};
				
				//Parameters for GX_Position3f32() are x, y, z -- We somehow need to get those out of VERT*

				GX_Position3f32(vert0->texcoord);
				glColor4ubv((GLubyte*)color0);
				glVertex4fv(vert0->coord);

				glTexCoord2fv(vert1->texcoord);
				glColor4ubv((GLubyte*)color1);
				glVertex4fv(vert1->coord);

				glTexCoord2fv(vert2->texcoord);
				glColor4ubv((GLubyte*)color2);
				glVertex4fv(vert2->coord);
			}

			GX_End();//This is called after the rendering done by GX_Begin() is done
		}
	}

	ENDGL();

	GL_ReadFramebuffer();
}

static void OGLVramReconfigureSignal()
{
	TexCache_Invalidate();
}

GPU3DInterface gpu3Dgl = {
	"OpenGL",
	OGLInit,
	OGLReset,
	OGLClose,
	OGLRender,
	OGLVramReconfigureSignal,
};

//Shader class functionality
/*
	Constructor
	Sets the shader's type.
*/
Shader::Shader(GLenum itsType)
{
	type=itsType;
	deletionFlag=false;
}

/*
	Flags the shader for deletion.
*/
void Shader::setDeletionFlag()
{
	deletionFlag=true;
}

/*
	Returns true if the shader is flagged for deletion, false otherwise.
*/
bool Shader::getDeletionFlag()
{
	return deletionFlag;
}

/*
	Returns the shader's type.
*/
GLenum Shader::getType()
{
	return type;
}

//Program List Node Class Functionality

/*
	Constructor
*/
ProgramListNode::ProgramListNode(GLuint itsShader)
{
	shader=itsShader;
	nextNode=0;
}

/*
	Returns the node's shader.
*/
GLuint ProgramListNode::getShader()
{
	return shader;
}

/*
	Returns the next node.
*/
GLuint ProgramListNode::getNextNode()
{
	return nextNode;
}

/*
	Sets the next node.
*/
void ProgramListNode::setNextNode(GLuint itsNextNode)
{
	nextNode=itsNextNode;
	return;
}

//Program class functionality
/*
	Constructor
*/
Program::Program()
{
	deletionFlag=false;
	deletableFlag=true;
	firstNode=0;
	numberOfAttachedShaders=0;
}

/*
	Destructor
	Deletes all attached shaders that are flagged for deletion.
*/
Program::~Program()
{
	if (firstNode==0)
	{
		return;
	} else
	{
		ProgramListNode* nodeObject=(ProgramListNode*) firstNode;
		while (true)
		{
			Shader* shader=(Shader*) nodeObject->getShader();
			{
				if (shader->getDeletionFlag()==true)
				{
					delete shader;
				}
			}
			if (nodeObject->getNextNode()==0)
			{
				break;
			}
		}
	}
	return;
}

/*
	Flags the program for deletion.
*/
void Program::setDeletionFlag()
{
	deletionFlag=true;
}

/*
	Checks whether shader is attached. Returns true if it is, otherwise false.
*/
bool Program::shaderIsAttached(GLuint shader)
{
	if (firstNode==0)
	{
		return false;
	} else
	{
		ProgramListNode* nodeObject=(ProgramListNode*) firstNode;
		while(true)
		{
			if (shader==nodeObject->getShader())
			{
				return true;
			}
			if (nodeObject->getNextNode()==0)
			{
				break;
			}
		}
	}
	return false;
}

/*
	Attaches a shader to the program.
*/
void Program::attachShader(GLuint shader)
{
	if (firstNode==0)
	{
		ProgramListNode* node=new ProgramListNode(shader);
		firstNode=(GLuint) node;
	} else
	{
		ProgramListNode* nodeObject=(ProgramListNode*) firstNode;
		while (true)
		{
			ProgramListNode* tempNodeObject=(ProgramListNode*) nodeObject->getNextNode();
			if (tempNodeObject==0)
			{
				break;
			} else
			{
				nodeObject=tempNodeObject;
			}
		}
		ProgramListNode* newNode=new ProgramListNode(shader);
		nodeObject->setNextNode((GLuint) newNode);
	}
	numberOfAttachedShaders++;
	return;
}

/*
	Detaches a shader from the program.
*/
void Program::detachShader(GLuint shader)
{
	ProgramListNode* nodeObject=(ProgramListNode*) firstNode;
	ProgramListNode* previousNodeObject=NULL;
	while(true)
	{
		if (nodeObject->getShader()==shader)
		{
			GLuint nextNode=nodeObject->getNextNode();
			if (previousNodeObject==NULL)
			{
				firstNode=nextNode;
			} else
			{
				previousNodeObject->setNextNode(nextNode);
			}
			break;
		} else
		{
			previousNodeObject=nodeObject;
			nodeObject=(ProgramListNode*) nodeObject->getNextNode();
		}
	}
	numberOfAttachedShaders--;
	return;
}

/*
	Returns true if the program can be deleted.
*/
bool Program::isDeletable()
{
	return deletableFlag;
}

/*
	Returns true if the program is flagged for deletion, false otherwise.
*/
bool Program::getDeletionFlag()
{
	return deletionFlag;
}

/*
	Returns the number of attached shaders.
*/
int Program::getNumberOfAttachedShaders()
{
	return numberOfAttachedShaders;
}
