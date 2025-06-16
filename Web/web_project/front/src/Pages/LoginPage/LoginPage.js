// src/Pages/LoginPage/LoginPage.js
import React, { useState, useRef, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import axios from 'axios';

import EasyLogo from '../../assets/logo.png';   // ⬅️ PNG 로고
import styles   from './LoginPage.module.css';             // CSS 모듈

export default function LoginPage({ onLogin, isLoggedIn }) {
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const usernameInputRef = useRef(null);
  const navigate = useNavigate();

  /* 로그인 성공 시 홈으로 리디렉션 */
  useEffect(() => {
    if (isLoggedIn) navigate('/');
  }, [isLoggedIn, navigate]);

  /* 로그인 버튼 클릭 */
  const handleLoginSubmit = async () => {
    try {
      await axios.post('/login', { username, password });
      onLogin(true);
      navigate('/');
    } catch (error) {
      alert(`로그인 오류: ${error.response?.data || error.message}`);
    }
  };

  /* 엔터 키 로그인 */
  const loginKeyPress = (e) => {
    if (e.key === 'Enter') handleLoginSubmit();
  };

  return (
    <div className={styles.loginBackground}>
      <div className={styles.formWrapper}>
        {/* ─── 로고 / 상단 ─────────────────────────── */}
        <div className={styles.topSection}>
          <img src={EasyLogo} alt="로고" width={80} height={80} />
          <span className={styles.brand}>E A S Y</span>
        </div>

        {/* ─── 아이디 입력 ─────────────────────────── */}
        <div className={styles.inputGroup}>
          <label>아이디</label>
          <input
            ref={usernameInputRef}
            name="username"
            placeholder="아이디를 입력해주세요"
            value={username}
            onChange={(e) => setUsername(e.target.value)}
          />
        </div>

        {/* ─── 비밀번호 입력 ───────────────────────── */}
        <div className={styles.inputGroup}>
          <label>비밀번호</label>
          <input
            type="password"
            name="password"
            placeholder="비밀번호를 입력해주세요"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            onKeyDown={loginKeyPress}
          />
        </div>

        {/* ─── 버튼 영역 ───────────────────────────── */}
        <button className={styles.loginBtn} onClick={handleLoginSubmit}>
          로그인
        </button>
        <button
          className={styles.registerBtn}
          onClick={() => navigate('/register')}
        >
          회원가입 하러가기
        </button>
      </div>
    </div>
  );
}
