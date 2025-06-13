import './RecProducts.css';
import Card from './../Card/Card.js';
import axios from 'axios';
import React, { useState, useEffect } from 'react';

function shuffleArray(array) {
    for (let i = array.length - 1; i > 0; i--) {
        const j = Math.floor(Math.random() * (i + 1));
        [array[i], array[j]] = [array[j], array[i]]; // Swap elements
    }
    return array;
}

export default function RecProducts({ userId, loggedInUsername, postCategory, postId }) {
    const [recPosts, setRecPosts] = useState([]);

    useEffect(() => {
        const fetchData = async () => {
            try {
                const recPostsResponse = await axios.get('/recPosts', { params: { postCategory } });
                const filteredRecPosts = recPostsResponse.data.filter(post => post._id !== postId && post.authorId !== userId);
                const shuffledPosts = shuffleArray(filteredRecPosts).slice(0, 4);
                setRecPosts(shuffledPosts);
            } catch (error) {
                console.error('데이터를 가져오는데 실패했습니다.:', error);
            }
        }

        fetchData();
    }, [postCategory, postId, userId]);

    if (recPosts.length === 0) {
        return null;
    }

    return (
        <div className='rec-products-container'>
            <div className='rec-info-msg'>
              <span>{ loggedInUsername } 님! 이런 상품은 어떠세요?</span>
            </div>
            <div className="rec-products">
                {recPosts.map((post, index) => (
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

