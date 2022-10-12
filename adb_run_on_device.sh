#!/bin/bash
set -e

DEVICE_PATH=/data/local/tmp/opengl-es-test
EXE=02_render_to_image_pbo

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