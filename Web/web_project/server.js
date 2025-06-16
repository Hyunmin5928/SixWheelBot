// SQLite3 기반 서버 (MongoDB 완전 제거 후 전환)
const express = require("express");
const requestIp = require('request-ip');
const cors = require("cors");
const path = require("path");
const crypto = require('crypto');
const bcrypt = require("bcrypt");
const moment = require('moment-timezone');
const sqlite3 = require('sqlite3').verbose();
const { open } = require('sqlite');
const session = require("express-session");
const passport = require("passport");
const LocalStrategy = require("passport-local");
const nodemailer = require('nodemailer');
const { createServer } = require('http');
const { Server } = require('socket.io');
const multer = require('multer');
const { S3Client, DeleteObjectCommand } = require('@aws-sdk/client-s3');
const multerS3 = require('multer-s3');
const schedule = require('node-schedule');

// DB 연결 및 테이블 초기화
let db;
(async () => {
  db = await open({ filename: './data.sqlite', driver: sqlite3.Database });
  await db.exec(`CREATE TABLE IF NOT EXISTS user (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE,
    password TEXT,
    email TEXT
  );`);
  await db.exec(`CREATE TABLE IF NOT EXISTS salesPost (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT,
    price TEXT,
    content TEXT,
    type TEXT,
    category TEXT,
    img TEXT,
    date TEXT,
    viewer INTEGER,
    authorId INTEGER,
    publisherName TEXT
  );`);
  await db.exec(`CREATE TABLE IF NOT EXISTS interestedProducts (
    userId INTEGER,
    postId INTEGER,
    PRIMARY KEY (userId, postId)
  );`);
  await db.exec(`CREATE TABLE IF NOT EXISTS bidProducts (
    userId INTEGER,
    postId INTEGER,
    bidAmount INTEGER,
    PRIMARY KEY (userId, postId)
  );`);
  await db.exec(`CREATE TABLE IF NOT EXISTS revAuctionPost (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    postTitle TEXT,
    desiredPrice TEXT,
    postContent TEXT,
    time INTEGER,
    img TEXT,
    date TEXT,
    endTime TEXT,
    viewer INTEGER,
    authorId INTEGER,
    publisherName TEXT,
    status TEXT,
    awardedTo TEXT,
    finalPrice TEXT
  );`);
  await db.exec(`CREATE TABLE IF NOT EXISTS revAuctionBids (
    postId INTEGER,
    bidderId INTEGER,
    bidPrice TEXT,
    PRIMARY KEY (postId, bidderId)
  );`);
  console.log("SQLite DB 연결 성공");
})();

const app = express();
app.use(cors());
app.use(express.json());
app.use(express.urlencoded({ extended: true }));
app.use(requestIp.mw());

// 세션 및 Passport 설정
app.use(session({ secret: process.env.SESSION_SECRET || 'secret', resave: false, saveUninitialized: false }));
app.use(passport.initialize());
app.use(passport.session());

passport.use(new LocalStrategy(async (username, password, done) => {
  try {
    const user = await db.get("SELECT * FROM user WHERE username = ?", [username]);
    if (!user) return done(null, false, { message: '존재하지 않는 아이디입니다.' });
    if (await bcrypt.compare(password, user.password)) return done(null, user);
    return done(null, false, { message: '비밀번호가 일치하지 않습니다.' });
  } catch (err) {
    return done(err);
  }
}));
passport.serializeUser((user, done) => done(null, { id: user.id, username: user.username }));
passport.deserializeUser(async (user, done) => {
  try {
    const row = await db.get("SELECT * FROM user WHERE id = ?", [user.id]);
    if (row) delete row.password;
    done(null, row);
  } catch (err) {
    done(err);
  }
});

// 파일 업로드 (S3 유지)
// const s3 = new S3Client({ region: 'ap-northeast-2', credentials: { accessKeyId: process.env.S3_ACCESS_KEY, secretAccessKey: process.env.S3_SECRET_ACCCESS_KEY } });
// const upload = multer({ storage: multerS3({ s3, bucket: process.env.S3_BUCKET, key: (req, file, cb) => {
//   const unique = Date.now().toString() + '-' + crypto.randomBytes(8).toString('hex');
//   cb(null, unique + path.extname(file.originalname));
// }}) });

// server.js 에서 S3 관련 코드를 모두 지우고, 아래로 대체
const uploadDir = path.resolve(__dirname, 'uploads');

// uploads/ 디렉토리 자동 생성 (없으면)
const fs = require('fs');
if (!fs.existsSync(uploadDir)) fs.mkdirSync(uploadDir);

const storage = multer.diskStorage({
  destination: (req, file, cb) => cb(null, uploadDir),
  filename:    (req, file, cb) => {
    const unique = Date.now() + '-' + crypto.randomBytes(8).toString('hex');
    cb(null, unique + path.extname(file.originalname));
  }
});

const upload = multer({ storage });

// 정적파일 서빙 추가
app.use('/uploads', express.static(uploadDir));

// 메일 발송
const transporter = nodemailer.createTransport({ service: 'gmail', auth: { user: process.env.EMAIL_USERNAME, pass: process.env.EMAIL_PASSWORD } });
function generateRandomCode(n) { let s = ''; for (let i = 0; i < n; i++) s += Math.floor(Math.random() * 10); return s; }

// 서버 및 WebSocket 시작
const server = createServer(app);
const io = new Server(server, { cors: { origin: "http://localhost:3000", methods: ["GET","POST"] } });
server.listen(process.env.PORT||4000, () => console.log("서버 실행 중"));

//--------------------------------- API 구현 ----------------------------------
// server.js 의 API 구현 직전쯤
// app.get('/', (req, res) => {
//   res.send('서버 정상 작동 중');
// });

// 회원가입
app.post("/register", async (req, res) => {
  const { username, password } = req.body;
  try {
    const exists = await db.get("SELECT * FROM user WHERE username = ?", [username]);
    if (exists) return res.status(400).send("이미 존재하는 아이디입니다.");
    const hash = await bcrypt.hash(password, 10);
    await db.run("INSERT INTO user (username,password) VALUES(?,?)", [username, hash]);
    res.json({ registerSuccess: true });
  } catch (e) { console.error(e); res.status(500).send("회원가입 오류"); }
});

// 로그인
app.post("/login", (req, res, next) => {
  passport.authenticate("local", (err, user, info) => {
    if (err) return res.status(500).json(err);
    if (!user) return res.status(401).json(info);
    req.logIn(user, (e) => { if (e) return next(e); res.json({ loginSuccess: true }); });
  })(req, res, next);
});

// 로그아웃
app.get("/logout", (req, res) => { req.logout(() => res.json({ logoutSuccess: true })); });

// 로그인 상태 확인
app.get("/checkLoggedIn", (req, res) => { res.json({ isLoggedIn: !!req.user }); });

// 로그인 정보 제공
app.get("/checkLogin", (req, res) => {
  if (req.user) res.json({ isLoggedIn: true, id: req.user.id, username: req.user.username });
  else res.json({ isLoggedIn: false });
});

// 판매글 작성
app.post("/add", upload.array('postImg',10), async (req,res) => {
  const { productName, productPrice, productContent, productType, selectedCategory } = req.body;
  try {
    const imgs = req.files.map(f=>f.location).join(',');
    const date = moment().tz("Asia/Seoul").format("YYYY-MM-DD HH:mm:ss");
    const authorId = req.user.id;
    const user = await db.get("SELECT username FROM user WHERE id = ?", [authorId]);
    await db.run(
      `INSERT INTO salesPost (title,price,content,type,category,img,date,viewer,authorId,publisherName)
       VALUES(?,?,?,?,?,?,?,?,?,?)`,
      [productName,productPrice,productContent,productType,selectedCategory,imgs,date,0,authorId,user.username]
    );
    res.json({ postSuccess: true });
  } catch (e) { console.error(e); res.status(500).send("업로드 오류"); }
});

// 전체 판매글 조회
app.get("/posts", async (req,res) => {
  try {
    const posts = await db.all("SELECT * FROM salesPost ORDER BY date DESC");
    res.json(posts);
  } catch (e) { console.error(e); res.status(500).send("조회 실패"); }
});

// 판매글 상세 조회 (조회수 증가)
app.get("/detail/:id", async (req,res) => {
  try {
    const { id } = req.params;
    const post = await db.get("SELECT * FROM salesPost WHERE id = ?", [id]);
    if (!post) return res.status(404).send("게시글을 찾을 수 없습니다.");
    await db.run("UPDATE salesPost SET viewer = viewer + 1 WHERE id = ?", [id]);
    res.json(post);
  } catch (e) { console.error(e); res.status(500).send("서버 에러"); }
});

// 판매글 수정
app.get("/salesPostsEdit/:id", async (req,res) => {
  try {
    const post = await db.get("SELECT * FROM salesPost WHERE id = ?", [req.params.id]);
    if (post) res.json(post);
    else res.status(404).send("게시글을 찾을 수 없습니다.");
  } catch (e) { console.error(e); res.status(500).send("서버 에러"); }
});
app.post("/salesPostsEdit/:id", upload.array('postImg',10), async (req,res) => {
  try {
    const { productName,productPrice,productContent,productType,selectedCategory,existingImg } = req.body;
    const newImgs = req.files.map(f=>f.location);
    const imgs = [...(existingImg?existingImg.split(','):[]),...newImgs].join(',');
    await db.run(
      "UPDATE salesPost SET title=?,price=?,content=?,type=?,category=?,img=? WHERE id=?",
      [productName,productPrice,productContent,productType,selectedCategory,imgs,req.params.id]
    );
    res.json({ editSuccess: true });
  } catch (e) { console.error(e); res.status(500).send("서버 에러"); }
});

// 판매글 삭제
app.delete("/salesPost/:id", async (req,res) => {
  try {
    const post = await db.get("SELECT * FROM salesPost WHERE id = ?", [req.params.id]);
    if (!post) return res.status(404).send("게시글을 찾을 수 없습니다.");
    await db.run("DELETE FROM interestedProducts WHERE postId = ?", [req.params.id]);
    await db.run("DELETE FROM bidProducts WHERE postId = ?", [req.params.id]);
    const imgs = post.img?post.img.split(','):[]; 
    for (let url of imgs) {
      const key = url.split('/').pop();
      await s3.send(new DeleteObjectCommand({ Bucket: process.env.S3_BUCKET, Key: key }));
    }
    await db.run("DELETE FROM salesPost WHERE id = ?", [req.params.id]);
    res.json({ deleteSuccess: true });
  } catch (e) { console.error(e); res.status(500).send("서버 에러"); }
});

// 마이페이지: 작성자 ID 조회
app.get("/userAuthorID", async (req,res) => {
  try {
    const user = await db.get("SELECT id FROM user WHERE username = ?", [req.query.username]);
    if (user) res.send(user.id.toString());
    else res.status(404).send("사용자를 찾을 수 없습니다.");
  } catch (e) { console.error(e); res.status(500).send("서버 에러"); }
});

// 마이페이지: 내 판매글 목록
app.get("/userSalesPosts", async (req,res) => {
  try {
    const posts = await db.all("SELECT * FROM salesPost WHERE authorId = ? ORDER BY date DESC", [req.query.myObjectID]);
    res.json(posts);
  } catch (e) { console.error(e); res.status(500).send("서버 에러"); }
});

// 추천 상품 조회
app.get("/recPosts", async (req,res) => {
  try {
    const posts = await db.all("SELECT * FROM salesPost WHERE category = ? ORDER BY date DESC", [req.query.postCategory]);
    res.json(posts);
  } catch (e) { console.error(e); res.status(500).send("서버 에러"); }
});

// 관심 상품
app.post("/interestedProducts", async (req, res) => {
  const { userObjId, postId } = req.body;
  try {
    await db.run("INSERT OR IGNORE INTO interestedProducts (userId, postId) VALUES (?, ?)", [userObjId, postId]);
    res.json({ success: true });
  } catch (e) { console.error(e); res.status(500).send("서버 에러"); }
});
app.delete("/interestedProducts", async (req,res) => {
  const { userObjId, postId } = req.query;
  try {
    await db.run("DELETE FROM interestedProducts WHERE userId = ? AND postId = ?", [userObjId, postId]);
    res.json({ success: true });
  } catch (e) { console.error(e); res.status(500).send("서버 에러"); }
});
app.get("/userInterestedProducts", async (req,res) => {
  if (!req.user) return res.json([]);
  const results = await db.all(
    `SELECT sp.* FROM salesPost sp
     JOIN interestedProducts ip ON sp.id = ip.postId
     WHERE ip.userId = ? ORDER BY sp.date DESC`,
    [req.user.id]
  );
  res.json(results);
});

// 검색
app.get("/search", async (req,res) => {
  const like = `%${req.query.searchValue}%`;
  try {
    const rows = await db.all("SELECT * FROM salesPost WHERE title LIKE ? ORDER BY date DESC", [like]);
    res.json(rows);
  } catch (e) { console.error(e); res.status(500).send("서버 에러"); }
});

// 역경매 게시물 등록
app.post("/revAuctionAdd", upload.single('postImgFile'), async (req,res) => {
  const { postTitle,desiredPrice,postContent,time } = req.body;
  try {
    const img = req.file.location;
    const date = moment().tz("Asia/Seoul").format("YYYY-MM-DD HH:mm:ss");
    const endTime = moment(date).add(parseInt(time), 'minutes').format("YYYY-MM-DD HH:mm:ss");
    const authorId = req.user.id;
    const user = await db.get("SELECT username FROM user WHERE id = ?", [authorId]);
    await db.run(
      `INSERT INTO revAuctionPost (postTitle,desiredPrice,postContent,time,img,date,endTime,viewer,authorId,publisherName,status)
       VALUES (?,?,?,?,?,?,?,?,?,?,?)`,
      [postTitle,desiredPrice,postContent,parseInt(time),img,date,endTime,0,authorId,user.username,'active']
    );
    schedule.scheduleJob(endTime, async () => {
      await db.run("UPDATE revAuctionPost SET status='close' WHERE endTime = ?", [endTime]);
    });
    res.json({ success: true });
  } catch (e) { console.error(e); res.status(500).send("서버 에러"); }
});

// 역경매 게시물 삭제
app.delete("/revAuctionPost/:id", async (req,res) => {
  const { id } = req.params;
  try {
    const post = await db.get("SELECT * FROM revAuctionPost WHERE id = ?", [id]);
    const key = post.img.split('/').pop();
    await s3.send(new DeleteObjectCommand({ Bucket: process.env.S3_BUCKET, Key: key }));
    await db.run("DELETE FROM revAuctionBids WHERE postId = ?", [id]);
    await db.run("DELETE FROM revAuctionPost WHERE id = ?", [id]);
    res.json({ success: true });
  } catch (e) { console.error(e); res.status(500).send("서버 에러"); }
});

// 역경매 게시물 조회
app.get("/revAuctionPosts", async (req,res) => {
  try {
    const rows = await db.all("SELECT * FROM revAuctionPost ORDER BY date DESC");
    res.json(rows);
  } catch (e) { console.error(e); res.status(500).send("서버 에러"); }
});
app.get("/revAuctionDetail/:id", async (req,res) => {
  const { id } = req.params;
  try {
    const post = await db.get("SELECT * FROM revAuctionPost WHERE id = ?", [id]);
    if (!post) return res.status(404).send("게시물을 찾을 수 없습니다.");
    await db.run("UPDATE revAuctionPost SET viewer = viewer + 1 WHERE id = ?", [id]);
    res.json(post);
  } catch (e) { console.error(e); res.status(500).send("서버 에러"); }
});

// 역경매 입찰하기
app.post("/revAuction/:id/bids", async (req,res) => {
  const { bid_price } = req.body;
  const postId = req.params.id;
  const bidderId = req.user.id;
  try {
    await db.run(
      "INSERT OR REPLACE INTO revAuctionBids (postId,bidderId,bidPrice) VALUES(?,?,?)",
      [postId, bidderId, bid_price]
    );
    res.json({ success: true });
  } catch (e) { console.error(e); res.status(500).send("서버 에러"); }
});

// 중복 입찰 방지
app.get("/checkDuplicateBid", async (req,res) => {
  const { revAuctionPostId } = req.query;
  const bidderId = req.user.id;
  try {
    const bid = await db.get(
      "SELECT * FROM revAuctionBids WHERE postId = ? AND bidderId = ?",
      [revAuctionPostId, bidderId]
    );
    res.json({ isDuplicate: !!bid });
  } catch (e) { console.error(e); res.status(500).send("서버 에러"); }
});


//------------------------------------------------------------------------------------------------------------------------------------//

//----------------------------------------------------------낙찰----------------------------------------------------------------------//

// 낙찰 처리
app.post('/revAuction/:id/award', async (req, res) => {
  const postId = req.params.id;
  const bidderId = req.user.id;
  const { bid_price } = req.body;
  try {
    await db.run(
      "UPDATE revAuctionPost SET status = 'awarded', awardedTo = ?, finalPrice = ? WHERE id = ?",
      [bidderId, bid_price, postId]
    );
    res.json({ success: true });
  } catch (e) {
    console.error('낙찰 처리 중 에러:', e);
    res.status(500).send('서버 에러');
  }
});

// 사용자가 올린 역경매 게시물 조회
app.get('/userRevAuctionPosts', async (req, res) => {
  try {
    const authorId = req.query.myObjectID;
    const posts = await db.all(
      "SELECT * FROM revAuctionPost WHERE authorId = ? ORDER BY date DESC",
      [authorId]
    );
    res.json(posts);
  } catch (e) {
    console.error('에러', e);
    res.status(500).send('서버 에러');
  }
});

// 사용자가 입찰한 역경매 게시물 조회
app.get('/userBidProducts', async (req, res) => {
  if (!req.user) return res.json([]);
  try {
    const bidderId = req.user.id;
    const results = await db.all(
      `SELECT r.* FROM revAuctionPost r
       JOIN revAuctionBids b ON r.id = b.postId
       WHERE b.bidderId = ?
       ORDER BY r.date DESC`,
      [bidderId]
    );
    res.json(results);
  } catch (e) {
    console.error('에러', e);
    res.status(500).send('서버 에러');
  }
});
//------------------------------------------------------------------------------------------------------------------------------------//

//------------------------------------------------------------경매 종료 확인----------------------------------------------------------//
// 경매 종료 확인
app.get('/checkRevAuctionStatus/:id', async (req, res) => {
  const postId = req.params.id;
  try {
    const post = await db.get("SELECT status FROM revAuctionPost WHERE id = ?", [postId]);
    const closed = post && (post.status === 'close' || post.status === 'awarded');
    res.json({ isClosedOrAwarded: closed });
  } catch (e) {
    console.error('경매 종료 처리 실패:', e);
    res.status(500).send('서버 에러');
  }
});

// 인증 메일 전송
app.post('/verify-email', (req, res) => {
  const { email } = req.body;
  const code = generateRandomCode(6);
  req.session.emailCode = code;
  req.session.email = email;
  transporter.sendMail({
    from: `[E A S Y] <${process.env.EMAIL_USERNAME}>`,
    to: email,
    subject: '[E A S Y] 인증번호를 확인해주세요.',
    html: `<h1>이메일 인증</h1><div>인증번호 [${code}]를 입력해주세요.</div>`
  }, (err, info) => {
    if (err) {
      console.error(err);
      res.status(500).json({ success: false, message: '이메일 전송 실패' });
    } else {
      res.json({ success: true, message: '이메일 전송 완료' });
    }
  });
});

// 인증 코드 확인
app.post('/verify-email-code', (req, res) => {
  const { email, emailCode } = req.body;
  const ok = req.session.email === email && req.session.emailCode === emailCode;
  res.json({ success: ok, message: ok ? '인증 성공' : '인증 실패' });
});

// 이메일 중복 체크
app.post('/check-email', async (req, res) => {
  const { email } = req.body;
  try {
    const user = await db.get("SELECT * FROM user WHERE email = ?", [email]);
    res.json({ available: !user });
  } catch (e) {
    console.error('이메일 확인 오류:', e);
    res.status(500).json({ message: '서버 에러' });
  }
});

// 채팅 API (SQLite 저장 필요시 테이블 및 로직 추가)
app.get('/chat/request', (req, res) => {
  // 필요 시 SQLite로 로직 전환
  res.status(501).send('Not Implemented');
});

io.on('connection', socket => {
  console.log('소켓 연결');
  socket.on('req_join', data => { /* 구현 유지 */ });
  socket.on('send_message', data => { /* 구현 유지 */ });
});

app.get('/chat/rooms', (req, res) => {
  res.status(501).send('Not Implemented');
});

app.get('/user/:userId', async (req, res) => {
  const id = req.params.userId;
  try {
    const user = await db.get("SELECT username FROM user WHERE id = ?", [id]);
    if (user) res.json({ username: user.username });
    else res.status(404).send('사용자 없음');
  } catch (e) {
    console.error('서버 에러:', e);
    res.status(500).send('서버 에러');
  }
});

// React 앱 build 결과물 정적 서빙
const buildPath = path.resolve(__dirname, 'front', 'build');
app.use(express.static(buildPath));

// 위의 API 경로 외 모든 GET 요청에 대해 React index.html 반환
app.get('*', (req, res) => {
  res.sendFile(path.join(buildPath, 'index.html'));
});

// WebSocket 메시징 예시 유지
io.on('connection', socket => console.log('소켓 연결'));


//------------------------------------------------------------------------------------------------------------------------------------//