import { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import axios from 'axios';

import logoImg from '../../assets/logo.png';
import styles  from './FindPw.module.css';

export default function FindPw() {
  const nav = useNavigate();

  /* ── 상태 ─────────────────────────── */
  const [form, setForm] = useState({
    userId: '',
    name:   '',
    email:  '',
  });
  const [ready, setReady] = useState(false);
  const [error, setError] = useState('');

  /* 입력 체크 */
  useEffect(() => {
    setReady(form.userId.trim() && form.name.trim() && form.email.trim());
  }, [form]);

  const onChange = (e) =>
    setForm((p) => ({ ...p, [e.target.name]: e.target.value }));

  /* ── 제출 ─────────────────────────── */
  const onSubmit = async (e) => {
    e.preventDefault();
    if (!ready) return;
    try {
      /*
        POST /api/find-pw
        body: { userId, name, email }
        success: { password: '******' }
      */
      const res = await axios.post('/api/find-pw', form);
      const { password } = res.data;
      alert(`비밀번호는 "${password}" 입니다.`);
      nav('/login');
    } catch {
      setError('다시 시도해주세요.');
    }
  };

  /* ── UI ──────────────────────────── */
  return (
    <div className={styles.page}>
      {/* Header */}
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

      {/* Form */}
      <main className={styles.wrapper}>
        <h1 className={styles.title}>비밀번호 찾기</h1>

        <form className={styles.form} onSubmit={onSubmit}>
          {[
            ['userId', '아이디', '아이디를 입력하세요.'],
            ['name',   '이름',   '이름을 입력하세요.'],
            ['email',  '이메일', '이메일을 입력하세요.'],
          ].map(([key, label, ph]) => (
            <label key={key} className={styles.field}>
              <span>{label}</span>
              <input
                name={key}
                value={form[key]}
                onChange={onChange}
                placeholder={ph}
              />
            </label>
          ))}

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
