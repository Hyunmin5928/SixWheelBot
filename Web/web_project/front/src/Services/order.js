// src/Services/order.js
import axios from 'axios';

/* ------------------------------------------------------------------
   배송 요청 목록 조회
   ------------------------------------------------------------------ */
    export const getOrders = (userId = '') =>
    axios.get('/api/order', {
    withCredentials: true,
    params: userId ? { userId } : {},   // ← 쿼리 전달
  })
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

    /* 상세 1건 조회 ---------------------------------------------------- */
export const getOrderById = (id) =>
  axios.get(`/api/order/${id}`, { withCredentials:true })
       .then(res => res.data);

/* 잠금 ----------------------------------------------------------- */
export const lockOrder = (id) =>
  axios.post(`/api/order/${id}/lock`, {}, { withCredentials:true })
       .then(res => res.data);

/* 이미 있던 unlockOrder / completeOrder 는 stopOrder / returnOrder
   이름 그대로 두셔도 되고, 헷갈리면 아래처럼 alias 해도 됩니다. */
export const unlockOrder   = stopOrder;      // 잠금 해제
export const completeOrder = returnOrder;    // 배송 완료