from flask import Flask, Response
from picamera2 import Picamera2, Preview
import cv2
import time

app = Flask(__name__)

# Picamera2 설정
picam2 = Picamera2()
config = picam2.create_preview_configuration(main={"size": (640, 480), "format": "YUV420"})
picam2.configure(config)
picam2.start()

def generate_mjpeg():
    """MJPEG 스트림 프레임 제너레이터"""
    while True:
        img = picam2.capture_array()
        # YUV → RGB 변환
        # img = cv2.cvtColor(img, cv2.COLOR_YUV2RGB_I420)
        img = cv2.cvtColor(img, cv2.COLOR_YUV2RGB_YV12)
        # ret, jpeg = cv2.imencode('.jpg', img)
        ret, jpeg = cv2.imencode('.jpg', img, [int(cv2.IMWRITE_JPEG_QUALITY), 70])
        if not ret:
            continue
        frame = jpeg.tobytes()
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')
        time.sleep(0.001)

@app.route('/stream')
def stream():
    return Response(generate_mjpeg(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/')
def index():
    return "<html><body><h1>Live Stream</h1><img src='/stream'></body></html>"

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080, threaded=True)
