#ifndef B47620F3_39DD_4C45_894A_066DE080C7C5
#define B47620F3_39DD_4C45_894A_066DE080C7C5

#define LOG_PREFIX "OPENGL-IMAGE-PROCESS. "

// #ifdef __ANDROID__
#ifndef NDEBUG
    #include <android/log.h>
    #define LOGI(...) \
        ((void)__android_log_print(ANDROID_LOG_INFO, "native-lib::", LOG_PREFIX __VA_ARGS__))
    #define LOGE(...) \
        ((void)__android_log_print(ANDROID_LOG_ERROR, "native-lib::", LOG_PREFIX __VA_ARGS__))
#else
    #include <cstdio>
    #define LOGI(...) do {(printf(LOG_PREFIX __VA_ARGS__)); puts("");} while(0)
    #define LOGE(...) do {(fprintf(stderr, LOG_PREFIX __VA_ARGS__)); puts("");} while(0)
#endif

#endif /* B47620F3_39DD_4C45_894A_066DE080C7C5 */