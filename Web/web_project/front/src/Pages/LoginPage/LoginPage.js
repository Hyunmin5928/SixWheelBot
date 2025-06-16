// src/Pages/LoginPage/LoginPage.js
import React, { useState, useRef, useEffect } from 'react';
import { ReactComponent as MyLogo } from '../../SixWheelBotLogo.svg';
import { useNavigate } from 'react-router-dom';
import axios from 'axios';
import './LoginPage.module.css';

export default function LoginPage({ onLogin, isLoggedIn }) {
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const usernameInputRef = useRef(null);
  const navigate = useNavigate();

  useEffect(() => {
    if (isLoggedIn) {
      navigate('/');
    }
  }, [isLoggedIn, navigate]);

  const handleLoginSubmit = async () => {
    try {
      await axios.post('/login', { username, password });
      onLogin(true);
      navigate('/');
    } catch (error) {
      alert(`로그인 오류: ${error.response?.data || error.message}`);
    }
  };

  const loginKeyPress = (event) => {
    if (event.key === 'Enter') {
      handleLoginSubmit();
    }
  };

  return (
    <div className="login_page_background">
      <div className="login_form_screen">
        <div className="login_form_screen_inside_top">
          <div className="top_logo">
            <MyLogo width={80} height={80} />
          </div>
          <div>
            <span>E A S Y</span>
          </div>
        </div>

        <div className="login_form_input">
          <div><span>아이디</span></div>
          <div>
            <input
              ref={usernameInputRef}
              placeholder="아이디를 입력해주세요"
              name="username"
              value={username}
              onChange={(e) => setUsername(e.target.value)}
            />
          </div>
        </div>

        <div className="login_form_input">
          <div><span>비밀번호</span></div>
          <div>
            <input
              type="password"
              placeholder="비밀번호를 입력해주세요"
              name="password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              onKeyDown={loginKeyPress}
            />
          </div>
        </div>

        <div className="login_form_screen_inside">
          <button
            onClick={handleLoginSubmit}
            type="button"
            className="login_btn"
          >
            로그인
          </button>
          <button
            onClick={() => navigate('/register')}
            type="button"
            className="to_register_btn"
          >
            회원가입 하러가기
          </button>
        </div>
      </div>
    </div>
  );
}
