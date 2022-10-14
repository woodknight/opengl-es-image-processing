#!/bin/bash
set -e

DEVICE_PATH=/data/local/tmp/opengl-es-test/

# EXE=01_render_to_image
# EXE=02_render_to_image_pbo
# EXE=03_memcpy_benchmark
EXE=04_ahardwarebuffer

adb shell mkdir -p ${DEVICE_PATH}
adb push build/bin/${EXE} ${DEVICE_PATH}
adb shell chmod +x ${DEVICE_PATH}/${EXE}
adb shell "
cd ${DEVICE_PATH}
./${EXE} images/toucan_1280x960.png
exit
"
adb pull ${DEVICE_PATH}/results/img_out.png tmp/
eog tmp/img_out.png