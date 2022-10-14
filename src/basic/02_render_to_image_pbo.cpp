#include "../gl_header/GLES3/gl3.h"
#include "../gl_header/GLES2/gl2ext.h"


#include "../utils/egl_wrapper.h"
#include "../utils/lodepng.h"
#include "../utils/log.h"
#include "../utils/timer.h"

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
    GLuint pbo_in;
    glGenBuffers(1, &pbo_in);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_in);
    timer.reset();
    glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * 3, img.data(), GL_STREAM_DRAW);
    printf("pbo_in input time: %fms\n", timer.elapsedUs() / 1000);

    /*
    // use map buff to modified the data
    void *mapped_buffer = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, width * height * 3, GL_MAP_WRITE_BIT);
    for(int i = 0; i < 100; i++)
        for(int j = 0; j < 100; j++)
            for(int k = 0; k < 3; k++) {
                int idx = (i * width + j) * 3 + k;
                ((uint8_t *)mapped_buffer)[idx] = 255;
        }
    */

    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

    timer_total.reset();
    GLuint texture_in;
    glGenTextures(1, &texture_in);
    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, texture_in);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    timer.reset();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    printf("glTexImage2D input time: %fms\n", timer.elapsedUs() / 1000);

    GLuint tex_sampler;
    tex_sampler = glGetUniformLocation(program, "tex");
    glUniform1i(tex_sampler, 8);

    //========== output texture
    GLuint texture_out;
    glGenTextures(1, &texture_out);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture_out);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    timer.reset();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    printf("glTexImage2D output time: %fms\n", timer.elapsedUs() / 1000);


    //========== output framebuffer
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_out, 0);
    glViewport(0, 0, width, height);

    //========== render
    timer.reset();
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    printf("render time: %fms\n", timer.elapsedUs() / 1000);

    //========== read output to PBO
    // timer.reset();
    GLuint pbo_out;
    glGenBuffers(1, &pbo_out);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo_out);
    glBufferData(GL_PIXEL_PACK_BUFFER, width * height * 3, NULL, GL_DYNAMIC_READ);
    timer.reset();
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, 0);
    printf("glReadPixels time: %fms\n", timer.elapsedUs() / 1000);

    //========== read output image
    timer.reset();
    void *pbo_out_data = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, width * height * 3, GL_MAP_READ_BIT);
    memcpy(img_out.data(), pbo_out_data, width * height * 3);
    printf("memcpy time: %fms\n", timer.elapsedUs() / 1000);

    printf("total time: %fms\n", timer_total.elapsedUs() / 1000);

    //========== write output image
    error = lodepng::encode("results/img_out.png", img_out, width, height, LCT_RGB);
    if(error) std::cout << "encoder error " << error << ": "<< lodepng_error_text(error) << std::endl;

    return 0;
}