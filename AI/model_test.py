import torch
import torch.nn.functional as F
import numpy as np
import matplotlib.pyplot as plt
from PIL import Image
import torchvision.transforms as T
import segmentation_models_pytorch as smp
from io import BytesIO
from google.colab import files

#  COLOR_MAP 정의
COLOR_MAP = {
    (230, 170, 255): 0, (0, 0, 255): 1, (0, 255, 0): 2, (255, 128, 255): 3,
    (255, 128, 0): 4, (255, 192, 0): 5, (255, 230, 153): 6, (255, 0, 0): 7,
    (138, 60, 200): 8, (55, 86, 35): 9, (217, 217, 217): 10, (110, 168, 70): 11,
    (198, 89, 17): 12, (128, 128, 128): 13, (255, 155, 155): 14, (88, 38, 128): 15,
    (255, 0, 255): 16, (208, 88, 255): 17, (0, 0, 0): 18
}

#  인덱스 → RGB 마스크
def index_to_color_mask(index_mask, color_map):
    color_mask = np.zeros((index_mask.shape[0], index_mask.shape[1], 3), dtype=np.uint8)
    for rgb, idx in color_map.items():
        color_mask[index_mask == idx] = rgb
    return color_mask

#  이미지 크기를 8의 배수로 패딩
def pad_to_multiple_of_8(img_tensor):
    _, _, h, w = img_tensor.shape
    new_h = ((h + 7) // 8) * 8
    new_w = ((w + 7) // 8) * 8
    pad_h = new_h - h
    pad_w = new_w - w
    padding = (0, pad_w, 0, pad_h)  # (left, right, top, bottom)
    padded = F.pad(img_tensor, padding, mode="reflect")
    return padded, (h, w)  # 원본 크기도 반환

# 장치 설정 및 모델 로드
DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")
model = smp.PSPNet(encoder_name="resnet34", encoder_weights=None, in_channels=3, classes=len(COLOR_MAP))
checkpoint = torch.load("/content/drive/MyDrive/pspnet_project/pspnet_checkpoint/pspnet_epoch_1.pt", map_location=DEVICE)
model.load_state_dict(checkpoint["model_state_dict"])
model = model.to(DEVICE)
model.eval()
print("✅ 모델 로드 완료")

#  이미지 업로드
uploaded = files.upload()
for filename in uploaded.keys():
    print(f"📄 처리 중: {filename}")
    img = Image.open(BytesIO(uploaded[filename])).convert("RGB")
    img_np = np.array(img)

    # 전처리 및 패딩
    input_tensor = T.ToTensor()(img_np).unsqueeze(0).to(DEVICE)
    input_tensor, orig_size = pad_to_multiple_of_8(input_tensor)

    #  예측
    with torch.no_grad():
        output = model(input_tensor)
        pred = torch.argmax(output, dim=1)[0].cpu().numpy()

    #  패딩 제거하여 원본 크기로 자르기
    pred = pred[:orig_size[0], :orig_size[1]]

    #  인덱스 → RGB
    pred_mask_rgb = index_to_color_mask(pred, COLOR_MAP)

    # 🖼 시각화
    plt.figure(figsize=(12, 5))
    plt.subplot(1, 2, 1)
    plt.imshow(img_np)
    plt.title("Original Image")
    plt.axis("off")

    plt.subplot(1, 2, 2)
    plt.imshow(pred_mask_rgb)
    plt.title("Predicted Mask")
    plt.axis("off")

    plt.tight_layout()
    plt.show()
