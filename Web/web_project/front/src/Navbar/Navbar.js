import { useNavigate } from "react-router-dom";
import React, { useState } from 'react';
import { ReactComponent as MyLogo } from "./../EasyLogo2.svg";
import "./Navbar.css";
import axios from "axios";

export default function Navbar({ storeCheckLoggedIn, isLoggedIn, handleLogout }) {
  const [searchValue, setSearchValue] = useState('');
  const navigate = useNavigate();

  const sellWriteCheckLoggedIn = async () => {
    try {
      const response = await axios.get("/checkLogin");
      const data = response.data.isLoggedIn;
      if (!data) {
        navigate("/login");
      } else {
        navigate("/sellWrite");
      }
    } catch (error) {
      console.error("에러:", error);
    }
  };

  const auctionWriteCheckLoggedIn = async () => {
    try {
      const response = await axios.get("/checkLogin");
      const data = response.data.isLoggedIn;
      if (!data) {
        navigate("/login");
      } else {
        navigate("/auctionWrite");
      }
    } catch (error) {
      console.error("에러:", error);
    }
  };

  const handleLoginClick = () => {
    navigate('/login');
  };

  const handleChatClick = async () => {
      try {
      const response = await axios.get("/checkLogin");
      const data = response.data.isLoggedIn;
      const userId = response.data.userId;
      if (!data) {
        navigate("/login");
      } else {
        navigate(`/chatList/${userId}`);
      }
    } catch (error) {
      console.error("에러:", error);
    }
  };

  const doSearch = async () => {
    if (!searchValue) {
      return;
    }
    try {
      setSearchValue('');
      navigate(`/search/${searchValue}`);
    } catch (error) {
      alert('검색 실패: ' + error.message);
    }
  };
  
  const handleKeyDown = (event) => {
    if (event.keyCode === 13) {  // keyCode 13은 Enter 키를 의미.
      doSearch();
    }
  };

  return (
    <nav className="navbar">
      <div
        onClick={() => navigate("/")}
        className="navbar_inside navbar_inside1"
      >
        <div>
          <MyLogo />
        </div>
        <div className="homepagetitle">
          <span>E A S Y</span>
        </div>
      </div>
      <div className="navbar_inside navbar_inside2">
        <div onClick={() => storeCheckLoggedIn(navigate)} className="mypage">
          <span>마이페이지</span>
        </div>
        {isLoggedIn ? (
          <div onClick={() => handleLogout(navigate)} className="logout">
            <span>로그아웃</span>
          </div>
        ) : (
          <div onClick={handleLoginClick} className="login">
            <span>로그인</span>
          </div>
        )}
        <div onClick={sellWriteCheckLoggedIn} className="sell-write">
          <span>판매 등록</span>
        </div>
        <div onClick={auctionWriteCheckLoggedIn} className="aution-write">
          <span>역경매 등록</span>
        </div>
        <div onClick={handleChatClick} className="nav-chat">
          <span>채팅</span>
        </div>
        <div className="inputspace">
          <input 
            type="text" 
            placeholder="검색" 
            value={searchValue}
            onChange={(e) => setSearchValue(e.target.value)}
            onKeyDown={handleKeyDown}
          />
          <button type='submit' onClick={doSearch}>검색</button>
        </div>
      </div>
    </nav>
  );
}
