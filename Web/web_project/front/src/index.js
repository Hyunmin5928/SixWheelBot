// src/index.js
import React from 'react';
import ReactDOM from 'react-dom/client';
import './index.css';   // 전역 스타일
import App from './App'; // 최상위 컴포넌트
import './Styles/globals.css';


const root = ReactDOM.createRoot(document.getElementById('root'));
root.render(
  <React.StrictMode>
    <App />
  </React.StrictMode>
);
