#define GL_GLEXT_PROTOTYPES
#include "../gl_header/GLES3/gl3.h"
#include "../gl_header/GLES2/gl2ext.h"

#define EGL_EGLEXT_PROTOTYPES
#include "../gl_header/EGL/egl.h"
#include "../gl_header/EGL/eglext.h"
// #include "../gl_header/EGL/eglplatform.h"

#include "../utils/egl_wrapper.h"
#include "../utils/lodepng.h"
#include "../utils/log.h"
#include "../utils/timer.h"

#include <android/hardware_buffer.h>
#include <vector>
#include <iostream>

const char *vert_src = "\
#version 300 es\n\
layout (location = 0) in vec3 in_position;\n\
layout (location = 1) in vec2 in_texcoord;\n\
out vec2 texcoord;\n\
void main()\n\
{\n\
    texcoord = in_texcoord;\n\
    gl_Position = vec4(in_position, 1.0);\n\
}\n\
";

const char *frag_src = "\
#version 300 es\n\
in vec2 texcoord;\n\
out vec3 fragcolor;\n\
uniform sampler2D tex;\n\
void main()\n\
{\n\
    fragcolor = 1.0 - texture(tex, texcoord).rgb;\n\
}\n\
";

int main(int argc, char *argv[])
{
    Timer timer;
    Timer timer_total;

    //========== EGL init
    EglWrapper egl_wrapper;

    //========== shader program
    GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert_shader, 1, &vert_src, NULL);
    glCompileShader(vert_shader);

    int success;
    char infoLog[512];
    glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(vert_shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader, 1, &frag_src, NULL);
    glCompileShader(frag_shader);

    glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(frag_shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vert_shader);
    glAttachShader(program, frag_shader);
    glLinkProgram(program);
    glUseProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    //========== VAO VBO
    float vertices [] = {
        // positions            // texture_coords
        -1.0f, -1.0f,  0.0f,    0.0f, 0.0f,
        -1.0f,  1.0f,  0.0f,    0.0f, 1.0f,
         1.0f, -1.0f,  0.0f,    1.0f, 0.0f,
         1.0f,  1.0f,  0.0f,    1.0f, 1.0f
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // positions attribute
    glVertexAttribPointer(glGetAttribLocation(program, "in_position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // texture_coords attribute
    glVertexAttribPointer(glGetAttribLocation(program, "in_texcoord"), 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //========== read image data
    std::vector<uint8_t> img;
    unsigned int width, height;
    auto error = lodepng::decode(img, width, height, argv[1], LCT_RGB);
    if(error) {
        std::cerr << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
        std::exit(1);
    }
    printf("image width: %d, height: %d\n", width, height);

    std::vector<uint8_t> img_out(width * height * 3);

    //========== upload image to input texture
    timer_total.reset();
    GLuint texture_in;
    glGenTextures(1, &texture_in);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_in);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    timer.reset();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, img.data());
    printf("glTexImage2D input time: %fms\n", timer.elapsedUs() / 1000);

    //========== create AHardwareBuffer and EGLImage
    AHardwareBuffer_Desc desc{};
    desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM;
    desc.width = width;
    desc.height = height;
    desc.layers = 1;
    desc.usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN |
                 AHARDWAREBUFFER_USAGE_CPU_WRITE_NEVER |
                 AHARDWAREBUFFER_USAGE_GPU_FRAMEBUFFER;
    AHardwareBuffer *hwbuffer;
    timer.reset();
    error = AHardwareBuffer_allocate(&desc, &hwbuffer);
    printf("AHardwareBuffer_allocate time: %f ms\n", timer.elapsedUs() / 1000);
    if(error == 1) {
        LOGE("AHardwareBuffer_allocate failed");
        std::exit(1);
    } else {
        LOGI("AHardwareBuffer_allocate success");
    }

    // Create EGLClientBuffer from the AHardwareBuffer.
    EGLClientBuffer native_buffer = eglGetNativeClientBufferANDROID(hwbuffer);

    // Create EGLImage from EGLClientBuffer
    EGLint egl_attrs[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE};
    EGLImageKHR egl_img = eglCreateImageKHR(eglGetCurrentDisplay(), EGL_NO_CONTEXT,
        EGL_NATIVE_BUFFER_ANDROID, native_buffer, egl_attrs);

    //========== output texture
    GLuint texture_out;
    glGenTextures(1, &texture_out);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture_out);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    // glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, egl_img);

    //========== output framebuffer
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_out, 0);
    // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_EXTERNAL_OES, texture_out, 0);
    glViewport(0, 0, width, height);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
        LOGI("glCheckFramebufferStatus success.");
    } else {
        LOGE("glCheckFramebufferStatus error. %d", glCheckFramebufferStatus(GL_FRAMEBUFFER));
        // std::exit(1);
    }
    LOGI("glGetError %d.", glGetError());

    //========== render
    timer.reset();
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    printf("render time: %fms\n", timer.elapsedUs() / 1000);

    //========== read output image
    // timer.reset();
    // void *buffer_reader;
    // error = AHardwareBuffer_lock(hwbuffer, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, nullptr, (void **)&buffer_reader);
    // if(error == 0) {
    //     LOGI("AHardwareBuffer_lock success.");
    // } else {
    //     LOGE("AHardwareBuffer_lock failed.");
    // }
    // printf("AHardwareBuffer_lock time: %fms\n", timer.elapsedUs() / 1000);

    // timer.reset();
    // memcpy(img_out.data(), buffer_reader, width * height * 3);
    // printf("memcpy time: %fms\n", timer.elapsedUs() / 1000);

    // AHardwareBuffer_unlock(hwbuffer, nullptr);

    timer.reset();
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *)img_out.data());
    printf("glReadPixels time: %fms\n", timer.elapsedUs() / 1000);

    printf("total time: %fms\n", timer_total.elapsedUs() / 1000);

    error = lodepng::encode("results/img_out.png", img_out, width, height, LCT_RGB);
    if(error) std::cout << "encoder error " << error << ": "<< lodepng_error_text(error) << std::endl;

    // eglDestroyImageKHR;

    return 0;
}