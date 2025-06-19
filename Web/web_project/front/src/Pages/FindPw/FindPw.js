import { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import axios from 'axios';

import logoImg from '../../assets/logo.png';
import styles  from './FindPw.module.css';

export default function FindPw() {
  const nav = useNavigate();
  const [form, setForm]   = useState({ userId:'', name:'', email:'' });
  const [error, setError] = useState('');

  const onChange = e => {
    setError('');
    setForm(prev => ({ ...prev, [e.target.name]: e.target.value }));
  };

  const ready = form.userId && form.name && form.email;

  const onSubmit = async e => {
    e.preventDefault();
    if (!ready) return;

    try {
      const res = await axios.post('/api/find-pw', form);

      if (res.data.ok) {
        // 일치 확인되면 재설정 페이지로 이동
        nav('/reset-pw', { state: { userId: form.userId } });
      }
    } catch (err) {
      if (err.response?.status === 404) {
        setError('잘못된 회원 정보입니다.');
      } else {
        setError(err.response?.data || err.message);
      }
    }
  };

  return (
    <div className={styles.page}>
      <header className={styles.header}>
        <div className={styles.logoBox} onClick={() => nav('/')}>
          <img src={logoImg} alt="logo" /><span>SixWheel</span>
        </div>
        <nav className={styles.menu}>
          <span onClick={() => nav('/login')}>로그인</span>
          <span onClick={() => nav('/category')}>카테고리</span>
        </nav>
      </header>

      <main className={styles.wrapper}>
        <h1 className={styles.title}>PW 찾기</h1>
        <form className={styles.form} onSubmit={onSubmit}>
          <label className={styles.field}>
            <span>아이디</span>
            <input
              name="userId"
              value={form.userId}
              onChange={onChange}
              placeholder="아이디를 입력하세요."
            />
          </label>

          <label className={styles.field}>
            <span>이름</span>
            <input
              name="name"
              value={form.name}
              onChange={onChange}
              placeholder="이름을 입력하세요."
            />
          </label>

          <label className={styles.field}>
            <span>이메일</span>
            <input
              name="email"
              value={form.email}
              onChange={onChange}
              placeholder="이메일을 입력하세요."
            />
          </label>

          {error && <p className={styles.error}>{error}</p>}

          <button
            type="submit"
            className={ready ? styles.submit : styles.submitDisabled}
            disabled={!ready}
          >
            확인
          </button>
        </form>
      </main>
    </div>
  );
}
