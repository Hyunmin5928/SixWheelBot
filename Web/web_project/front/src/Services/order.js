// src/Services/order.js
import axios from 'axios';

/* ------------------------------------------------------------------
   배송 요청 목록 조회
   ------------------------------------------------------------------ */
export const getOrders = () =>
  axios
    .get('/api/order', { withCredentials: true }) // 세션 쿠키 전송
    .then((res) => res.data);

/* ------------------------------------------------------------------
   새 배송 요청 생성
   ------------------------------------------------------------------ */
export const createOrder = (payload) =>
  axios
    .post('/api/order', payload, { withCredentials: true })
    .then((res) => res.data);

/* ------------------------------------------------------------------
   배송 요청 수락 (PENDING → IN_PROGRESS)
   ------------------------------------------------------------------ */
export const acceptOrder = (id) =>
  axios
    .post(`/api/order/${id}/accept`, {}, { withCredentials: true })
    .then((res) => res.data);

/* ------------------------------------------------------------------
   배송 로봇 정지 신호 전송
   ------------------------------------------------------------------ */
export const stopOrder = (id) =>
  axios
    .post(`/api/order/${id}/unlock`, {}, { withCredentials: true })
    .then((res) => res.data);

/* ------------------------------------------------------------------
   배송 완료 → 로봇 복귀 신호 전송
   ------------------------------------------------------------------ */
export const returnOrder = (id) =>
  axios
    .post(`/api/order/${id}/complete`, {}, { withCredentials: true })
    .then((res) => res.data);
