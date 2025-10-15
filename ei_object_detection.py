# Edge Impulse - OpenMV FOMO Object Detection Example
#
# This work is licensed under the MIT license.
# Copyright (c) 2013-2024 OpenMV LLC. All rights reserved.
# https://github.com/openmv/openmv/blob/master/LICENSE

import sensor, image, time, ml, math, uos, gc
from pyb import Pin
import utime
from pyb import UART
sensor.reset()                         # Reset and initialize the sensor.
sensor.set_pixformat(sensor.RGB565)    # Set pixel format to RGB565 (or GRAYSCALE)
sensor.set_framesize(sensor.QVGA)      # Set frame size to QVGA (320x240)
sensor.set_windowing((240, 240))       # Set 240x240 window.
sensor.skip_frames(time=2000)          # Let the camera adjust.

net = None
labels = None
min_confidence = 0.5

try:
    # load the model, alloc the model file on the heap if we have at least 64K free after loading
    net = ml.Model("trained.tflite", load_to_fb=uos.stat('trained.tflite')[6] > (gc.mem_free() - (64*1024)))
except Exception as e:
    raise Exception('Failed to load "trained.tflite", did you copy the .tflite and labels.txt file onto the mass-storage device? (' + str(e) + ')')

try:
    labels = [line.rstrip('\n') for line in open("labels.txt")]
except Exception as e:
    raise Exception('Failed to load "labels.txt", did you copy the .tflite and labels.txt file onto the mass-storage device? (' + str(e) + ')')

colors = [ # Add more colors if you are detecting more than 7 types of classes at once.
    (255,   0,   0),
    (  0, 255,   0),
    (255, 255,   0),
    (  0,   0, 255),
    (255,   0, 255),
    (  0, 255, 255),
    (255, 255, 255),
]

threshold_list = [(math.ceil(min_confidence * 255), 255)]

def fomo_post_process(model, inputs, outputs):
    ob, oh, ow, oc = model.output_shape[0]

    x_scale = inputs[0].roi[2] / ow
    y_scale = inputs[0].roi[3] / oh

    scale = min(x_scale, y_scale)

    x_offset = ((inputs[0].roi[2] - (ow * scale)) / 2) + inputs[0].roi[0]
    y_offset = ((inputs[0].roi[3] - (ow * scale)) / 2) + inputs[0].roi[1]

    l = [[] for i in range(oc)]

    for i in range(oc):
        img = image.Image(outputs[0][0, :, :, i] * 255)
        blobs = img.find_blobs(
            threshold_list, x_stride=1, y_stride=1, area_threshold=1, pixels_threshold=1
        )
        for b in blobs:
            rect = b.rect()
            x, y, w, h = rect
            score = (
                img.get_statistics(thresholds=threshold_list, roi=rect).l_mean() / 255.0
            )
            x = int((x * scale) + x_offset)
            y = int((y * scale) + y_offset)
            w = int(w * scale)
            h = int(h * scale)
            l[i].append((x, y, w, h, score))
    return l

clock = time.clock()
while(True):
    clock.tick()

    img = sensor.snapshot()

    for i, detection_list in enumerate(net.predict([img], callback=fomo_post_process)):
        if i == 0: continue  # background class
        if len(detection_list) == 0: continue  # no detections for this class?

        print("********** %s **********" % labels[i])
        for x, y, w, h, score in detection_list:
            center_x = math.floor(x + (w / 2))
            center_y = math.floor(y + (h / 2))
            print(f"x {center_x}\ty {center_y}\tscore {score}")
            img.draw_circle((center_x, center_y, 12), color=colors[i])

    print(clock.fps(), "fps", end="\n\n")
    # --- one-time init ---
    uart = UART(1, 115200, timeout_char=50)  # if no data, try UART(1) or UART(2)
    uart.write("hello\n")
    TRIG_NAME = 'D3'  # <-- choose one that appears in dir(Pin.board)
    TRIG_PIN  = getattr(Pin.board, TRIG_NAME)
    trig = Pin('D3', Pin.OUT_PP)
    trig.low()

    # re-arm logic (only trigger once until no detections for some frames)
    REARM_FRAMES = 5
    _frames_since_hit = REARM_FRAMES

    def trigger_arduino(ms=30):
        trig.high()
        utime.sleep_ms(ms)
        trig.low()

    # --- inside your detection loop where you confirm a ROCK with score >= 0.7 ---
    rock_seen = False
    for x, y, w, h, score in detection_list:
        if score >= 0.70:
            rock_seen = True
            # draw / print as you already do...
            # (cx, cy) computed already

    if rock_seen:
        if _frames_since_hit >= REARM_FRAMES:
            trigger_arduino(30)   # 30 ms pulse
            _frames_since_hit = 0
    else:
        _frames_since_hit = min(REARM_FRAMES, _frames_since_hit + 1)


