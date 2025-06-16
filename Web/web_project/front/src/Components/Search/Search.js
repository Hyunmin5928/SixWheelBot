import './SearchResult.css';
import ClipLoader from "react-spinners/ClipLoader";
import Card from '../Card/Card.js';
import axios from "axios";
import React, { useState, useEffect } from 'react';
import { useParams } from 'react-router-dom';

export default function SearchResult() {
    const { searchValue } = useParams();
    const [loading, setLoading] = useState(false);
    const [searchResultsPosts, setSearchResultsPosts] = useState([])
    const [avgPrice, setAvgPrice] = useState(0);
    const [minPrice, setMinPrice] = useState(0);
    const [maxPrice, setMaxPrice] = useState(0);
    const [sortOrder, setSortOrder] = useState('최신순');

    useEffect(() => {
        const fetchData = async () => {
          try {
            setLoading(true);
            const searchRes = await axios.get(`/search?searchValue=${searchValue}`);
            const searchResults = searchRes.data;  

            const prices = searchResults.map(post => parseInt(post.price.replace(/,/g, "")));
            setAvgPrice((prices.reduce((a, b) => a + b, 0) / prices.length || 0).toLocaleString());
            setMinPrice(Math.min(...prices).toLocaleString());
            setMaxPrice(Math.max(...prices).toLocaleString());

            setSearchResultsPosts(searchResults);
          } catch (error) {
              console.error('오류:', error);
          } finally {
            setLoading(false);
          }
        };
  
        fetchData();
    }, [searchValue]);
    
    const sortPosts = (order) => {
        let sortedPosts = [...searchResultsPosts];
        if (order === '최신순') {
            sortedPosts.sort((a, b) => new Date(b.date) - new Date(a.date));
        } else if (order === '오래된 순') {
            sortedPosts.sort((a, b) => new Date(a.date) - new Date(b.date));
        } else if (order === '낮은 가격 순') {
            sortedPosts.sort((a, b) => parseInt(a.price.replace(/,/g, "")) - parseInt(b.price.replace(/,/g, "")));
        } else if (order === '높은 가격 순') {
            sortedPosts.sort((a, b) => parseInt(b.price.replace(/,/g, "")) - parseInt(a.price.replace(/,/g, "")));
        }
        setSearchResultsPosts(sortedPosts);
        setSortOrder(order);
    }

    if(loading) {
        return (
           <div className='searchResult-loading-msg'>
              <div className='searchResult-loading-inside'>
                <ClipLoader color="#dcdcdc" loading={loading} size={50} />
                <span>데이터를 불러오고 있어요!</span>
              </div>
           </div>
        ) 
    }

    return (
        <div className='searchResult-container'>
            <div className='search-result-msg'>
                <div className='search-result-keyword'>
                    <strong className='strong-value'>'{searchValue}'</strong> 검색 결과
                </div>
                <div className='search-result-count'> 총 {searchResultsPosts.length}개의 검색 결과가 있어요!</div>
            </div>
            <div className='searchResult-price-info'>
                <div className='avg-price'>
                    <div className='avg-price-title'>
                        평균가격
                    </div>
                    <div className='avg-price-value'>
                        {avgPrice} 원
                    </div>
                </div>
                <div className='low-price'>
                    <div className='low-price-title'>
                        최저가
                    </div>
                    <div className='low-price-value'>
                        {minPrice} 원
                    </div>
                </div>
                <div className='high-price'>
                    <div className='high-price-title'>
                        최고가
                    </div>
                    <div className='high-price-value'>
                        {maxPrice} 원
                    </div>
                </div>
            </div>
            <ul className='sort-types'>
                <li className={sortOrder === '최신순' ? 'active' : ''} onClick={() => sortPosts('최신순')}>최신순</li>
                <li className={sortOrder === '오래된 순' ? 'active' : ''} onClick={() => sortPosts('오래된 순')}>오래된 순</li>
                <li className={sortOrder === '낮은 가격 순' ? 'active' : ''} onClick={() => sortPosts('낮은 가격 순')}>낮은 가격 순</li>
                <li className={sortOrder === '높은 가격 순' ? 'active' : ''} onClick={() => sortPosts('높은 가격 순')}>높은 가격 순</li>
            </ul>
            <div className='search-result-cards'>
                    {searchResultsPosts.map((post, index) => (
                        <Card 
                            key={index} 
                            id={post._id}
                            imgSrc={post.img[0]} 
                            title={post.title}
                            price={post.price}
                            uploadDate={post.date}
                        />
                    ))}
            </div>
        </div>
    )
}