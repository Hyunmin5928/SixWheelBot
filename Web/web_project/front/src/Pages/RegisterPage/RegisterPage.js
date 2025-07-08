// src/Pages/RegisterPage/RegisterPage.js
import { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import axios from 'axios';

import logoImg from '../../assets/logo.png';
import styles  from './RegisterPage.module.css';

export default function RegisterPage() {
  const nav = useNavigate();

  /* ---------- 입력 상태 ---------- */
  const [form, setForm] = useState({
    userId: '', password: '', confirmPw: '', name: '',
    zip: '', addr1: '', addr2: '', phone: '', email: '',
  });

  const [pwMatch, setPwMatch] = useState(true);
  const [ready,   setReady]   = useState(false);
  const [error,   setError]   = useState('');

  /* 필드 변경 */
  const onChange = (e) =>
    setForm((p) => ({ ...p, [e.target.name]: e.target.value }));

  /* 필수 입력 & 비밀번호 확인 */
  useEffect(() => {
    setPwMatch(form.password === form.confirmPw);

    const filled =
      form.userId && form.password && form.confirmPw &&
      form.name   && form.zip      && form.addr1 &&
      form.addr2  && form.phone    && form.email;

    setReady(filled && form.password === form.confirmPw);
  }, [form]);

  /* ---------- 제출 ---------- */
  const onSubmit = async (e) => {
    e.preventDefault();
    if (!ready) return;
    try {
      await axios.post('/api/register', {
        userId: form.userId, password: form.password, name: form.name,
        zip: form.zip, addr1: form.addr1, addr2: form.addr2,
        phone: form.phone, email: form.email,
      });
      alert('회원가입 완료! 로그인 해주세요.');
      nav('/login');
    } catch (err) {
      setError(err.response?.data || err.message);
    }
  };

  return (
    <div className={styles.page}>
      {/* ---------- Header ---------- */}
      <header className={styles.header}>
        <div className={styles.logoBox} onClick={() => nav('/')}>
          <img src={logoImg} alt="logo" />
          <span>SixWheel</span>
        </div>
        <nav className={styles.menu}>
          <span onClick={() => nav('/login')}>로그인</span>
          <span onClick={() => nav('/category')}>카테고리</span>
        </nav>
      </header>

      {/* ---------- Form ---------- */}
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

          {/* 비밀번호 확인 */}
          <label className={styles.field}>
            <span>비밀번호 확인</span>
            <input
              type="password"
              name="confirmPw"
              value={form.confirmPw}
              onChange={onChange}
              placeholder="비밀번호를 입력하세요."
              className={!pwMatch && form.confirmPw ? styles.inputError : ''}
            />
            {form.confirmPw && (
              pwMatch
                ? <p className={styles.success}>비밀번호 일치</p>
                : <p className={styles.warn}>비밀번호 불일치</p>
            )}
          </label>

          {/* 이하 이름·주소·전화·이메일 필드 (변경 없음) */}
          <label className={styles.field}>
            <span>이름</span>
            <input name="name" value={form.name} onChange={onChange}
                   placeholder="이름을 입력하세요." />
          </label>

          <label className={styles.field}>
            <span>우편번호</span>
            <input name="zip" value={form.zip} onChange={onChange}
                   placeholder="우편번호를 입력하세요." />
          </label>

          <label className={styles.field}>
            <span>주소</span>
            <input name="addr1" value={form.addr1} onChange={onChange}
                   placeholder="도로명, 지번을 입력하세요." />
          </label>

          <label className={styles.field}>
            <span>상세주소</span>
            <input name="addr2" value={form.addr2} onChange={onChange}
                   placeholder="건물명 상세주소를 입력하세요. ex) 000동 000호" />
          </label>

          <label className={styles.field}>
            <span>전화번호</span>
            <input name="phone" value={form.phone} onChange={onChange}
                   placeholder="전화번호를 입력하세요. ex)01012345678" />
          </label>

          <label className={styles.field}>
            <span>이메일</span>
            <input name="email" value={form.email} onChange={onChange}
                   placeholder="이메일을 입력하세요." />
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
