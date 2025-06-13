import './MainPage.css';
import ClipLoader from "react-spinners/ClipLoader";
import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import axios from 'axios';
import Card from './../Card/Card.js';
import RevAuctionCard from './../Card/RevAuctionCard.js';

export default function MainPage() {
    const navigate = useNavigate();
    const [revAuctionProducts, setRevAuctionProducts] = useState([]);
    const [newProductPosts, setNewProductPosts] = useState([]);
    const [usedProductPosts, setUsedProductPosts] = useState([]);
    const [popularNewProducts, setPopularNewProducts] = useState([]);
    const [popularUsedProducts, setPopularUsedProducts] = useState([]);
    const [loading, setLoading] = useState(false);
    
    useEffect(() => {
        const fetchPosts = async () => {
            try {
                setLoading(true);
                const response = await axios.get('/posts');
                const revAuctionPostsRes = await axios.get('/revAuctionPosts');
                const data = response.data;
                const revAuctionPosts = revAuctionPostsRes.data;

                const filterRevAuctionPosts = () => {
                    return revAuctionPosts.sort((a, b) => {
                        return new Date(b.date) - new Date(a.date);
                    })
                    .slice(0, 4)
                }

                const processPosts = (type, sortBy) => {
                    return data.filter(post => post.type === type)
                               .sort((a, b) => {
                                   // 0 보다 작다면 a가 앞으로 정렬, 오름차순
                                   // 0 보다 크다면 b가 앞으로 정렬, 내림차순
                                   // 0인 경우는 무시됨
                                   if(sortBy === 'date') {
                                      return new Date(b.date) - new Date(a.date);
                                   } else if (sortBy === 'viewer') {
                                      return b.viewer - a.viewer;
                                   }
                                   return 0;  // 만약 sortBy가 'date'나 'viewer'가 아닌 경우, 순서 변경 없음
                               })
                               .slice(0, 4)
                               .map(post => ({ ...post, id: post._id }));
                }

                // 새상품 필터링
                const filteredNewProducts = processPosts('새상품', 'date');
                const popularNewProducts = processPosts('새상품', 'viewer');

                // 중고품 필터링
                const filteredUsedProducts = processPosts('중고', 'date');
                const popularUsedProducts = processPosts('중고', 'viewer');

                // 상태 업데이트
                setNewProductPosts(filteredNewProducts);
                setPopularNewProducts(popularNewProducts);
                setUsedProductPosts(filteredUsedProducts);
                setPopularUsedProducts(popularUsedProducts);
                setRevAuctionProducts(filterRevAuctionPosts());
            } catch (error) {
                console.error('데이터를 가져오는데 실패했습니다.:', error);
            } finally {
                setLoading(false);
            }
        };

        fetchPosts();
    }, []);

    const moreSeeNewProductPosts = () => {
        navigate('/moreSee/newProductPosts');
    };
    const moreSeeUsedProductPosts = () => {
        navigate('/moreSee/usedProductPosts');
    };
    const moreSeePopularNewProducts = () => {
        navigate('/moreSee/popularNewProducts');
    };
    const moreSeePopularUsedProducts = () => {
        navigate('/moreSee/popularUsedProducts');
    };

    if(loading) {
        return (
           <div className='main-loading-msg'>
              <div className='main-loading-inside'>
                <ClipLoader color="#dcdcdc" loading={loading} size={50} />
                <span>데이터를 불러오고 있어요!</span>
              </div>
           </div>
        ) 
    }

    return (
        <div className="main-container">
            <section className='container-space'>
                <div className='top-title-space'>
                    <div className="title-space">
                        <div className='revAuction-products'>
                            <span>나 이거 살래요!</span>
                        </div>
                        <div className='revAuction-title-img'>
                            <img src='https://easysiteimgs.s3.ap-northeast-2.amazonaws.com/folded-hands_1f64f.png' alt='hands'></img>
                        </div>
                    </div>
                    <div className='more-see'>
                        <span>{`더보기 >`}</span>
                    </div>
                </div>
                <div className={revAuctionProducts.length > 0 ? 'cards' : 'cards-empty'}>
                    {revAuctionProducts.length > 0 ? (
                        revAuctionProducts.map((post, index) => (
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
                    ) : (
                        <div className="cards-empty-message">
                            <span>존재하는 상품이 없어요.</span>
                        </div>
                    )}
                </div>
            </section>
            <section className='container-space'>
                <div className='top-title-space'>
                    <div className="title-space">
                        <div className='new-upload-products'>
                            <span>새로 올라온 새상품!</span>
                        </div>
                        <div>
                            <span className='title-space-keyword'>NEW!!</span>
                        </div>
                    </div>
                    <div className='more-see' onClick={moreSeeNewProductPosts}>
                        <span>{`더보기 >`}</span>
                    </div>
                </div>
                <div className='cards'>
                    {newProductPosts.map((post, index) => (
                        <Card 
                            key={index} 
                            id={post.id}
                            imgSrc={post.img[0]} 
                            title={post.title}
                            price={post.price}
                            uploadDate={post.date}
                        />
                    ))}
                </div>
            </section>
            <section className='container-space'>
                <div className='top-title-space'>
                    <div className="title-space">
                        <div className='new-upload-products'>
                            <span>새로 올라온 중고품!</span>
                        </div>
                        <div>
                            <span className='title-space-keyword'>NEW!!</span>
                        </div>
                    </div>
                    <div className='more-see' onClick={moreSeeUsedProductPosts}>
                        <span>{`더보기 >`}</span>
                    </div>
                </div>
                <div className='cards'>
                    {usedProductPosts.map((post, index) => (
                        <Card 
                            key={index} 
                            id={post.id}
                            imgSrc={post.img[0]} 
                            title={post.title}
                            price={post.price}
                            uploadDate={post.date}
                        />
                    ))}
                </div>
            </section>
            <section className='container-space'>
                <div className='top-title-space'>
                    <div className="title-space">
                        <div>
                            <span>실시간 인기 새상품!</span>
                        </div>
                        <div>
                            <img src='https://easysiteimgs.s3.ap-northeast-2.amazonaws.com/fire.gif' alt='fire'></img>
                        </div>
                        <div>
                            <span className='title-space-keyword'>HOT!!</span>
                        </div>
                    </div>
                    <div className='more-see' onClick={moreSeePopularNewProducts}>
                        <span>{`더보기 >`}</span>
                    </div>
                </div>
                <div className='cards'>
                    {popularNewProducts.map((post, index) => (
                        <Card 
                            key={index} 
                            id={post.id}
                            imgSrc={post.img[0]} 
                            title={post.title}
                            price={post.price}
                            uploadDate={post.date}
                        />
                    ))}
                </div>
            </section>
            <section className='container-space'>
                <div className='top-title-space'>
                    <div className="title-space">
                        <div>
                            <span>실시간 인기 중고상품!</span>
                        </div>
                        <div>
                            <img src='https://easysiteimgs.s3.ap-northeast-2.amazonaws.com/fire.gif' alt='fire'></img>
                        </div>
                        <div>
                            <span className='title-space-keyword'>HOT!!</span>
                        </div>
                    </div>
                    <div className='more-see' onClick={moreSeePopularUsedProducts}>
                        <span>{`더보기 >`}</span>
                    </div>
                </div>
                <div className='cards'>
                    {popularUsedProducts.map((post, index) => (
                        <Card 
                            key={index} 
                            id={post.id}
                            imgSrc={post.img[0]} 
                            title={post.title}
                            price={post.price}
                            uploadDate={post.date}
                        />
                    ))}
                </div>
            </section>
        </div>
    )
}