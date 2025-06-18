import React, { useEffect, useState } from 'react';
import { useNavigate, NavLink } from 'react-router-dom';

import logoImg from '../../assets/logo.png';
import styles  from './MyPage.module.css';
import {
  getUserProfile,
  updateUserProfile,
} from '../../Services/user';

export default function MyPage() {
  const nav    = useNavigate();
  const userId = localStorage.getItem('USER_ID') || 'OOO';

  /* ---------- 상태 ---------- */
  const [avatar, setAvatar] = useState(null);
  const [profile, setProfile] = useState({
    userId:   '',
    password: '',
    name:     '',
    zip:      '',
    address:  '',
    detail:   '',
    phone:    '',
    email:    '',
  });
  const [isEdit, setIsEdit] = useState(false);          // 편집 모드

  /* ---------- 프로필 로딩 ---------- */
  useEffect(() => {
    getUserProfile(userId).then(setProfile);
  }, [userId]);

  /* ---------- 입력 변경 ---------- */
  const onChange = (e) =>
    setProfile((p) => ({ ...p, [e.target.name]: e.target.value }));

  /* ---------- 저장 ---------- */
  const save = async () => {
    try {
      await updateUserProfile(userId, profile);
      alert('프로필이 수정되었습니다.');
      setIsEdit(false);
    } catch (e) {
      alert('저장 실패: ' + (e.response?.data || e.message));
    }
  };

  /* ---------- 로그아웃 ---------- */
  const logout = () => {
    localStorage.removeItem('TOKEN');
    localStorage.removeItem('USER_ID');
    nav('/');
  };

  return (
    <div className={styles.page}>
      {/* Header */}
      <header className={styles.header}>
        <div className={styles.logoArea} onClick={() => nav('/home')}>
          <img src={logoImg} alt="logo" />
          <span>SixWheel</span>
        </div>
        <nav className={styles.userMenu}>
          <NavLink to="/mypage" className={styles.menuLink}>마이페이지</NavLink>
          <button onClick={logout} className={styles.menuLink}>로그아웃</button>
          <NavLink to="/category" className={styles.menuLink}>카테고리</NavLink>
        </nav>
      </header>

      {/* Avatar */}
      <section className={styles.avatarSection}>
        <div
          className={styles.avatar}
          style={avatar ? { backgroundImage: `url(${avatar})` } : {}}
        />
        <label className={styles.changeBtn}>
          프로필 사진 변경
          <input
            type="file"
            accept="image/*"
            hidden
            onChange={(e) => {
              const f = e.target.files[0];
              if (f) setAvatar(URL.createObjectURL(f));
            }}
          />
        </label>
      </section>

      {/* 프로필 폼 */}
      <section className={styles.form}>
        <h2 className={styles.sectionTitle}>프로필 정보</h2>

        {[
          ['userId',   '아이디',     true],
          ['password', '비밀번호'],
          ['name',     '이름'],
          ['zip',      '우편번호'],
          ['address',  '주소'],
          ['detail',   '상세주소'],
          ['phone',    '전화번호'],
          ['email',    '이메일'],
        ].map(([key, label, alwaysDisabled]) => (
          <div className={styles.row} key={key}>
            <label>{label}</label>
            <input
              name={key}
              value={profile[key] || ''}
              onChange={onChange}
              disabled={alwaysDisabled || !isEdit}
            />
          </div>
        ))}
      </section>

      {/* 버튼: 변경하기 ↔ 저장하기 */}
      {isEdit ? (
        <button className={styles.save} onClick={save}>저장하기</button>
      ) : (
        <button
          className={styles.edit}
          onClick={() => setIsEdit(true)}
        >
          변경하기
        </button>
      )}
    </div>
  );
}
