import { Route, Routes } from 'react-router-dom';
import './App.css';
import RegisterForm from './RegisterPage/RegisterForm.js';
import LoginForm from './LoginPage/LoginForm.js';
import axios from 'axios';
import MainPage from './MainPage/MainPage.js';
import StorePage from './StorePage/StorePage.js';
import Navbar from './Navbar/Navbar.js';
import SellWritePage from './WritePage/SellWritePage.js';
import RevAuctionWrite from './WritePage/RevAuctionWrite.js';
import Detail from './Detail/Detail.js';
import MoreSee from './MoreSee/MoreSee.js';
import Edit from './Edit/Edit.js';
import RevAuctionDetail from './Detail/RevAuctionDetail.js';
import SearchResult from './Search/SearchResult.js';
import Chat from './Chat/Chat.js';
import ChatList from './ChatList/ChatList.js';
import { useState, useEffect } from 'react';

function App() {
  // 페이지가 새로고침되어도 상태가 유지되도록 localStorage에서 데이터 가져오기
  const [isLoggedIn, setIsLoggedIn] = useState(localStorage.getItem('isLoggedIn') === 'true');
  let userObjId = '';
  useEffect(() => {
    const mainCheckLoggedIn = async () => {
      try {
        const response = await axios.get('/storeCheckLoggedIn');
        const { loggedIn } = response.data;
        if (loggedIn) {
          setIsLoggedIn(true);
          // 로그인 상태와 사용자 이름을 localStorage에 저장
          localStorage.setItem('isLoggedIn', true);
        }
      } catch (error) {
        console.error('오류:', error);
      }
    };

    mainCheckLoggedIn();
  }, []);

  const storeCheckLoggedIn = async (navigate) => {
    try {
      const response = await axios.get('/storeCheckLoggedIn');
      const { loggedIn, _id } = response.data;
      if (!loggedIn) {
        navigate('/login');
      } else {
        userObjId = _id
        navigate(`/store/${userObjId}`);
        // 로그인 상태와 사용자 이름을 localStorage에 저장
        localStorage.setItem('isLoggedIn', true);
      }
    } catch (error) {
      console.error('오류:', error);
    }
  };


  const handleLogout = async (navigate) => {
    try {
      await axios.get('/logout');
      setIsLoggedIn(false);
      navigate('/');
      // 로그아웃 시 localStorage의 데이터 삭제
      localStorage.removeItem('isLoggedIn');
    } catch (err) {
      alert('로그아웃 에러: ', err);
    }
  };

  return (
    <div className="App">
      <Navbar
        storeCheckLoggedIn={storeCheckLoggedIn}
        isLoggedIn={isLoggedIn}
        handleLogout={handleLogout}
      />
      <Routes>
        <Route path='/' element={<MainPage />} />
        <Route path='/login' element={<LoginForm isLoggedIn ={isLoggedIn} onLogin={(loggedIn) => setIsLoggedIn(loggedIn)} />} />
        <Route path='/register' element={<RegisterForm />} />
        <Route path='/store/:userObjId' element={<StorePage />} />
        <Route path='/search/:searchValue' element={<SearchResult />}/>
        <Route path='/sellWrite' element={<SellWritePage />} />
        <Route path='/auctionWrite' element={<RevAuctionWrite />} />
        <Route path='/detail/:id' element={<Detail />} />
        <Route path='/revAuctionDetail/:id' element={<RevAuctionDetail />} />
        <Route path='/salesPostsEdit/:id' element={<Edit />} />
        <Route path='/chat/detail/:roomId' element={<Chat />} />
        <Route path='/chatList/:userId' element={<ChatList />} />
        <Route path='/moreSee/newProductPosts' element={<MoreSee
                  moreSeeTitle = {'새로 올라온 새상품!'}
                  type = {'새상품'}
                  sortBy = {'date'}
        />} />
        <Route path='/moreSee/usedProductPosts' element={<MoreSee 
                  moreSeeTitle = {'새로 올라온 중고품!'}
                  type = {'중고'}
                  sortBy = {'date'}
        />} />
        <Route path='/moreSee/popularNewProducts' element={<MoreSee 
                  moreSeeTitle = {'실시간 인기 새상품!'}
                  type = {'새상품'}
                  sortBy = {'viewer'}
        />} />
        <Route path='/moreSee/popularUsedProducts' element={<MoreSee
                  moreSeeTitle = {'실시간 인기 중고상품!'}
                  type = {'중고'}
                  sortBy = {'viewer'}
        />} />
        <Route path='*' element={<div>없는 페이지임</div>} />
      </Routes>
    </div>
  );
}

export default App;
