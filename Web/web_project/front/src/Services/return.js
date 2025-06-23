// src/Services/return.js
import axios from 'axios';

/* ------------------------------------------------------------------
   반품 요청 목록 조회
   ------------------------------------------------------------------ */
export const getReturns = () =>
  axios
    .get('/api/return', { withCredentials: true }) // 세션 쿠키 포함
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

export const acceptReturn = (id) =>
  axios.post(`/api/return/${id}/accept`, {}, { withCredentials:true })
       .then(r => r.data);    