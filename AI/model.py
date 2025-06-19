# ✅ PSPNet Semantic Segmentation 학습 스크립트 (Colab 용)

import os
import numpy as np
from PIL import Image
from glob import glob
import torch
from torch.utils.data import Dataset, DataLoader
import torchvision.transforms as T
import segmentation_models_pytorch as smp
from tqdm import tqdm
import matplotlib.pyplot as plt
from sklearn.model_selection import train_test_split
from collections import defaultdict

# ✅ CUDA 사용 가능 여부 확인
DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")
print(f"✅ Using device: {DEVICE}")

# ⚙️ 1. COLOR_MAP & CLASS_NAMES (19 classes)
COLOR_MAP = {
    (230, 170, 255): 0,  # alley_normal
    (0, 0, 255): 1,       # sidewalk_blocks
    (0, 255, 0): 2,       # caution_zone_tree_zone
    (255, 128, 255): 3,   # roadway_normal
    (255, 128, 0): 4,     # caution_zone_grating
    (255, 192, 0): 5,     # caution_zone_stairs
    (255, 230, 153): 6,   # sidewalk_soil_stone
    (255, 0, 0): 7,       # caution_zone_manhole
    (138, 60, 200): 8,    # alley_speed_dump
    (55, 86, 35): 9,      # sidewalk_damaged
    (217, 217, 217): 10,  # sidewalk_cement
    (110, 168, 70): 11,   # sidewalk_other
    (198, 89, 17): 12,    # sidewalk_urethane
    (128, 128, 128): 13,  # sidewalk_asphalt
    (255, 155, 155): 14,  # bike_lane
    (88, 38, 128): 15,    # alley_damaged
    (255, 0, 255): 16,    # sidewalk_crosswalk
    (208, 88, 255): 17,   # roadway_crosswalk
    (0, 0, 0): 18         # background
}

# 📦 2. 마스크 색상 → 인덱스 변환 함수
def color_to_index_mask(mask, color_map):
    h, w, _ = mask.shape
    index_mask = np.zeros((h, w), dtype=np.uint8)
    for rgb, idx in color_map.items():
        match = np.all(mask == rgb, axis=-1)
        index_mask[match] = idx
    return index_mask

# 🧱 3. Dataset 클래스 정의
class SegmentationDataset(Dataset):
    def __init__(self, image_paths, mask_paths, color_map, transform=None):
        self.image_paths = image_paths
        self.mask_paths = mask_paths
        self.color_map = color_map
        self.transform = transform

    def __len__(self):
        return len(self.image_paths)

    def __getitem__(self, idx):
        img = Image.open(self.image_paths[idx]).convert('RGB')
        mask = Image.open(self.mask_paths[idx]).convert('RGB')

        img = np.array(img)
        mask = np.array(mask)
        label = color_to_index_mask(mask, self.color_map)

        img = T.ToTensor()(img)
        label = torch.from_numpy(label).long()
        return img, label

# 📁 4. 데이터 경로 불러오기 및 매칭된 리스트 기반으로 필터링
image_paths = []
mask_paths = []
for n in range(1, 6):
    image_paths += glob(f'/content/drive/MyDrive/pspnet_project/surface_data_1/Surface_*/MP_SEL_SUR_*.jpg')
    mask_paths  += glob(f'/content/drive/MyDrive/pspnet_project/surface_data_1/Surface_*/MASK/MP_SEL_SUR_*.png')

image_paths = sorted(image_paths)
mask_paths = sorted(mask_paths)

# 📦 이미지-마스크 매칭 사전 구축 및 공통 키 필터링
image_dict = {os.path.basename(p).replace(".jpg", ""): p for p in image_paths}
mask_dict  = {os.path.basename(p).replace(".png", ""): p for p in mask_paths}
common_keys = sorted(set(image_dict) & set(mask_dict))
image_paths = [image_dict[k] for k in common_keys]
mask_paths  = [mask_dict[k] for k in common_keys]

# surface_data 그룹별 샘 수 확인용
folder_counts = defaultdict(int)
for path in image_paths:
    for i in range(1, 6):
        if f"surface_data_{i}" in path:
            folder_counts[f"surface_data_{i}"] += 1

print("📊 surface_data 별 매칭 이미지 수:")
for k in sorted(folder_counts.keys()):
    print(f"{k}: {folder_counts[k]}")

# 📦 5. Train/Validation 분할 및 DataLoader 준비
train_imgs, val_imgs, train_masks, val_masks = train_test_split(
    image_paths, mask_paths, test_size=0.2, random_state=42
)

train_dataset = SegmentationDataset(train_imgs, train_masks, COLOR_MAP)
val_dataset = SegmentationDataset(val_imgs, val_masks, COLOR_MAP)

train_loader = DataLoader(train_dataset, batch_size=4, shuffle=True, num_workers=2)
val_loader = DataLoader(val_dataset, batch_size=4, shuffle=False, num_workers=2)

# 🧠 6. PSPNet 모델 준비
model = smp.PSPNet(
    encoder_name="resnet34",
    encoder_weights="imagenet",
    in_channels=3,
    classes=len(COLOR_MAP),
)
model = model.to(DEVICE)

# ⚙️ 7. Optimizer & Loss
optimizer = torch.optim.Adam(model.parameters(), lr=1e-4)
criterion = torch.nn.CrossEntropyLoss()

# 🌀 8. 학습 루프 + 체크포인트 저장
os.makedirs("/content/drive/MyDrive/pspnet_project/pspnet_checkpoint", exist_ok=True)

for epoch in range(1, 6):  # 5 에폭만 예시
    model.train()
    total_loss = 0

    for imgs, masks in tqdm(train_loader):
        imgs, masks = imgs.to(DEVICE), masks.to(DEVICE)
        optimizer.zero_grad()
        outputs = model(imgs)
        loss = criterion(outputs, masks)
        loss.backward()
        optimizer.step()
        total_loss += loss.item()

    print(f"[Epoch {epoch}] Train Loss: {total_loss / len(train_loader):.4f}")

    # 🔍 Validation
    model.eval()
    val_loss = 0
    with torch.no_grad():
        for imgs, masks in val_loader:
            imgs, masks = imgs.to(DEVICE), masks.to(DEVICE)
            outputs = model(imgs)
            loss = criterion(outputs, masks)
            val_loss += loss.item()
    print(f"[Epoch {epoch}] Val Loss: {val_loss / len(val_loader):.4f}")

    # ✅ 체크포인트 저장
    ckpt_path = f"/content/drive/MyDrive/pspnet_project/pspnet_checkpoint/pspnet_epoch_{epoch}.pt"
    torch.save(model.state_dict(), ckpt_path)
    print(f"✅ 모델 저장됨: {ckpt_path}")

# 🎨 9. 시각화 함수 (선택)
def visualize_sample(model, dataset, idx=0):
    model.eval()
    with torch.no_grad():
        img, label = dataset[idx]
        img = img.to(DEVICE)
        output = model(img.unsqueeze(0))[0].argmax(0).cpu().numpy()

        fig, ax = plt.subplots(1, 2, figsize=(10, 5))
        ax[0].imshow(label.numpy())
        ax[0].set_title("GT")
        ax[1].imshow(output)
        ax[1].set_title("Pred")
        plt.show()

# 사용 예시
# visualize_sample(model, val_dataset, idx=10)
