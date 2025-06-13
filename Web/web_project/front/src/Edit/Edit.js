import './Edit.css';
import { useParams, useNavigate } from 'react-router-dom';
import React, { useEffect, useState, useReducer, useCallback } from 'react';
import axios from 'axios';
import { readFileAsync } from './../WritePage/readFileAsync.js';
import { reducer } from './../WritePage/reducer.js';

const initialState = {
    fileCount: 0,
    imgFile: [],
    postImg: [], //서버로 새로운 이미지 전달할 거
    prewImg: [],
    productName: "",
    productContent: "",
    productType: "중고",
    selectedCategory: ""
}

export default function Edit() {
    const { id } = useParams();
    const navigate = useNavigate();
    const maxProductPrice = 100000000;
    const [prevProductPrice, setPrevProductPrice] = useState("");
    const [productPrice, setProductPrice] = useState("");
    const [state, dispatch] = useReducer(reducer, initialState);

    const formatNumber = useCallback((num) => num.toLocaleString(), []);
    
    const handlePriceChange = useCallback((event) => {
        const numericValue = event.target.value.replace(/,/g, '').replace(/\D/g, '');
        const cleanedNumber = numericValue ? parseInt(numericValue, 10) : "";
        if(cleanedNumber <= maxProductPrice) {
            const formattedNumber = formatNumber(cleanedNumber);
            setProductPrice(formattedNumber);
            setPrevProductPrice(formattedNumber);
        } else {
            setProductPrice(prevProductPrice);
        }
    }, [formatNumber, maxProductPrice, prevProductPrice]);

    useEffect(() => {
        const fetchData = async () => {
            try {
                const response = await axios.get(`/salesPostsEdit/${id}`);
                dispatch({
                    type: 'SET_EDIT_DATA',
                    payload: {
                        productName: response.data.title,
                        productContent: response.data.content,
                        productType: response.data.type,
                        selectedCategory: response.data.category,
                        fileCount: response.data.img?.length || 0,
                        prewImg: response.data.img || [], // 이미지를 추가하면서 보여줄 거
                        imgFile: response.data.img || []  // 기존 이미지
                    }
                });
                setProductPrice(response.data.price);
            } catch (err) {
                console.error('데이터를 가져오는데 실패했습니다.', err);
            }
        };

        fetchData();
    }, [id]);

    const uploadImgFile = async (e) => {
        const files = Array.from(e.target.files);
        if (state.prewImg.length + files.length > 10) {
          alert("최대 10개까지만 업로드 가능합니다.");
          return;
        }
      
        const fileReadPromises = files.map(file => readFileAsync(file));
      
        try {
          const dataURLs = await Promise.all(fileReadPromises);
          dispatch({
            type: 'ADD_EDIT_IMAGES',
            payload: {
                prewImg: dataURLs,
                postImg: files,
                newFilesCount: files.length
            }
          })
        } catch(error) {
          console.error("파일 읽기에 실패했습니다.:", error);
        }
      
        e.target.value = null; //
    };

    const deleteImage = useCallback((index) => {
        dispatch({ type: 'DELETE_EDIT_IMAGE', payload: index });
    }, []);

    const handlePostSubmit = async () => {
        if (!state.productName || !productPrice || !state.productContent || !state.productType || !state.selectedCategory || state.prewImg.length === 0) {
          alert("빈칸을 모두 채우고 이미지를 하나 이상 선택하세요.");
          return;
        }
    
        const formData = new FormData();
        state.postImg.forEach(img => formData.append('postImg', img));
        if (state.imgFile.length > 0) {
            formData.append('existingImg', state.imgFile.join(',')); 
        }
        formData.append('productName', state.productName);
        formData.append('productPrice', productPrice);
        formData.append('productContent', state.productContent);
        formData.append('productType', state.productType);
        formData.append('selectedCategory', state.selectedCategory);
      
        try {
          await axios.post(`/salesPostsEdit/${id}`, formData, {
            headers: {
              'Content-Type': 'multipart/form-data'
            }
          });
          navigate('/');
        } catch (error) {
          alert(`에러남: ${error.response ? error.response.data : error.message}`);
        }
    };
    
    const handleChange = useCallback((field, value) => {
        dispatch({ type: 'UPDATE_FIELD', payload: { field, value } });
    }, []);

    return (
        <div className="container">
            <div className="screen">
                <div className="upload_image">
                    <label htmlFor="chooseFile">
                        <div className="real_fileSelector">
                        <i class="bi bi-camera-fill"></i>
                        <p>{state.fileCount}/10</p>
                        </div>
                    </label>
                    <input
                        id="chooseFile"
                        type="file"
                        accept="image/png, image/jpeg, image/jpg"
                        onChange={uploadImgFile}
                        multiple
                        required
                    />
                    <div className="uploaded_image">
                        {state.prewImg.map((data, index) => {
                        return (
                            <div className="uploaded_image_value" key={index}>
                            <img src={data} alt=""></img>
                            <i
                                onClick={() => deleteImage(index)}
                                class="bi bi-x-circle-fill"
                            ></i>
                            </div>
                        );
                        })}
                    </div>
                </div>
                <div className="product_name">
                    <input
                        type="text"
                        placeholder="상품명"
                        value={state.productName}
                        onChange={(e) => handleChange('productName', e.target.value)}
                    />
                </div>
                <div className="price_category">
                    <div>
                        <input
                        type="text"
                        placeholder="상품가격(원)"
                        inputMode="numeric"
                        value={productPrice}
                        onChange={handlePriceChange}
                        />
                    </div>
                    <div>
                        <select
                        value={state.selectedCategory}
                        onChange={(e) => handleChange('selectedCategory', e.target.value)}
                        required
                        >
                            <option value="">[카테고리 설정]</option>
                            <option value="디지털기기">디지털기기</option>
                            <option value="생활가전">생활가전</option>
                            <option value="가구/인테리어">가구/인테리어</option>
                            <option value="생활/주방">생활/주방</option>
                            <option value="유아용품">유아용품</option>
                            <option value="여성의류">여성의류</option>
                            <option value="여성잡화">여성잡화</option>
                            <option value="남성의류">남성의류</option>
                            <option value="남성잡화">남성잡화</option>
                            <option value="뷰티/미용">뷰티/미용</option>
                            <option value="스포츠/레저">스포츠/레저</option>
                            <option value="도서">도서</option>
                            <option value="티켓/교환권">티켓/교환권</option>
                            <option value="반려동물용품">반려동물용품</option>
                            <option value="가공식품">가공식품</option>
                            <option value="식물">식물</option>
                            <option value="취미/게임/음반">취미/게임/음반</option>
                            <option value="기타물품">기타물품</option>
                        </select>
                    </div>
                    <div className="used_product">
                        <button
                        type="button"
                        onClick={() => handleChange('productType', '중고')}
                        className={state.productType === "중고" ? "selected" : ""}
                        >
                        중고
                        </button>
                    </div>
                    <div className="unused_product">
                        <button
                        type="button"
                        onClick={() => handleChange('productType', '새상품')}
                        className={state.productType === "새상품" ? "selected" : ""}
                        >
                        새상품
                        </button>
                    </div>
                </div>
                <div className="textarea">
                    <textarea
                        maxLength="1000"
                        placeholder="상품설명"
                        value={state.productContent}
                        onChange={(e) => handleChange('productContent', e.target.value)}
                    />
                    </div>
                <div className="submit_btn">
                    <button type="submit" onClick={handlePostSubmit}>수정하기</button>
                </div>
            </div>
        </div>
    )
}