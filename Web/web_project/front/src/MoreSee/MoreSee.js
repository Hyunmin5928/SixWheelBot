import React, { useState, useEffect } from 'react';
import ClipLoader from "react-spinners/ClipLoader";
import './MoreSee.css';
import axios from 'axios';
import Card from './../Card/Card'

export default function MoreSee({moreSeeTitle, type, sortBy}) {
    const [productPosts, setProductPosts] = useState([]);
    const [loading, setLoading] = useState(false);

    useEffect(() => {
        const fetchPosts = async () => {
            try {
                setLoading(true);
                const response = await axios.get('/posts');
                const data = response.data;
                
                const filteredAndSortedPosts = data.filter(post => post.type === type)
                                                   .sort((a, b) => {
                                                        if(sortBy === 'date') {
                                                            return new Date(b.date) - new Date(a.date);
                                                        } else if (sortBy === 'viewer') {
                                                            return b.viewer - a.viewer;
                                                        }
                                                        return 0;  // 만약 sortBy가 'date'나 'viewer'가 아닌 경우, 순서 변경 없음
                                                    })
                                                    .map(post => ({ ...post, id: post._id }));
                setProductPosts(filteredAndSortedPosts);
            } catch(error) {
                console.error('데이터를 가져오는데 실패했습니다.:', error);
            } finally {
                setLoading(false);
            }
        };
        fetchPosts();
    }, [moreSeeTitle, type, sortBy])

    if (loading) {
        return (
            <div className='moresee-loading-msg'>
               <div className='moresee-loading-inside'>
                 <ClipLoader color="#dcdcdc" loading={loading} size={50} />
                 <span>데이터를 불러오고 있어요!</span>
               </div>
            </div>
        );
    }

    return (
        <div className="moreSee-container">
            <div className='moreSeeTitle'>{moreSeeTitle}</div>
            <div className='more-cards'>
                    {productPosts.map((post, index) => (
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
        </div>
    )
}