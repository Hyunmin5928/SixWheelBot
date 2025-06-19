import argparse
import torch
import torch.nn.functional as F
import numpy as np
from PIL import Image
import segmentation_models_pytorch as smp

import time
from datetime import datetime

# 🔁 COLOR_MAP 정의 (원본 그대로)
COLOR_MAP = {
    (230, 170, 255): 0, (0, 0, 255): 1, (0, 255, 0): 2, (255, 128, 255): 3,
    (255, 128, 0): 4, (255, 192, 0): 5, (255, 230, 153): 6, (255, 0, 0): 7,
    (138, 60, 200): 8, (55, 86, 35): 9, (217, 217, 217): 10, (110, 168, 70): 11,
    (198, 89, 17): 12, (128, 128, 128): 13, (255, 155, 155): 14, (88, 38, 128): 15,
    (255, 0, 255): 16, (208, 88, 255): 17, (0, 0, 0): 18
}

def index_to_color_mask(index_mask, color_map):
    h, w = index_mask.shape
    color_mask = np.zeros((h, w, 3), dtype=np.uint8)
    for rgb, idx in color_map.items():
        color_mask[index_mask == idx] = rgb
    return color_mask

def pad_to_multiple_of_8(img_tensor):
    _, _, h, w = img_tensor.shape
    new_h = ((h + 7) // 8) * 8
    new_w = ((w + 7) // 8) * 8
    pad_h = new_h - h
    pad_w = new_w - w
    # (left, right, top, bottom)
    padding = (0, pad_w, 0, pad_h)
    padded = F.pad(img_tensor, padding, mode="reflect")
    return padded, (h, w)

def main():
    parser = argparse.ArgumentParser(description="Semantic segmentation on Raspberry Pi")
    parser.add_argument("input",  help="입력 이미지 파일 경로")
    parser.add_argument("output", help="출력 마스크 이미지 저장 경로")
    parser.add_argument("--checkpoint", default="pspnet_epoch_3.pt",
                        help="학습된 모델 체크포인트 파일 경로")
    args = parser.parse_args()

    start_time = time.time()
    print(f"[{datetime.now()}] 🏁 스크립트 시작")

    # 1) 장치 설정 (CPU만 사용)
    device = torch.device("cpu")

    # 2) 모델 로드
    model = smp.PSPNet(
        encoder_name="resnet34",
        encoder_weights=None,
        in_channels=3,
        classes=len(COLOR_MAP)
    )
    checkpoint = torch.load(args.checkpoint, map_location=device)
    model.load_state_dict(checkpoint)
    model.to(device)
    model.eval()
    print(f"[{datetime.now()}] ✅ 모델 로드 완료")

    # 3) 이미지 로드 및 전처리
    img = Image.open(args.input).convert("RGB")
    img_np = np.array(img)
    # ToTensor: [0,255] -> [0,1], CxHxW
    img_tensor = torch.from_numpy(img_np).permute(2,0,1).float().div(255.0).unsqueeze(0).to(device)
    img_tensor, (orig_h, orig_w) = pad_to_multiple_of_8(img_tensor)

    # 4) 추론
    infer_start = time.time()
    with torch.no_grad():
        output = model(img_tensor)
        pred = torch.argmax(output, dim=1)[0].cpu().numpy()
    # 5) 패딩 제거
    pred = pred[:orig_h, :orig_w]
    infer_end = time.time()
    print(f"[{datetime.now()}] 🔮 추론 완료  (소요: {infer_end - infer_start:.3f}초)")

    # 6) 색상 마스크로 변환
    mask_rgb = index_to_color_mask(pred, COLOR_MAP)
    mask_img = Image.fromarray(mask_rgb)
    print(f"[{datetime.now()}] 💾 마스크 저장 완료: {args.output}")

    # 7) 저장
    mask_img.save(args.output)
    print(f"예측 마스크를 {args.output}에 저장했습니다.")
    end_time = time.time()
    print(f"[{datetime.now()}] 🏁 스크립트 종료 (총 소요: {end_time - start_time:.3f}초)")

if __name__ == "__main__":
    main()
