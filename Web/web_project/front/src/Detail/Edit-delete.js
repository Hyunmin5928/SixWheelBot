import './Edit-delete.css';
import { useNavigate } from 'react-router-dom';
import axios from 'axios';
export default function EditDelete({ id }) {
    const navigate = useNavigate();
    const doEdit = () => {
        navigate(`/salesPostsEdit/${id}`);
    };
    const deleteSalesPost = async () => {
        if (window.confirm('정말로 삭제하시겠습니까?')) {
            try {
                const response = await axios.delete(`/salesPost/${id}`);
                alert(response.data);
                navigate('/');
            } catch (error) {
                alert('삭제 실패: ' + error.message);
            }
        }
    }
    return (
        <div className='edit-delete'>
                <div className='edit' onClick={doEdit}>
                    <button className='edit-btn'><i class="bi bi-pencil"></i>&nbsp;&nbsp;수정하기</button>
                </div>
                <div className='delete' onClick={deleteSalesPost}>
                    <button className='delete-btn'><i class="bi bi-trash"></i>&nbsp;&nbsp;삭제하기</button>
                </div>
        </div>
    )
}