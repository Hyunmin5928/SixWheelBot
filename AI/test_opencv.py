import cv2
import numpy as np
import time
import os

# 로컬 이미지 경로
input_path = "./MP_SEL_SUR_010347.jpg"       # ← 사용할 이미지 경로로 변경하세요
output_path = "./path_result.jpg"     # 결과 저장 경로

# 이미지 불러오기
image = cv2.imread(input_path)
if image is None:
    raise FileNotFoundError(f"이미지를 찾을 수 없습니다: {input_path}")

# 해상도 줄이기 (optional)
image = cv2.resize(image, (640, 480))

start = time.time()

# HSV 변환
hsv_image = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)

# 하단 중앙 픽셀의 HSV 값 추출
h, w = hsv_image.shape[:2]
center_x, center_y = w // 2, int(h * 0.9)
base_color = hsv_image[center_y, center_x]
h_val, s_val, v_val = [int(c) for c in base_color]

# 색상 범위 지정
lower = np.array([max(h_val - 10, 0), max(s_val - 40, 0), max(v_val - 40, 0)], dtype=np.uint8)
upper = np.array([min(h_val + 10, 179), min(s_val + 40, 255), min(v_val + 40, 255)], dtype=np.uint8)

# 마스크 및 윤곽선 추출
mask = cv2.inRange(hsv_image, lower, upper)
edges = cv2.Canny(mask, 50, 150)
contours, _ = cv2.findContours(edges, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

# 결과 이미지에 윤곽선 그리기
result = image.copy()
cv2.drawContours(result, contours, -1, (0, 255, 0), 2)

end = time.time()
print(f"✅ 처리 소요 시간: {end - start:.4f}초")

# 결과 이미지 저장
cv2.imwrite(output_path, result)
print(f"📁 결과 이미지 저장 완료: {output_path}")
