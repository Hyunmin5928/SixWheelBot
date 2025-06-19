import { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import axios from 'axios';

import logoImg from '../../assets/logo.png';
import styles  from './LoginPage.module.css';

export default function LoginPage() {
  const nav = useNavigate();

  const [form, setForm]   = useState({ userId: '', password: '' });
  const [ready, setReady] = useState(false);
  const [error, setError] = useState('');

  useEffect(() => {
    setReady(form.userId.trim() && form.password.trim());
  }, [form]);

  const onChange = (e) =>
    setForm((p) => ({ ...p, [e.target.name]: e.target.value }));

  /* ---------- 수정된 onSubmit ---------- */
  const onSubmit = async (e) => {
    e.preventDefault();
    if (!ready) return;

    try {
      const res = await axios.post('/api/login', form, { withCredentials: true });

      localStorage.setItem('USER_ID', res.data.userId);
      localStorage.setItem('ROLE',    res.data.role);

      if (res.data.role === 'ADMIN') {
        nav('/admin');
      } else {
        nav('/home');
      }
    } catch (err) {
      setError(err.response?.data?.error || err.message);
    }
  };

  return (
    /* --- JSX 는 이전과 동일 --- */
    <div className={styles.page}>
      {/* Header */}
      <header className={styles.header}>
        <div className={styles.logoBox} onClick={() => nav('/')}>
          <img src={logoImg} alt="logo" />
          <span>SixWheel</span>
        </div>
        <nav className={styles.menu}>
          <span onClick={() => nav('/register')}>회원가입</span>
          <span onClick={() => nav('/category')}>카테고리</span>
        </nav>
      </header>

      {/* Form */}
      <main className={styles.wrapper}>
        <h1 className={styles.title}>로그인</h1>
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
            <span>비밀번호</span>
            <input
              type="password"
              name="password"
              value={form.password}
              onChange={onChange}
              placeholder="비밀번호를 입력하세요."
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

        <div className={styles.links}>
          <span onClick={() => nav('/findid')}>ID 찾기</span>
          <span>/</span>
          <span onClick={() => nav('/findpw')}>PW 찾기</span>
        </div>
      </main>
    </div>
  );
}
