import picamera
import time

camera = picamera.PiCamera()
camera.resolution = (2592, 1944) # (64, 64) ~ (2592, 1944) px

time.sleep(3)
camera.capture('snapshot.jpg')