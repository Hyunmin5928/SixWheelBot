import axios from 'axios';

/* ─── 프로필 조회 ─── */
export const getUserProfile = async (userId) => {
  const res = await axios.get(`/api/user/${userId}`);
  return res.data;                       // { userId, password, name, ... }
};

/* ─── 프로필 수정 ─── */
export const updateUserProfile = async (userId, payload) => {
  // payload 예: { password, name, zip, address, detail, phone, email }
  const res = await axios.put(`/api/user/${userId}`, payload);
  return res.data;                       // { updateSuccess:true }
};
