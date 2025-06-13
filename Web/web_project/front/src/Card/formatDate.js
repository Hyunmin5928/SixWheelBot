import { formatDistanceToNow } from "date-fns";
import {ko} from "date-fns/locale";

export const formatDate = (date) => {
    const postDate = new Date(date); // 서버로부터 받은 날짜 문자열을 Date 객체로 변환
    let relativeTime = formatDistanceToNow(postDate, { addSuffix: true, locale: ko, includeSeconds: true });
    relativeTime = relativeTime.replace('약 ', '');
    return relativeTime;
}