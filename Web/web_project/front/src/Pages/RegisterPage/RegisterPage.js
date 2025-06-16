// src/Pages/RegisterPage/RegisterPage.js
import React, { useState, useEffect } from 'react';
import { ReactComponent as EasyLogo } from '../../SixWheelBotLogo.svg';
import styles from './RegisterPage.module.css';

export default function RegisterPage({ onLogin }) {
  const [form, setForm] = useState({ username: '', password: '' });

  // 예시 useEffect: onLogin 상태 변화에 따른 처리
  useEffect(() => {
    // 만약 부모로부터 로그인 상태(state)가 props로 넘어온다면
    // 여기서 처리하거나, dependency 배열을 수정하세요.
  }, [/* state 또는 onLogin 등 */]);

  const handleChange = (e) => {
    const { name, value } = e.target;
    setForm((f) => ({ ...f, [name]: value }));
  };

  const handleSubmit = async () => {
    try {
      // 실제 axios 호출로 교체
      // await axios.post('/register', form);
      console.log('회원가입:', form);
      onLogin(true);
    } catch (err) {
      alert('회원가입 오류: ' + err.message);
    }
  };

  return (
    <div className={styles.page}>
      <div className={styles.header}>
        <EasyLogo width={80} height={80} />
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
