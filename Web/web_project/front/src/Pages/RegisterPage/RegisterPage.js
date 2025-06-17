import { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import axios from 'axios';

import logoImg from '../../assets/logo.png';
import styles  from './RegisterPage.module.css';

export default function RegisterPage() {
  const nav = useNavigate();

  /* ── 상태 ─────────────────────────── */
  const [form, setForm] = useState({
    userId:        '',
    password:      '',
    name:          '',
    zip:    '',
    add1:       '',
    add2: '',
    phone:         '',
    email:         '',
  });
  const [ready, setReady] = useState(false);
  const [error, setError] = useState('');

  /* 모든 항목 입력 여부 체크 */
  useEffect(() => {
    setReady(Object.values(form).every((v) => v.trim()));
  }, [form]);

  const onChange = (e) => {
    const { name, value } = e.target;
    setForm((prev) => ({ ...prev, [name]: value }));
  };

  const onSubmit = async (e) => {
    e.preventDefault();
    if (!ready) return;
    try {
      await axios.post('/api/register', form);   // 실제 API 로 교체하세요
      alert('회원가입 완료! 로그인 해주세요.');
      nav('/login');
    } catch (err) {
      setError(err.response?.data || err.message);
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
        <h1 className={styles.title}>회원가입</h1>

        <form className={styles.form} onSubmit={onSubmit}>
          {/* 아이디 */}
          <label className={styles.field}>
            <span>아이디</span>
            <input
              name="userId"
              value={form.userId}
              onChange={onChange}
              placeholder="아이디를 입력하세요."
            />
          </label>

          {/* 비밀번호 */}
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

          {/* 이름 */}
          <label className={styles.field}>
            <span>이름</span>
            <input
              name="name"
              value={form.name}
              onChange={onChange}
              placeholder="이름을 입력하세요."
            />
          </label>

          {/* 우편번호 */}
          <label className={styles.field}>
            <span>우편번호</span>
            <input
              name="zip"
              value={form.zip}
              onChange={onChange}
              placeholder="우편번호를 입력하세요."
            />
          </label>

          {/* 주소 */}
          <label className={styles.field}>
            <span>주소</span>
            <input
              name="add1"
              value={form.add1}
              onChange={onChange}
              placeholder="도로명, 지번을 입력하세요."
            />
          </label>

          {/* 상세주소 */}
          <label className={styles.field}>
            <span>상세주소</span>
            <input
              name="add2"
              value={form.add2}
              onChange={onChange}
              placeholder="건물명 상세주소를 입력하세요. ex) 000동 0000호"
            />
          </label>

          {/* 전화번호 */}
          <label className={styles.field}>
            <span>전화번호</span>
            <input
              name="phone"
              value={form.phone}
              onChange={onChange}
              placeholder="전화번호를 입력하세요."
            />
          </label>

          {/* 이메일 */}
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
