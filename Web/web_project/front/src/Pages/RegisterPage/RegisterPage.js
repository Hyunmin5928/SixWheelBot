import { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import axios from 'axios';

import logoImg from '../../assets/logo.png';
import styles  from './RegisterPage.module.css';

export default function RegisterPage() {
  const nav = useNavigate();

  /* ── 입력 상태 ─────────────────────── */
  const [form, setForm] = useState({
    userId: '',
    password: '',
    name: '',
    profileName: '',
    address: '',
    phone: '',
    email: '',
  });
  const [ready, setReady] = useState(false);
  const [error, setError] = useState('');

  /* ── 모든 항목 작성 여부 체크 ──────── */
  useEffect(() => {
    setReady(Object.values(form).every((v) => v.trim()));
  }, [form]);

  const onChange = (e) => {
    const { name, value } = e.target;
    setForm((prev) => ({ ...prev, [name]: value }));
  };

  /* ── 제출 ─────────────────────────── */
  const onSubmit = async (e) => {
    e.preventDefault();
    if (!ready) return;
    try {
      // 실제 엔드포인트로 교체
      await axios.post('/api/register', form);
      alert('회원가입 완료! 로그인 해주세요.');
      nav('/login');
    } catch (err) {
      setError(err.response?.data || err.message);
    }
  };

  /* ── UI ───────────────────────────── */
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

      {/* ---------- Form Card ---------- */}
      <main className={styles.formWrapper}>
        <h1 className={styles.title}>회원가입</h1>

        <form className={styles.form} onSubmit={onSubmit}>
          {[
            ['userId', '아이디'],
            ['password', '비밀번호', 'password'],
            ['name', '이름'],
            ['profileName', '프로필 이름'],
            ['address', '주소'],
            ['phone', '전화번호'],
            ['email', '이메일'],
          ].map(([key, label, type = 'text']) => (
            <label key={key} className={styles.field}>
              <span>{label}</span>
              <input
                type={type}
                name={key}
                value={form[key]}
                onChange={onChange}
                placeholder={`${label}을 입력하세요.`}
              />
            </label>
          ))}

          {error && <p className={styles.error}>{error}</p>}

          <button
            type="submit"
            disabled={!ready}
            className={ready ? styles.submit : styles.submitDisabled}
          >
            확인
          </button>
        </form>
      </main>
    </div>
  );
}
