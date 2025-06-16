import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import axios from 'axios';

import EasyLogo from '../../assets/logo.png';   // ← PNG 로고
import styles   from './RegisterPage.module.css';

export default function RegisterPage() {
  const nav  = useNavigate();
  const [form, setForm] = useState({ username: '', password: '' });

  /* ── 입력 핸들러 ─────────────────────── */
  const handleChange = (e) => {
    const { name, value } = e.target;
    setForm((f) => ({ ...f, [name]: value }));
  };

  /* ── 회원가입 제출 ───────────────────── */
  const handleSubmit = async () => {
    try {
      await axios.post('/register', form);        // 실제 API 로 교체
      alert('회원가입 완료! 로그인 해주세요.');
      nav('/login');
    } catch (err) {
      alert('회원가입 오류: ' + (err.response?.data || err.message));
    }
  };

  /* ── UI ─────────────────────────────── */
  return (
    <div className={styles.page}>
      <div className={styles.header}>
        <img src={EasyLogo} alt="로고" width={80} height={80} />
        <h2>회원가입</h2>
      </div>

      <div className={styles.formGroup}>
        <label>아이디</label>
        <input
          name="username"
          value={form.username}
          onChange={handleChange}
          placeholder="아이디를 입력하세요"
        />
      </div>

      <div className={styles.formGroup}>
        <label>비밀번호</label>
        <input
          name="password"
          type="password"
          value={form.password}
          onChange={handleChange}
          placeholder="비밀번호를 입력하세요"
        />
      </div>

      <button className={styles.submitBtn} onClick={handleSubmit}>
        회원가입
      </button>
    </div>
  );
}
