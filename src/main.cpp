#define GL_GLEXT_PROTOTYPES

// #include <GLES3/gl31.h>
// #include <GLES3/gl3platform.h>
// #include <GLES3/gl3ext.h>

#include "common/GLES2/gl2.h"
#include "common/GLES2/gl2ext.h"
#include "common/GLES3/gl3.h"

#include "utils/timer.h"

#include "egl_wrapper.h"

#include <iostream>

int main()
{
    Timer timer;
    EglWrapper egl_wrapper;

    int width = 1280;
    int height = 960;

    float vertices[] = {
        // positions        // texture coords
        -1.0f, -1.0f, 0.0f,      0.0f,    0.0f,
        -1.0f,  1.0f, 0.0f,      0.0f,    1.0f,
         1.0f, -1.0f, 0.0f,      1.0f,    0.0f,
         1.0f,  1.0f, 0.0f,      1.0f,    1.0f
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // uint8_t image_ref[height][width];
    uint8_t *image_ref = new uint8_t[height * width];
    for(int i = 0; i < width * height; i++)
        image_ref[i] = 13;
    // for(int i = 0; i < height; i++) {
    //     for(int j = 0; j < width; j++) {
    //         uint8_t c = (((i & 0x8)==0) ^ ((j & 0x8)==0)) * 255;
    //         image_ref[i][j] = c;
    //     }
    // }

    // uint8_t image_tar[height][width];
    uint8_t *image_tar = new uint8_t[height * width];
    // for(int i = 1; i < height; i++)
    //     for(int j = 1; j < width; j++)
    //         image_tar[i][j] = image_ref[i - 1][j - 1];



    timer.reset();
    GLuint texture_ref;
    glGenTextures(1, &texture_ref);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture_ref);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_R8, GL_UNSIGNED_BYTE, image_ref);
    printf("texture_ref time: %f ms\n", timer.elapsedUs() / 1000);

    GLuint texture_tar;
    glGenTextures(1, &texture_tar);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texture_tar);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_R8, GL_UNSIGNED_BYTE, image_tar);

    // float image_out[height][width][4];
    float *image_out = new float[height * width * 4];
    GLuint texture_out;
    glGenTextures(1, &texture_out);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, texture_out);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA16F, GL_FLOAT_VEC4, image_out);

    // glCopyTexImage2D(texture_ref, 0, GL_R8, 0, 0, width, height, 0);


    timer.reset();
    glTexEstimateMotionQCOM(texture_ref, texture_tar, texture_out);
    glFinish();
    printf("glTexEstimateMotionQCOM time: %f ms\n", timer.elapsedUs() / 1000);

        // draw
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    timer.reset();
    uint8_t pixel;
    glReadPixels(0, 0, width, height, GL_R8, GL_UNSIGNED_BYTE, image_tar);
    printf("glReadPixels time: %f ms\n", timer.elapsedUs() / 1000);
    printf("pixel: %d\n", image_tar[0]);

    glFinish();
    // for(int i = 0; i < height; i++)
    //     for(int j = 0; j < width; j++)
    //         std::cout << image_out[4*(i * width + j)];
    // std::cout << std::endl;

    delete []image_out;
    delete []image_tar;
    delete []image_ref;
    return 0;
}