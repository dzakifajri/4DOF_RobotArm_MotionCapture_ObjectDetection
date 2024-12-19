import cv2
import numpy as np

def get_dominant_color(image):
    # Convert image to RGB (OpenCV uses BGR by default)
    rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    
    # Reshape the image to be a list of pixels
    pixels = rgb.reshape(-1, 3)
    
    # Calculate the mean color
    mean_color = np.mean(pixels, axis=0)
    
    # Convert mean color to HSV
    hsv_color = cv2.cvtColor(np.uint8([[mean_color]]), cv2.COLOR_RGB2HSV)[0][0]
    
    # Get Hue value
    hue = hsv_color[0]
    
    # Define color ranges more precisely
    if (hue >= 0 and hue < 10) or (hue >= 170 and hue <= 180):
        return "Merah"
    elif hue >= 10 and hue < 20:
        return "Oranye"
    elif hue >= 20 and hue < 40:
        return "Kuning"
    elif hue >= 40 and hue < 80:
        return "Hijau"
    elif hue >= 80 and hue < 140:
        return "Biru"
    elif hue >= 140 and hue < 170:
        return "Ungu"
    else:
        return "Tidak dikenal"

def apply_white_balance(image):
    # Simple white balancing using gray world assumption
    result = cv2.cvtColor(image, cv2.COLOR_BGR2LAB)
    avg_a = np.average(result[:, :, 1])
    avg_b = np.average(result[:, :, 2])
    result[:, :, 1] = result[:, :, 1] - ((avg_a - 128) * (result[:, :, 0] / 255.0) * 1.1)
    result[:, :, 2] = result[:, :, 2] - ((avg_b - 128) * (result[:, :, 0] / 255.0) * 1.1)
    return cv2.cvtColor(result, cv2.COLOR_LAB2BGR)

# Initialize camera
cap = cv2.VideoCapture(1)

while True:
    # Read frame from camera
    ret, frame = cap.read()
    if not ret:
        break
    
    # Apply white balance
    frame = apply_white_balance(frame)
    
    # Define area of interest (center of frame)
    height, width = frame.shape[:2]
    roi = frame[height//3:2*height//3, width//3:2*width//3]
    
    # Get dominant color
    color = get_dominant_color(roi)
    
    # Display results
    cv2.putText(frame, f"Warna: {color}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
    cv2.rectangle(frame, (width//3, height//3), (2*width//3, 2*height//3), (0, 255, 0), 2)
    
    # Show frame
    cv2.imshow('Deteksi Warna', frame)
    
    # Exit if 'q' is pressed
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# Cleanup
cap.release()
cv2.destroyAllWindows()