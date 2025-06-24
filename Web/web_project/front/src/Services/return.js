// src/Services/return.js
import axios from 'axios';

/* ------------------------------------------------------------------
   반품 요청 목록 조회
   ------------------------------------------------------------------ */
export const getReturns = () =>
  axios
    .get('/api/return', { withCredentials: true })
    .then((res) => res.data);

/* ------------------------------------------------------------------
   새 반품 요청 생성
   ------------------------------------------------------------------ */
export const createReturn = (payload) =>
  axios
    .post('/api/return', payload, { withCredentials: true })
    .then((res) => res.data);

/* ------------------------------------------------------------------
   반품 요청 수락 (PENDING → IN_PROGRESS)
   ------------------------------------------------------------------ */
export const acceptReturn = (id) =>
  axios
    .post(`/api/return/${id}/accept`, {}, { withCredentials: true })
    .then((res) => res.data);

/* ------------------------------------------------------------------
   반품 로봇 정지 신호 전송
   ------------------------------------------------------------------ */
export const stopReturn = (id) =>
  axios
    .post(`/api/return/${id}/unlock`, {}, { withCredentials: true })
    .then((res) => res.data);

/* ------------------------------------------------------------------
   반품 완료 → 로봇 복귀 신호 전송
   ------------------------------------------------------------------ */
export const returnReturn = (id) =>
  axios
    .post(`/api/return/${id}/complete`, {}, { withCredentials: true })
    .then((res) => res.data);

   // 로그인 사용자의 반품 전체 조회 ---------------------------- */
export const getMyReturns = (userId) =>
  axios.get('/api/return', {
    withCredentials: true,
    params: { userId }          // → ?userId=ymh
  }).then(res => res.data);