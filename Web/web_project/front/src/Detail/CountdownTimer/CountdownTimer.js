import './CountdownTimer.css';
import React, { useState, useEffect } from 'react';
import axios from 'axios';
import { useNavigate } from 'react-router-dom';

export default function CountdownTimer({ id }) {
    const [timeLeft, setTimeLeft] = useState(0);
    const navigate = useNavigate();

    useEffect(() => {
        let intervalId = null;

        async function fetchAuctionDetails() {
            try {
                const response = await axios.get(`/revAuctionDetail/${id}`);
                const startTime = new Date(response.data.date).getTime();
                const durationMinutes = response.data.time; 
                const endTime = new Date(startTime + durationMinutes * 60000).getTime();

                const updateTimer = () => {
                    const now = new Date().getTime();
                    const timeLeft = endTime - now;
                    setTimeLeft(timeLeft > 0 ? timeLeft : 0);

                    if (timeLeft <= 0) {
                        clearInterval(intervalId);
                    }
                };
                updateTimer();
                intervalId = setInterval(updateTimer, 1000);

            } catch (error) {
                console.error('데이터를 불러오는데 실패했습니다.:', error);
            }

            return () => clearInterval(intervalId);
        };

        fetchAuctionDetails();
    }, [id, navigate]);

    const formatTime = (time) => time.toString().padStart(2, '0');

    const hours = Math.floor(timeLeft / (1000 * 60 * 60));
    const minutes = Math.floor((timeLeft / (1000 * 60)) % 60);
    const seconds = Math.floor((timeLeft / 1000) % 60);

    return (
        <div className='remaining-time-container'>
            <div className='remaining-time'>
                남은 시간&nbsp;&nbsp;
                <span className='remaining-time-hour'>{formatTime(hours)}</span> :&nbsp; 
                <span className='remaining-time-minute'>{formatTime(minutes)}</span> :&nbsp; 
                <span className='remaining-time-second'>{formatTime(seconds)}</span>
            </div>
        </div>
    );
}