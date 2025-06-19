import { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import axios from 'axios';

import logoImg from '../../assets/logo.png';
import styles  from './FindId.module.css';

export default function FindId() {
  const nav = useNavigate();

  /* ── 상태 ─────────────────────────── */
  const [form, setForm] = useState({ name: '', email: '' });
  const [ready, setReady] = useState(false);
  const [error, setError] = useState('');

  useEffect(() => {
    setReady(form.name.trim() && form.email.trim());
  }, [form]);

  const onChange = (e) =>
    setForm((prev) => ({ ...prev, [e.target.name]: e.target.value }));

  /* ── 제출 ─────────────────────────── */
  const onSubmit = async (e) => {
    e.preventDefault();
    if (!ready) return;
    try {
      /*
        백엔드 예시
        POST /api/find-id
        body: { name, email }
        success: { password: 'P@ssword123' }
      */
      const res = await axios.post('/api/find-id', form);
      const { userId } = res.data;
      alert(`아이디는 "${userId}" 입니다.`);
      nav('/login');
    } catch {
      setError('다시 시도해주세요.'); // 이름·이메일 불일치
    }
  };

  /* ── UI ─────────────────────────── */
  return (
    <div className={styles.page}>
      {/* ---------- Header ---------- */}
      <header className={styles.header}>
        <div className={styles.logoBox} onClick={() => nav('/')}>
          <img src={logoImg} alt="logo" />
          <span>SixWheel</span>
        </div>
        <nav className={styles.menu}>
          <span onClick={() => nav('/mypage')}>마이페이지</span>
          <span onClick={() => nav('/login')}>로그인</span>
          <span onClick={() => nav('/category')}>카테고리</span>
        </nav>
      </header>

      {/* ---------- Form ---------- */}
      <main className={styles.wrapper}>
        <h1 className={styles.title}>아이디 찾기</h1>

        <form className={styles.form} onSubmit={onSubmit}>
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
