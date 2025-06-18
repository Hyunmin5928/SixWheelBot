import { useState, useEffect } from 'react';
import { useNavigate, useLocation } from 'react-router-dom';
import axios from 'axios';

import logoImg from '../../assets/logo.png';
import styles  from './ResetPwPage.module.css';

export default function ResetPwPage() {
  const nav       = useNavigate();
  const { state } = useLocation();
  const userId    = state?.userId;  // FindPw에서 넘겨준 userId

  // 직접 URL 접근 방지
  useEffect(() => {
    if (!userId) nav('/findpw');
  }, [userId, nav]);

  const [pw,   setPw]   = useState('');
  const [chk,  setChk]  = useState('');
  const [match, setMatch] = useState(true);
  const [ready, setReady] = useState(false);
  const [error, setError] = useState('');

  useEffect(() => {
    setMatch(pw === chk);
    setReady(pw && chk && pw === chk);
  }, [pw, chk]);

  const onSubmit = async e => {
    e.preventDefault();
    if (!ready) return;

    try {
      await axios.post('/api/reset-pw', { userId, newPassword: pw });
      alert('비밀번호가 재설정되었습니다. 다시 로그인 해주세요.');
      nav('/login');
    } catch (err) {
      setError(err.response?.data || err.message);
    }
  };

  return (
    <div className={styles.page}>
      {/* Header */}
      <header className={styles.header}>
        <div className={styles.logoBox} onClick={() => nav('/')}>
          <img src={logoImg} alt="logo"/><span>SixWheel</span>
        </div>
        <nav className={styles.menu}>
          <span onClick={() => nav('/login')}>로그인</span>
          <span onClick={() => nav('/category')}>카테고리</span>
        </nav>
      </header>

      {/* Form */}
      <main className={styles.wrapper}>
        <h1 className={styles.title}>비밀번호 재설정</h1>
        <form className={styles.form} onSubmit={onSubmit}>
          <label className={styles.field}>
            <span>비밀번호</span>
            <input
              type="password"
              value={pw}
              onChange={e => { setError(''); setPw(e.target.value); }}
              placeholder="비밀번호를 입력하세요."
            />
          </label>

          <label className={styles.field}>
            <span>비밀번호 확인</span>
            <input
              type="password"
              value={chk}
              onChange={e => { setError(''); setChk(e.target.value); }}
              placeholder="비밀번호를 다시 입력하세요."
              className={!match && chk ? styles.inputError : ''}
            />
            {chk && (
              match
                ? <p className={styles.success}>비밀번호 일치</p>
                : <p className={styles.warn}>비밀번호 불일치</p>
            )}
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
