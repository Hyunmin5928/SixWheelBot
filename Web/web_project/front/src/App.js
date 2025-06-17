// src/App.js
import React from 'react';
import { BrowserRouter, Routes, Route } from 'react-router-dom';

import MainPage          from './Pages/MainPage/MainPage';
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
        <Route path="/"               element={<MainPage />} />
        <Route path="/login"          element={<LoginPage />} />
        <Route path="/register"       element={<RegisterPage />} />
        <Route path="/findid"        element={<FindId />} />
        <Route path="/findpw"        element={<FindPw />} />
        <Route path="/mypage"         element={<MyPage />} />
        <Route path="/order/new"      element={<OrderRequestPage />} />
        <Route path="/order/:id"      element={<OrderDetailPage />} />
        <Route path="/return/new"     element={<ReturnRequestPage />} />
        <Route path="/return/:id"     element={<ReturnDetailPage />} />
        <Route path="/admin/*"        element={<AdminDashboard />} />
      </Routes>
    </BrowserRouter>
  );
}
