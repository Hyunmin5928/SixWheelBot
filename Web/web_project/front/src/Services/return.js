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
   payload 예시)
   {
     userId:  'ymh',
     sender:  { address:'부산 …', detail:'2층' },
     itemType:'전자제품'
   }
   ------------------------------------------------------------------ */
export const createReturn = (payload) =>
  axios
    .post('/api/return', payload, { withCredentials: true })
    .then((res) => res.data);

/* ------------------------------------------------------------------
   반품 요청 수락 (PENDING → ACCEPTED)
   ------------------------------------------------------------------ */
export const acceptReturn = (id) =>
  axios
    .post(`/api/return/${id}/accept`, {}, { withCredentials: true })
    .then((r) => r.data);

/** 로봇 “정지” 명령 전송 (unlock) */
export const stopReturn = (id) =>
  axios
    .post(`/api/return/${id}/unlock`, {}, { withCredentials: true })
    .then((r) => r.data);

/** 반품 완료 → “복귀” 명령 전송 (complete) */
export const returnReturn = (id) =>
  axios
    .post(`/api/return/${id}/complete`, {}, { withCredentials: true })
    .then((r) => r.data);
