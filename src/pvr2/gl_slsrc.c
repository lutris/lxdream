/**
 * This file is automatically generated - do not edit
 */

const char *glsl_vertex_shader_src = "// Standard PVR2 vertex shader\n\
\n\
void main()\n\
{\n\
    vec4 tmp = ftransform();\n\
    float w = gl_Vertex.z;\n\
    gl_Position  = tmp * w;\n\
    gl_FrontColor = gl_Color;\n\
    gl_FrontSecondaryColor = gl_SecondaryColor;\n\
    gl_TexCoord[0] = gl_MultiTexCoord0;\n\
}\n\
";

const char *glsl_fragment_shader_src = "// Standard PVR2 fragment shader\n\
\n\
void main()\n\
{\n\
	gl_FragColor = gl_Color;\n\
	gl_FragDepth = gl_FragCoord.z;\n\
}";
