export const reducer = (state, action) => {
    switch (action.type) {
      case 'ADD_IMAGES':
        return {
          ...state,
          imgFile: [...state.imgFile, ...action.payload.imgFile],
          postImg: [...state.postImg, ...action.payload.postImg],
          fileCount: state.fileCount + action.payload.newFilesCount
        };
      case 'ADD_EDIT_IMAGES':
        return {
          ...state,
          prewImg: [...state.prewImg, ...action.payload.prewImg],
          postImg: [...state.postImg, ...action.payload.postImg],
          fileCount: state.fileCount + action.payload.newFilesCount
        };
      case 'DELETE_IMAGE':
        const newPostImg = [...state.postImg];
        newPostImg.splice(action.payload, 1);
        const newPrevImg = [...state.imgFile];
        newPrevImg.splice(action.payload, 1);
        return {
          ...state,
          postImg: newPostImg,
          imgFile : newPrevImg,
          fileCount: state.fileCount - 1
        };
      case 'DELETE_EDIT_IMAGE':
          const newEditPrevImgFile = [...state.prewImg];
          newEditPrevImgFile.splice(action.payload, 1);
          const newEditPostImg = [...state.postImg];
          newEditPostImg.splice(action.payload, 1);
          const newEditExistingImg = [...state.imgFile];
          newEditExistingImg.splice(action.payload, 1);
          return {
            ...state,
            prewImg: newEditPrevImgFile,
            postImg: newEditPostImg,
            imgFile : newEditExistingImg,
            fileCount: state.fileCount - 1
          };
      case 'UPDATE_FIELD':
        if (action.payload.field === 'productName' && action.payload.value.length > 30) {
            return state; 
        }
        
        return {
            ...state,
            [action.payload.field]: action.payload.value
        };
      case 'SET_EDIT_DATA':
        return {
          ...state,
          ...action.payload
        };
      default:
        return state;
    }
  }

