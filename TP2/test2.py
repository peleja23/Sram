import socket
import struct
import numpy as np
import cv2

PORT = 5555
MAX_BUFFER = 1024  # Define the same buffer size used in the server
DIGIT_IMAGE_SIZE = 8192  # Adjust size if needed

class PDU:
    def __init__(self, A, F, timestamp, digitImages):
        self.A = A
        self.F = F
        self.timestamp = timestamp
        self.digitImages = digitImages

def receive_data():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('0.0.0.0', PORT))

    while True:
        data, addr = sock.recvfrom(65536)
        if len(data) < 8 + MAX_BUFFER + 6 * DIGIT_IMAGE_SIZE:
            print(f"Received incomplete PDU, length: {len(data)}")
            continue
        A = struct.unpack('i', data[0:4])[0]
        F = struct.unpack('i', data[4:8])[0]
        timestamp = data[8:8+MAX_BUFFER].decode('utf-8').strip('\x00')
        digitImages = [data[8+MAX_BUFFER+i*DIGIT_IMAGE_SIZE:8+MAX_BUFFER+(i+1)*DIGIT_IMAGE_SIZE] for i in range(6)]

        print(f"A: {A}, F: {F}, Time: {timestamp}")
        for i in range(7):
            nparr = np.frombuffer(digitImages[i], np.uint8)
            img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
            if img is not None:
                cv2.imshow(f"Digit {i}", img)
                cv2.waitKey(1)  # Display the image

if __name__ == "__main__":
    receive_data()
