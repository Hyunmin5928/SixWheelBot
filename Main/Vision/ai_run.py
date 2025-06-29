#!/usr/bin/env python3
"""
server.py
 1) JPEG 수신 → 2) 세그멘테이션(idx_map + 컬러 mask)
 3) angle(idx_map 기반) → 4) 파일 저장 + 마스크 색·인덱스 확인 → 5) 각도 전송
"""

import socket, struct, time
from pathlib import Path
import cv2, numpy as np, torch

# ────────────────────────────────── 설정 ─────────────────────────────────
HOST, PORT = "0.0.0.0", 9999
MODEL_PATH = Path(__file__).with_name("pspnet_resnet34_8_scripted.pt")
DEVICE     = "cuda" if torch.cuda.is_available() else "cpu"
INPUT_SIZE = 256

FORBIDDEN = {2, 3, 17,18}         # 도로, 나무 보호대, 횡단보도
def rgb(r,g,b): return (r<<16)|(g<<8)|b
COLOR_MAP = {
    rgb(230,170,255):0, rgb(0,0,255):1,   rgb(0,255,0):2,
    rgb(255,128,255):3, rgb(255,128,0):4, rgb(255,192,0):5,
    rgb(255,230,153):6, rgb(255,0,0):7,   rgb(138,60,200):8,
    rgb(55,86,35):9,    rgb(217,217,217):10, rgb(110,168,70):11,
    rgb(198,89,17):12,  rgb(128,128,128):13, rgb(255,155,155):14,
    rgb(88,38,128):15,  rgb(255,0,255):16,  rgb(208,88,255):17,
    rgb(0,0,0):18
}
# 팔레트 LUT (index→BGR)
COLOR_LUT = np.zeros((19,3), np.uint8)
for rgb_val, idx in COLOR_MAP.items():
    COLOR_LUT[idx] = [rgb_val & 255, (rgb_val>>8)&255, (rgb_val>>16)&255]

# 역팔레트: BGR 튜플 → 인덱스
BGR2IDX = {tuple(color): idx for idx, color in enumerate(COLOR_LUT)}

# ───────────────────────── 모델 로드 ────────────────────────────
print("🟢 loading", MODEL_PATH)
model = torch.jit.load(str(MODEL_PATH), map_location=DEVICE).eval().to(DEVICE)

@torch.inference_mode()
def seg_infer(img_bgr):
    """BGR 입력 → idx_map, mask_bgr"""
    h0,w0 = img_bgr.shape[:2]
    rgb_rs = cv2.resize(cv2.cvtColor(img_bgr, cv2.COLOR_BGR2RGB),
                        (INPUT_SIZE,INPUT_SIZE))
    t = torch.from_numpy(rgb_rs.transpose(2,0,1)).float().div(255)\
        .unsqueeze(0).to(DEVICE)
    logits = model(t)[0] if model(t).ndim==4 else model(t)
    pred = logits.argmax(0).cpu().numpy().astype(np.uint8)        # 256×256
    idx_map = cv2.resize(pred, (w0,h0), cv2.INTER_NEAREST)
    mask_bgr= cv2.resize(COLOR_LUT[pred], (w0,h0), cv2.INTER_NEAREST)
    return idx_map, mask_bgr

def compute_angle(idx_map):
    h,w = idx_map.shape; y0,cx = h//2, w//2
    allow=forbid=sumX=allowL=allowR = 0
    for y in range(y0,h):
        row = idx_map[y]
        forbid += np.count_nonzero(np.isin(row, list(FORBIDDEN)))
        allow_mask = ~np.isin(row, list(FORBIDDEN))
        allow += int(allow_mask.sum())
        if allow_mask.any():
            xs = np.where(allow_mask)[0]
            sumX += xs.sum()
            allowL += int((xs < cx).sum())
            allowR += int((xs >= cx).sum())

    if allow == 0:
        return 0.0
    if forbid / ((h-y0)*w) <= 0.5:
        avgX = sumX / allow
        return float(np.arctan2(avgX-cx, h/2) * 180/np.pi)
    return -25.0 if allowL > allowR else 25.0

def unique_from_mask(mask_bgr):
    """컬러 마스크 → 등장 인덱스 집합 (팔레트 역변환)"""
    flat = mask_bgr.reshape(-1,3)
    uniq = np.unique(flat, axis=0)
    return sorted({BGR2IDX.get(tuple(c), -1) for c in uniq if tuple(c) in BGR2IDX})

def recvall(sock,n):
    buf=b''
    while len(buf)<n:
        chunk=sock.recv(n-len(buf))
        if not chunk: raise ConnectionError("socket closed")
        buf += chunk
    return buf

# ───────────────────────── 메인 루프 ────────────────────────────
save_idx = 0
with socket.socket() as serv:
    serv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    serv.bind((HOST,PORT)); serv.listen(5)
    print(f"🎮 listening on {HOST}:{PORT}")

    try:
        while True:
            cli, addr = serv.accept()
            with cli:
                print("📥", addr)

                length, = struct.unpack("!I", recvall(cli,4))
                jpg = recvall(cli,length)
                img = cv2.imdecode(np.frombuffer(jpg,np.uint8), cv2.IMREAD_COLOR)
                img = cv2.flip(img, 1)
                if img is None: print("❌ JPEG decode"); continue

                idx = save_idx; save_idx += 1
                cv2.imwrite(f"input_{idx:04d}.jpg", img)

                t0=time.time()
                idx_map, mask = seg_infer(img)
                angle = compute_angle(idx_map)
                cv2.imwrite(f"mask_{idx:04d}.png", mask)
                dt_ms = (time.time()-t0)*1000

                # ── 마스크 색을 거꾸로 인덱스로 환산해 집합 확인
                present = unique_from_mask(mask)
                print(f"[DBG] idx {idx:04d} contains → {present}")
                print(f"↩  angle {angle:.2f}°, {dt_ms:.1f} ms")

                payload = f"{angle:.2f}".encode()
                cli.sendall(struct.pack("!I",len(payload))+payload)

    except KeyboardInterrupt:
        print("\n🛑  server shutdown")
#1. ai 모델 실행 로직
#1-1. 모델 로드 -> 스레드 종료 시 모델 종료
#31-2. input.img를 ai 모델을 통해 추론하는 로직
#2. vision_module.cpp과 통신 하는 로직
#2-1. 스레드 생성 시 넘어오는 명령어(start) -> 모델 로드
#2-2. 스레드가 생성한 img 파일 경로를 참고하여 ai 모델에 넣어주는 로직
#2-3. output.img 생성하고 vision_module.cpp에 전달하는 로직


#ip랑 port는 localhost = 127.0.0.1
#msg = "start", "input.img", "output.img", "stop"

