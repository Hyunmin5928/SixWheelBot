// src/App.js
import React from 'react';
import { BrowserRouter, Routes, Route } from 'react-router-dom';

/* ---------- 페이지 컴포넌트 ---------- */
import MainPage          from './Pages/MainPage/MainPage';               // 게스트 메인
import AuthMainPage      from './Pages/AuthMainPage/AuthMainPage';       // 로그인 메인
import LoginPage         from './Pages/LoginPage/LoginPage';
import RegisterPage      from './Pages/RegisterPage/RegisterPage';
import FindId            from './Pages/FindId/FindId';
import FindPw            from './Pages/FindPw/FindPw';
import MyPage            from './Pages/MyPage/MyPage';
import OrderRequestPage  from './Pages/OrderRequestPage/OrderRequestPage';
import OrderDetailPage   from './Pages/OrderDetailPage/OrderDetailPage';
import ReturnRequestPage from './Pages/ReturnRequestPage/ReturnRequestPage';
import ReturnDetailPage  from './Pages/ReturnDetailPage/ReturnDetailPage';
import AdminDashboard    from './Pages/AdminDashboard/AdminDashboard';

export default function App() {
  return (
    <BrowserRouter>
      <Routes>
        {/* 게스트 메인 */}
        <Route path="/"      element={<MainPage />} />

        {/* 로그인 후 메인 */}
        <Route path="/home"  element={<AuthMainPage />} />

        {/* 인증 · 계정 */}
        <Route path="/login"    element={<LoginPage />} />
        <Route path="/register" element={<RegisterPage />} />
        <Route path="/findid"   element={<FindId />} />
        <Route path="/findpw"   element={<FindPw />} />

        {/* 사용자 · 주문 · 반품 */}
        <Route path="/mypage"        element={<MyPage />} />
        <Route path="/order/new"     element={<OrderRequestPage />} />
        <Route path="/order/:id"     element={<OrderDetailPage />} />
        <Route path="/return/new"    element={<ReturnRequestPage />} />
        <Route path="/return/:id"    element={<ReturnDetailPage />} />

        {/* 관리자 대시보드 */}
        <Route path="/admin/*"    element={<AdminDashboard />} />
      </Routes>
    </BrowserRouter>
  );
}
