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
#include <cstring>

const char * vert_src = R"(
    #version 320 es
    precision lowp float;
    layout (location = 0) in vec3 in_position;
    layout (location = 1) in vec2 in_texcoord;
    out vec2 texcoord;
    void main()
    {
        texcoord = in_texcoord;
        gl_Position = vec4(in_position, 1.0);
    }
)";

const char *frag_src = R"(
    #version 320 es
    precision lowp float;
    in vec2 texcoord;
    out vec3 fragcolor;
    uniform sampler2D tex;
    void main()
    {
        fragcolor = 1.0 - texture(tex, texcoord).rgb;
    }
)";

int main(int argc, char *argv[])
{
    Timer timer;
    Timer timer_total;
    int error;

    GLsync sync;

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
    printf("glGetAttribLocation: %d\n", glGetAttribLocation(program, "in_position"));
    glVertexAttribPointer(glGetAttribLocation(program, "in_position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // texture_coords attribute
    glVertexAttribPointer(glGetAttribLocation(program, "in_texcoord"), 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //========== read image data and allocate output data
    std::vector<uint8_t> img;
    unsigned int width, height;
    error = lodepng::decode(img, width, height, argv[1], LCT_RGB);
    if(error) {
        std::cerr << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
        std::exit(1);
    }
    printf("image width: %d, height: %d\n", width, height);

    // allocate output memory
    std::vector<uint8_t> img_out(width * height * 3);

    //========== create AHardwareBuffer, EGLImage, GL_TEXTURE_2D for input
    AHardwareBuffer_Desc input_ahb_desc{};
    input_ahb_desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM;
    input_ahb_desc.width = width;
    input_ahb_desc.height = height;
    input_ahb_desc.layers = 1;
    input_ahb_desc.usage = AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN |
                           AHARDWAREBUFFER_USAGE_CPU_READ_NEVER |
                           AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE;

    AHardwareBuffer *input_ahb;
    error = AHardwareBuffer_allocate(&input_ahb_desc, &input_ahb);
    if(error != 0)
    {
        LOGE("AHardwareBuffer_allocate failed %d, %d\n", error, __LINE__);
        std::exit(1);
    }

    // Create EGLClientBuffer from the AHardwareBuffer.
    EGLClientBuffer native_buffer_input = eglGetNativeClientBufferANDROID(input_ahb);

    // Create EGLImage from EGLClientBuffer
    EGLint egl_img_input_attrs[] = {EGL_NONE};

    EGLImageKHR egl_img_input = eglCreateImageKHR(eglGetCurrentDisplay(), EGL_NO_CONTEXT,
        EGL_NATIVE_BUFFER_ANDROID, native_buffer_input, egl_img_input_attrs);
    if(egl_img_input == EGL_NO_IMAGE) {
        LOGE("eglCreateImageKHR failed.");
    } else {
        LOGI("eglCreateImageKHR success.");
    }

    // Create texture from EGLImage
    GLuint texture_in;
    glGenTextures(1, &texture_in);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_in);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, egl_img_input);

    //========== create AHardwareBuffer, EGLImage, GL_TEXTURE_2D for output
    AHardwareBuffer_Desc output_ahb_desc{};
    output_ahb_desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM;
    output_ahb_desc.width = width;
    output_ahb_desc.height = height;
    output_ahb_desc.layers = 1;
    output_ahb_desc.usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN |
                            AHARDWAREBUFFER_USAGE_CPU_WRITE_NEVER |
                            AHARDWAREBUFFER_USAGE_GPU_FRAMEBUFFER;
    AHardwareBuffer *output_ahb;
    error = AHardwareBuffer_allocate(&output_ahb_desc, &output_ahb);
    if(error == 1) {
        LOGE("AHardwareBuffer_allocate failed");
        std::exit(1);
    } else {
        LOGI("AHardwareBuffer_allocate success");
    }

    // Create EGLClientBuffer from the AHardwareBuffer.
    EGLClientBuffer native_buffer_output = eglGetNativeClientBufferANDROID(output_ahb);

    // Create EGLImage from EGLClientBuffer
    EGLint egl_img_attrs[] = {EGL_NONE};

    EGLImageKHR egl_img_output = eglCreateImageKHR(eglGetCurrentDisplay(), EGL_NO_CONTEXT,
        EGL_NATIVE_BUFFER_ANDROID, native_buffer_output, egl_img_attrs);
    if(egl_img_output == EGL_NO_IMAGE) {
        LOGE("eglCreateImageKHR failed.");
    } else {
        LOGI("eglCreateImageKHR success.");
    }

    // Create texture from EGLImage
    GLuint texture_out;
    glGenTextures(1, &texture_out);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture_out);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, egl_img_output);

    //========== create output framebuffer
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_out, 0);
    glViewport(0, 0, width, height);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
        LOGI("glCheckFramebufferStatus success.");
    } else {
        LOGE("glCheckFramebufferStatus error. %d", glCheckFramebufferStatus(GL_FRAMEBUFFER));
        // std::exit(1);
    }
    LOGI("glGetError %d.", glGetError());

    // finish initialize work
    glFinish();

    timer_total.reset();
    //========== map input data to texture
    timer.reset();

    void *buffer_writer = nullptr;
    error = AHardwareBuffer_lock(input_ahb, AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN, -1, nullptr, (void **) &buffer_writer);
    if(error != 0) {
        LOGE("AHardwareBuffer_lock failed.");
    }

    std::memcpy(buffer_writer, img.data(), width * height * 3);

    error = AHardwareBuffer_unlock(input_ahb, nullptr);
    if(error != 0) {
        LOGE("AHardwareBuffer_unlock failed.");
    }

    printf("map input time line %d, %fms\n", __LINE__, timer.elapsedUs() / 1000);

    //========== render
    timer.reset();

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // finish GPU side
    sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 1e9);

    printf("render time line %d, %fms\n", __LINE__, timer.elapsedUs() / 1000);

    //========== read output data
    timer.reset();

    // map output memory
    void *buffer_reader;
    error = AHardwareBuffer_lock(output_ahb, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, nullptr, (void **)&buffer_reader);
    if(error == 0) {
        LOGI("AHardwareBuffer_lock success.");
    } else {
        LOGE("AHardwareBuffer_lock failed.");
    }
    memcpy(img_out.data(), buffer_reader, width * height * 3);
    AHardwareBuffer_unlock(output_ahb, nullptr);

    printf("map output time line %d, %fms\n", __LINE__, timer.elapsedUs() / 1000);

    //==========
    printf("total time: %fms\n", timer_total.elapsedUs() / 1000);

    error = lodepng::encode("results/img_out.png", img_out, width, height, LCT_RGB);
    if(error) std::cout << "encoder error " << error << ": "<< lodepng_error_text(error) << std::endl;

    glDeleteSync(sync);
    eglDestroyImageKHR(eglGetCurrentDisplay(), egl_img_output);
    AHardwareBuffer_release(output_ahb);

    return 0;
}