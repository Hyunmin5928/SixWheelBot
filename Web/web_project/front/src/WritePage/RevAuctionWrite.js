import './RevAuctionWrite.css';
import { readFileAsync } from './readFileAsync.js'
import axios from 'axios';
import { useNavigate } from "react-router-dom";
import { useState } from "react";

export default function RevAuctionWrite() {
    const maxPrice = 100000000;
    const [postTitle, setPostTitle] = useState('');
    const [prevPrice, setPrevPrice] = useState('');
    const [desiredPrice, setDesiredPrice] = useState('');
    const [postContent, setPostContent] = useState('');
    const [time, setTime] = useState('');
    const [previewImgFile, setPreviewImgFile] = useState('');
    const [postImgFile, setPostImgFile] = useState('');
    const [checkImg, setCheckImg] = useState(false);
    const navigate = useNavigate();

    const formatNumber = (num) => {
        return num.toLocaleString(); //천 단위로 콤마 넣기 
    };
    
    const handlePriceChange = (e) => {
        // 쉼표를 포함하여 숫자가 아닌 모든 문자 제거
        const numericValue = e.target.value.replace(/,/g, '').replace(/\D/g, '');
        
        const cleanedNumber = numericValue ? parseInt(numericValue, 10) : "";
    
        if(cleanedNumber <= maxPrice) {
          const formattedNumber = formatNumber(cleanedNumber);
          setDesiredPrice(formattedNumber);
          setPrevPrice(formattedNumber);
        } else {
          setDesiredPrice(prevPrice);
        }
      };
    
    const handleTitleChange = (e) => {
        if(e.target.value.length > 10) {
            return;
        } else {
            setPostTitle(e.target.value);
        }
    }

    const uploadImgFile = async (e) => {
        if (e.target.files.length > 1) {
            alert("이미지는 1장만 선택 가능합니다.");
            return;
        }
        
        setPostImgFile(e.target.files[0]);

        try {
            const fileDataUrl = await readFileAsync(e.target.files[0]); // 파일을 데이터 URL로 변환합니다.
            setPreviewImgFile(fileDataUrl); // 미리보기 URL을 상태로 설정합니다.
            setCheckImg(true)
        } catch (error) {
            console.error('파일을 읽는 중 오류가 발생했습니다.:', error);
        }
    };

    const handlePostSubmit = async () => {
        if (!postTitle || !desiredPrice || !postContent || !time || !postImgFile) {
            alert("빈칸을 모두 채우고 이미지를 선택하세요.");
            return;
        } else {
            const formData = new FormData();
            formData.append('postTitle', postTitle);
            formData.append('desiredPrice', desiredPrice);
            formData.append('postContent', postContent);
            formData.append('time', time);
            formData.append('postImgFile', postImgFile);

            try {
                await axios.post('/revAuctionAdd', formData, {
                  headers: {
                    'Content-Type': 'multipart/form-data'
                  }
                });
                navigate('/');
              } catch (error) {
                alert(`Error: ${error.response ? error.response.data : error.message}`);
            }
        }
    }

    const removeImage = () => {
        setPreviewImgFile('');
        setPostImgFile('');
        setCheckImg(false);
    };

    return (
        <div className='rev-auction-container'>
            <div className='rev-auction-screen'>
                <div className='rev-auction-title'>
                    <h1>{`구매 희망(역경매) 등록`}</h1>
                </div>
                <div className='rev-auction-input-space'>
                        {!checkImg && (
                            <div className='rev-auction-input-image'>
                                <label htmlFor="choose-img-file">
                                    <div className="img-file-selector">
                                        <i class="bi bi-camera-fill"></i>
                                    </div>
                                </label>
                                <input
                                    id="choose-img-file"
                                    type="file"
                                    accept="image/png, image/jpeg, image/jpg"
                                    onChange={uploadImgFile}
                                    required
                                />
                            </div>
                        )}
                        {checkImg && (
                            <div className='rev-auction-preview-img'>
                                <img src={previewImgFile} alt=""></img>
                                <i onClick={removeImage} class="bi bi-x-circle-fill"></i>
                            </div> 
                            )
                        }        
                    <div className='rev-auction-input-text'>
                        <div className="rev-auction-product-name">
                            <input
                                type="text"
                                placeholder="상품명"
                                value={postTitle}
                                onChange={handleTitleChange}
                            />
                        </div>
                        <div className='rev-auction-price-and-time'>
                            <div className='rev-auction-product-price'>
                                <input
                                    type="text"
                                    placeholder="상품가격(원)"
                                    inputMode="numeric"
                                    value={desiredPrice}
                                    onChange={handlePriceChange}
                                />
                            </div>
                            <div className='rev-auction-product-time'>
                                <select
                                    value={time}
                                    onChange={(e) => setTime(e.target.value)}
                                    required
                                >
                                    <option value="">{`[시간을 선택하세요]`}</option>
                                    <option value="15">15분</option>
                                    <option value="30">30분</option>
                                    <option value="45">45분</option>
                                    <option value="60">1시간</option>
                                    <option value="120">2시간</option>
                                    <option value="180">3시간</option>
                                    <option value="240">4시간</option>
                                </select>
                            </div>
                        </div>
                        <div className='rev-auction-product-content'>
                            <textarea
                                maxLength="250"
                                placeholder="상품설명"
                                value={postContent}
                                onChange={(e) => setPostContent(e.target.value)}
                            />
                        </div>
                        <div className="submit_btn">
                            <button onClick={handlePostSubmit} type="submit">등록하기</button>
                        </div>
                    </div>
                </div>
                
           </div>
        </div>
    )
}