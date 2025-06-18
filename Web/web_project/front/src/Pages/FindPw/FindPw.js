import { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import axios from 'axios';

import logoImg from '../../assets/logo.png';
import styles  from './FindPw.module.css';

export default function FindPw() {
  const nav = useNavigate();

  // 입력 상태
  const [form, setForm]   = useState({ userId: '', name: '', email: '' });
  const [error, setError] = useState('');

  // 입력값 변경 시
  const onChange = e => {
    setError('');
    setForm(prev => ({ ...prev, [e.target.name]: e.target.value }));
  };

  // 세 칸 모두 채워졌는지
  const ready = form.userId && form.name && form.email;

  // 확인 클릭
  const onSubmit = async e => {
    e.preventDefault();
    if (!ready) return;

    try {
      const res = await axios.post('/api/find-pw', form);

      // 일치 확인되면 reset-pw 페이지로
      if (res.data.ok) {
        nav('/reset-pw', { state: { userId: form.userId } });
      } else {
        setError('잘못된 회원 정보입니다.');
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
      {/* Header */}
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

      {/* Form */}
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
