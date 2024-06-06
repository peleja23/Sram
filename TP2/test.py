import socket
import struct
import time
from datetime import datetime
import cv2

PORT = 5555
DIGIT_IMAGE_SIZE = 8192  # Adjust size if needed
MAX_BUFFER = 1024  # Define buffer size for timestamp

# List of digit image file paths
digitFiles = [
    "digitos/zero.png", "digitos/um.png", "digitos/dois.png",
    "digitos/tres.png", "digitos/quatro.png", "digitos/cinco.png",
    "digitos/seis.png", "digitos/sete.png", "digitos/oito.png",
    "digitos/nove.png"
]

def load_image(filename):
    img = cv2.imread(filename, cv2.IMREAD_COLOR)
    _, buffer = cv2.imencode('.png', img)
    return buffer.tobytes()

def generate_timestamp(F):
    now = datetime.now()
    ms = now.microsecond // 1000
    if F == 0:
        return now.strftime("%H%M%S")
    elif F == 1:
        return now.strftime("%H%M%S") + f"{ms // 100}"
    elif F == 2:
        return now.strftime("%H%M%S") + f"{ms // 10}"
    elif F == 3:
        return now.strftime("%H%M%S") + f"{ms}"
    elif F == 4:
        return now.strftime("%H%M%S") + f"{ms}{now.microsecond // 100000}"
    elif F == 5:
        return now.strftime("%H%M%S") + f"{ms}{now.microsecond // 10000}"
    elif F == 6:
        return now.strftime("%H%M%S") + f"{ms}{now.microsecond // 1000}"
    elif F == 7:
        return now.strftime("%H%M%S") + f"{ms}{now.microsecond}"
    return now.strftime("%H%M%S")

def prepare_pdu(F, A):
    timestamp = generate_timestamp(F)
    print(f"Generated timestamp: {timestamp}")

    digitImages = []
    for char in timestamp:
        if char.isdigit():
            digitImages.append(load_image(digitFiles[int(char)]))
        else:
            digitImages.append(b'\x00' * DIGIT_IMAGE_SIZE) 

    return A, F, timestamp.encode('utf-8'), digitImages

def send_data(F, A):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_address = ('127.0.0.1', PORT)

    while True:
        A, F, timestamp, digitImages = prepare_pdu(F, A)
        pdu = struct.pack('ii', A, F) + timestamp.ljust(MAX_BUFFER, b'\x00')
        for img in digitImages:
            pdu += img.ljust(DIGIT_IMAGE_SIZE, b'\x00')
        
        sock.sendto(pdu, server_address)
        
        time.sleep(1)  # Send every second

if __name__ == "__main__":
    F = 1
    A = 2
    send_data(F, A)
