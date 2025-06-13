import ClipLoader from "react-spinners/ClipLoader";
import axios from 'axios';
import './StorePage.css';
import { useParams } from 'react-router-dom';
import { useNavigate } from 'react-router-dom';
import { useEffect, useState } from 'react';
import Card from './../Card/Card.js'
import RevAuctionCard from './../Card/RevAuctionCard.js';
import StoreRevAuctionCard from './../Card/StoreRevAuctionCard.js';
export default function StorePage() {
    const tabs = [
      { id: 1, title: '판매중 (새상품)'},
      { id: 2, title: '판매중 (중고)'},
      { id: 3, title: '역경매'},
      { id: 4, title: '입찰목록'},
      { id: 5, title: '관심상품'},
      { id: 6, title: '판매완료'}
    ];
    const { userObjId } = useParams();
    const myObjectID = userObjId;
    const [userId, setUserId] = useState(null);
    const [isLoggedIn, setIsLoggedIn] = useState(false);
    const [activeTab, setActiveTab] = useState(tabs[0].id);
    const [storeSalesPosts, setStoreSalesPosts] = useState([]);
    const [newProductsSalesPosts, setNewProductsSalesPosts] = useState([]);
    const [usedProductsSalesPosts, setUsedProductsSalesPosts] = useState([]);
    const [revAuctionPosts, setRevAuctionPosts] = useState([]);
    const [bidProducts, setBidProducts] = useState([]);
    const [interestedProducts, setInterestedProducts] = useState([]);
    const [loading, setLoading] = useState(false);
    const navigate = useNavigate();
    
    useEffect(() => {
      const fetchData = async () => {
        try {
            setLoading(true);
            const loginResponse = await axios.get("/checkLogin");
            if (loginResponse.data.isLoggedIn) {
              setUserId(loginResponse.data.userId);
            }

            const salesPostsResponse = await axios.get('/userSalesPosts', { params: { myObjectID } });
            const salesPosts = salesPostsResponse.data;
          
            const userRevAuctionPostsRes = await axios.get('/userRevAuctionPosts', { params: { myObjectID } });
            const revAuctionPosts = userRevAuctionPostsRes.data;
            setRevAuctionPosts(revAuctionPosts);
            
            const userBidPostsRes = await axios.get('/userBidProducts');
            const bidPosts = userBidPostsRes.data;
            setBidProducts(bidPosts);

            const filteredUsed = salesPosts.filter(post => post.type === '중고');
            const filteredNew = salesPosts.filter(post => post.type === '새상품');

            const interestedProductsResponse = await axios.get('/userInterestedProducts');
            setInterestedProducts(interestedProductsResponse.data);

            setIsLoggedIn(loginResponse.data.isLoggedIn);
            setStoreSalesPosts(salesPosts[0]);
            setNewProductsSalesPosts(filteredNew);
            setUsedProductsSalesPosts(filteredUsed);
        } catch (error) {
            console.error('오류:', error);
        } finally {
          setLoading(false);
        }
      };

      fetchData();
    }, [navigate, myObjectID]);

    

    const selectTab = (tabId) => {
        setActiveTab(tabId);
    };

    const showGrid = () => {
      if (loading) {
        return false;
      }
      if (activeTab === 1 && newProductsSalesPosts.length === 0) {
        return false;
      }
      if (activeTab === 2 && usedProductsSalesPosts.length === 0) {
        return false;
      }
      if (activeTab === 3 && revAuctionPosts.length === 0) {
        return false;
      }
      if (activeTab === 4 && bidProducts.length === 0) {
        return false;
      }
      if (activeTab === 5 && interestedProducts.length === 0) {
        return false;
      }
      if (activeTab === 6) {
        return false;
      }
      return true;
    };
    
    if(loading) {
      return (
        <div className='store-loading-msg'>
          <div className='store-loading-inside'>
            <ClipLoader color="#dcdcdc" loading={loading} size={50} />
            <span>데이터를 불러오고 있어요!</span>
          </div>
        </div>
      )
    }

    //로그인이 되어있고 나의 스토어인지 확인
    const isAuthor = isLoggedIn && userId === storeSalesPosts.authorId;
    
    return (
      <div className="store-page-container">
        <div className="store-page-screen">
          <div className="store-page-name">
            <div className="user-image">
              <i class="bi bi-person-circle"></i>
            </div>
            { isAuthor ?
             <div className="username-hi">{ storeSalesPosts.publisherName } 님 안녕하세요!</div> : 
             <div className="username-hi">{ storeSalesPosts.publisherName } 님의 스토어</div>
            }
          </div>
          <div>
            { isAuthor ? <h1><i class="bi bi-house-fill"></i>&nbsp;&nbsp;나의 상품</h1> : <h1><i class="bi bi-house-fill"></i>&nbsp;&nbsp;{ storeSalesPosts.publisherName } 님의 상품</h1>}
          </div>
          <div className="store-page-tabs">
            <ul className="tab-titles">
              {tabs.filter(tab => isAuthor || [1, 2, 3, 6].includes(tab.id)).map((tab) => (
                <li
                  key={tab.id}
                  className={tab.id === activeTab ? "active" : ""}
                  onClick={() => selectTab(tab.id)}
                >
                  {tab.title}
                </li>
              ))}
            </ul>
          </div>
          <div className={showGrid() ? "store-page-products-grid" : "store-page-products"}>
            {activeTab === 1 &&
              (newProductsSalesPosts.length === 0 ? (
                <div className='empty-msg'><i class="bi bi-emoji-frown"></i> 판매 중인 새상품이 없어요.</div>
              ) : (
                newProductsSalesPosts.map((post, index) => (
                  <Card
                    key={index}
                    id={post._id}
                    imgSrc={post.img[0]}
                    title={post.title}
                    price={post.price}
                    uploadDate={post.date}
                  />
                ))
            ))}
            {activeTab === 2 &&
              (usedProductsSalesPosts.length === 0 ? (
                <div className='empty-msg'><i class="bi bi-emoji-frown"></i> 판매 중인 중고 상품이 없어요.</div>
              ) : (
                usedProductsSalesPosts.map((post, index) => (
                  <Card
                    key={index}
                    id={post._id}
                    imgSrc={post.img[0]}
                    title={post.title}
                    price={post.price}
                    uploadDate={post.date}
                  />
                ))
            ))}
            {activeTab === 3 && 
              (revAuctionPosts.length === 0 ? (
                <div className='empty-msg'><i class="bi bi-emoji-frown"></i> 역경매가 진행 중인 상품이 없어요.</div>
              ) : (
                revAuctionPosts.map((post, index) => (
                  <StoreRevAuctionCard
                    key={index}
                    id={post._id}
                    imgSrc={post.img}
                    postTitle={post.postTitle}
                    desiredPrice={post.desiredPrice}
                    uploadDate={post.date}
                    progressTime={post.time}
                    isMyStore={isAuthor ? true : false}
                  />
                ))
              ))}
            {activeTab === 4 && 
              (bidProducts.length === 0 ? (
                <div className='empty-msg'><i class="bi bi-emoji-frown"></i> 입찰 진행 중인 상품이 없어요.</div>
              ) : (
                bidProducts.map((post, index) => (
                  <RevAuctionCard
                    key={index}
                    id={post._id}
                    imgSrc={post.img}
                    postTitle={post.postTitle}
                    desiredPrice={post.desiredPrice}
                    uploadDate={post.date}
                    progressTime={post.time}
                  />
                ))
              ))}
            {activeTab === 5 &&
              (interestedProducts.length === 0 ? (
                <div className='empty-msg'><i class="bi bi-emoji-frown"></i>관심상품이 없어요.</div>
              ) : (
                interestedProducts.map((post, index) => (
                  <Card
                    key={index}
                    id={post._id}
                    imgSrc={post.img[0]}
                    title={post.title}
                    price={post.price}
                    uploadDate={post.date}
                  />
                ))
            ))}
            {activeTab === 6 && <div className='empty-msg'><i class="bi bi-emoji-frown"></i> 판매완료된 상품이 없어요.</div>}
          </div>
        </div>
      </div>
    );
  }