// server.js
const express           = require("express");
const requestIp         = require('request-ip');
const cors              = require("cors");
const bcrypt            = require("bcrypt");
const session           = require("express-session");
const passport          = require("passport");
const LocalStrategy     = require("passport-local").Strategy;
const { createServer }  = require('http');
const { Server }        = require('socket.io');
const path              = require("path");
const axios             = require("axios");
const dgram             = require('dgram');
const dbPromise         = require('./database.js');

const app = express();
app.use(cors());
app.use(express.json());
app.use(express.urlencoded({ extended: true }));
app.use(requestIp.mw());

const liveCoord = {};  // { orderId: { lat, lng } }

const coordUdpSock = dgram.createSocket('udp4');
coordUdpSock.on('message', (msg) => {
  try {
    const data = JSON.parse(msg.toString());
    // orderId 같이 포함되면 좋지만 없으면 마지막 값 하나만 유지
    liveCoord.latest = { lat: data.lat, lng: data.lng };
  } catch (e) {
    console.error('좌표 파싱 에러:', e);
  }
});
coordUdpSock.bind(6002); // Python이 보내는 포트

// ── SSE: 좌표 스트리밍 endpoint ───────────────────────
app.get('/api/v1/orders/:id/coords/stream', (req, res) => {
  res.set({
    'Content-Type': 'text/event-stream',
    'Cache-Control': 'no-cache',
    'Connection': 'keep-alive'
  });
  res.flushHeaders();
  // 예제: 2초마다 테스트 좌표
  const timer = setInterval(() => {
    if (liveCoord.latest) {
      res.write(`data: ${JSON.stringify(liveCoord.latest)}\n\n`);
    }
  }, 2000);
  req.on('close', () => clearInterval(timer));
});
app.get('/api/v1/returns/:id/coords/stream', (req, res) => {
  res.set({
    'Content-Type': 'text/event-stream',
    'Cache-Control': 'no-cache',
    'Connection': 'keep-alive'
  });
  res.flushHeaders();
  // 예제: 2초마다 테스트 좌표
  const timer = setInterval(() => {
    if (liveCoord.latest) {
      res.write(`data: ${JSON.stringify(liveCoord.latest)}\n\n`);
    }
  }, 2000);

  // const points = [
  //   { lat: 37.566826,  lng: 126.9786567 },   // 광화문
  //   { lat: 37.567400,  lng: 126.9791000 },   // 북서쪽
  //   { lat: 37.566300,  lng: 126.9798000 },   // 남서쪽
  //   { lat: 37.566900,  lng: 126.9778000 },   // 동쪽
  //   { lat: 37.567600,  lng: 126.9783000 },   // 북동쪽
  // ];
  // let idx = 0;

  // const timer = setInterval(() => {
  //   const { lat, lng } = points[idx];
  //   res.write(`data: {"lat":${lat},"lng":${lng}}\n\n`);
  //   idx = (idx + 1) % points.length;         // 0→4 순환
  // }, 1000);

  req.on('close', () => clearInterval(timer));
});

// ── 세션·Passport 설정 ─────────────────────────────────
app.use(session({
  secret: process.env.SESSION_SECRET || 'secret',
  resave: false,
  saveUninitialized: false
}));
app.use(passport.initialize());
app.use(passport.session());

// React build 파일 서빙
const buildPath = path.resolve(__dirname, 'front', 'build');
app.use(express.static(buildPath));

// ── UDP 제어 메시지 전송 함수 (server.py 제어) ─────────
const PYTHON_IP        = '127.0.0.1';
const PYTHON_CTRL_PORT = 6001;
function sendControl(type, payload = {}) {
  const client = dgram.createSocket('udp4');
  const msg    = Buffer.from(JSON.stringify({ type, ...payload }));
  client.send(msg, PYTHON_CTRL_PORT, PYTHON_IP, err => {
    if (err) console.error('Control UDP 전송 에러:', err);
    client.close();
  });
}

// ── Passport LocalStrategy ────────────────────────────
passport.use(new LocalStrategy({
    usernameField: 'userId',
    passwordField: 'password'
  },
  async (userId, password, done) => {
    try {
      const db  = await dbPromise;
      const row = await db.get(`
        SELECT MEM_NUM, MEM_ID, MEM_PW, MEM_NAME, MEM_EMAIL, MEM_ADMIN
         FROM MEMBER
        WHERE MEM_ID = ?`, [userId]
      );
      if (!row) return done(null, false, { message: '존재하지 않는 아이디입니다.' });
      const match = await bcrypt.compare(password, row.MEM_PW);
      if (!match) return done(null, false, { message: '비밀번호가 일치하지 않습니다.' });
      done(null, {
        memNum: row.MEM_NUM,
        userId: row.MEM_ID,
        name:   row.MEM_NAME,
        email:  row.MEM_EMAIL,
        role:   row.MEM_ADMIN === 'Y' ? 'ADMIN' : 'USER'
      });
    } catch (e) {
      done(e);
    }
  }
));
passport.serializeUser((user, done) => done(null, user.memNum));
passport.deserializeUser(async (memNum, done) => {
  try {
    const db  = await dbPromise;
    const row = await db.get(`
      SELECT MEM_NUM, MEM_ID, MEM_NAME, MEM_EMAIL, MEM_ADMIN
       FROM MEMBER
      WHERE MEM_NUM = ?`, [memNum]
    );
    if (!row) return done(null, false);
    done(null, {
      memNum: row.MEM_NUM,
      userId: row.MEM_ID,
      name:   row.MEM_NAME,
      email:  row.MEM_EMAIL,
      role:   row.MEM_ADMIN === 'Y' ? 'ADMIN' : 'USER'
    });
  } catch (e) {
    done(e);
  }
});

// ── 인증·회원가입 라우트 ─────────────────────────────────
app.post("/api/register", async (req, res) => {
  const { userId, password, name, zip, add1, add2, phone, email } = req.body;
  try {
    const db  = await dbPromise;
    const dup = await db.get(
      "SELECT 1 FROM MEMBER WHERE MEM_ID = ? OR MEM_EMAIL = ?",
      [userId, email]
    );
    if (dup) return res.status(400).send('이미 존재하는 아이디 또는 이메일입니다.');
    const hash = await bcrypt.hash(password, 10);
    await db.run(
      `INSERT INTO MEMBER
         (MEM_ID, MEM_PW, MEM_NAME, MEM_ZIP, MEM_ADD1, MEM_ADD2, MEM_PHONE, MEM_EMAIL)
       VALUES (?, ?, ?, ?, ?, ?, ?, ?)`,
      [userId, hash, name, zip, add1, add2, phone, email]
    );
    res.json({ registerSuccess: true });
  } catch (e) {
    console.error('회원가입 오류:', e);
    res.status(500).send('회원가입 오류');
  }
});
app.post('/api/login', (req, res, next) => {
  passport.authenticate('local', (err, user, info) => {
    if (err)   return res.status(500).json({ error: err.message });
    if (!user) return res.status(401).json({ error: info.message });
    req.logIn(user, e => {
      if (e) return next(e);
      res.json({
        loginSuccess: true,
        userId: user.userId,
        role:   user.role
      });
    });
  })(req, res, next);
});
app.get("/api/logout", (_req, res) => {
  _req.logout(() => res.json({ logoutSuccess: true }));
});
app.get("/api/checkLoggedIn", (_req, res) => {
  res.json({ isLoggedIn: !!_req.user });
});

// ── 아이디/비번 찾기, 재설정 ─────────────────────────────
app.post('/api/find-id', async (req, res) => {
  const { name, email } = req.body;
  try {
    const db  = await dbPromise;
    const row = await db.get(
      "SELECT MEM_ID FROM MEMBER WHERE MEM_NAME = ? AND MEM_EMAIL = ?",
      [name, email]
    );
    if (!row) return res.status(404).json({ message: '일치하는 사용자가 없습니다.' });
    res.json({ userId: row.MEM_ID });
  } catch (e) {
    console.error('아이디 찾기 오류:', e);
    res.status(500).send('서버 오류');
  }
});
app.post('/api/find-pw', async (req, res) => {
  const { userId, name, email } = req.body;
  try {
    const db  = await dbPromise;
    const row = await db.get(`
      SELECT 1 FROM MEMBER
       WHERE MEM_ID = ? AND MEM_NAME = ? AND MEM_EMAIL = ?`,
      [userId, name, email]
    );
    if (!row) return res.status(404).send();
    res.json({ ok: true });
  } catch (e) {
    console.error('비밀번호 찾기 오류:', e);
    res.status(500).send('서버 오류');
  }
});
app.post('/api/reset-pw', async (req, res) => {
  const { userId, newPassword } = req.body;
  if (!userId || !newPassword) return res.status(400).send('필수값 누락');
  try {
    const db   = await dbPromise;
    const hash = await bcrypt.hash(newPassword, 10);
    const { changes } = await db.run(
      "UPDATE MEMBER SET MEM_PW = ? WHERE MEM_ID = ?",
      [hash, userId]
    );
    if (changes === 0) return res.status(404).send('해당 아이디가 없습니다.');
    res.json({ resetSuccess: true });
  } catch (e) {
    console.error('비밀번호 재설정 오류:', e);
    res.status(500).send('서버 오류');
  }
});

// ── 회원 프로필 조회·수정 ─────────────────────────────────
app.get('/api/user/:id', async (req, res) => {
  try {
    const db  = await dbPromise;
    const row = await db.get(`
      SELECT
        MEM_ID   AS userId,
        MEM_PW   AS password,
        MEM_NAME AS name,
        MEM_ZIP  AS zip,
        MEM_ADD1 AS address,
        MEM_ADD2 AS detail,
        MEM_PHONE AS phone,
        MEM_EMAIL AS email
      FROM MEMBER
      WHERE MEM_ID = ?`, [req.params.id]
    );
    if (!row) return res.status(404).send('해당 회원이 없습니다.');
    res.json(row);
  } catch (e) {
    console.error('프로필 조회 오류:', e);
    res.status(500).send('서버 오류');
  }
});
app.put('/api/user/:id', async (req, res) => {
  const userId = req.params.id;
  const { password, name, zip, address, detail, phone, email } = req.body;
  try {
    const db   = await dbPromise;
    const hash = password ? await bcrypt.hash(password, 10) : null;
    const { changes } = await db.run(`
      UPDATE MEMBER SET
        MEM_PW   = COALESCE(?, MEM_PW),
        MEM_NAME = ?,
        MEM_ZIP  = ?,
        MEM_ADD1 = ?,
        MEM_ADD2 = ?,
        MEM_PHONE= ?,
        MEM_EMAIL= ?
      WHERE MEM_ID = ?`,
      [hash, name, zip, address, detail, phone, email, userId]
    );
    if (changes === 0) return res.status(404).send('해당 회원이 없습니다.');
    res.json({ updateSuccess: true });
  } catch (e) {
    console.error('프로필 수정 오류:', e);
    res.status(500).send('서버 오류');
  }
});

// ── 배송 / 반품 API ─────────────────────────────────────
//  POST /api/order      { userId, receiver:{…}, itemType }  
//  GET  /api/order      pending 목록
//  POST /api/order/:id/accept    → ACCEPTED + 경로 계산 + sendControl('start')
//  POST /api/order/:id/unlock    → sendControl('unlock')
//  POST /api/order/:id/complete  → sendControl('return')
// 반품도 동일 구조
app.post('/api/order', async (req, res) => {
  const { userId, receiver, itemType } = req.body;
  if (!userId || !receiver?.address || !itemType) return res.status(400).send('값 누락');
  try {
    const db = await dbPromise;
    const { lastID } = await db.run(
      `INSERT INTO ORDER_REQ (MEM_ID, REC_ADDR, REC_DETAIL, ITEM_TYPE)
       VALUES (?, ?, ?, ?)`,
      [userId, receiver.address, receiver.detail, itemType]
    );
    res.json({ id: lastID, ok: true });
  } catch (e) {
    console.error('배송 신청 오류', e);
    res.status(500).send('서버 오류');
  }
});
// 회원별/전체 조회 모두 지원 --------------------
 app.get('/api/order', async (req, res) => {
   try {
     const { userId } = req.query;          // 예: /api/order?userId=ymh
     const db = await dbPromise;

     let sql = `
       SELECT  ORD_ID  AS id,
               MEM_ID  AS userId,
               (REC_ADDR || ' ' || IFNULL(REC_DETAIL,'')) AS address,
               ITEM_TYPE       AS itemType,
               STATUS          AS status
         FROM  ORDER_REQ`;

     const params = [];
     if (userId) {          // 회원별 필터
       sql += ' WHERE MEM_ID = ?';
       params.push(userId);
     }

     sql += ' ORDER BY REQ_TIME DESC';
     const rows = await db.all(sql, params);
     res.json(rows);
   } catch (e) {
     console.error('order list 오류', e);
     res.status(500).send('서버 오류');
   }
 });

/* ── 배송 요청 수락 ─────────────────────── */
const TMAP_API_KEY = '6uHPB650j41F9NmAfTKjs5DxEZ0eBcTC77dm55iX';
app.post('/api/order/:id/accept', async (req, res) => {
  try {
    const db = await dbPromise;
    const { changes } = await db.run(
      `UPDATE ORDER_REQ SET STATUS='ACCEPTED' WHERE ORD_ID = ?`,
      [req.params.id]
    );
    if (changes === 0) return res.status(404).send('해당 주문이 없습니다.');
    
    //주문 정보 조회
    const order = await db.get(`
    SELECT REC_ADDR, REC_DETAIL FROM ORDER_REQ WHERE ORD_ID = ?
    `, [req.params.id]);
    const TMAP_API_KEY = '6uHPB650j41F9NmAfTKjs5DxEZ0eBcTC77dm55iX';
    //주소-> 좌표 API 호출

    // 좌표 -> 경로 API 호출
    const address = order.REC_ADDR + ' ' + (order.REC_DETAIL || '');
    const [city_do, gu_gun, dong, bunji, ...detail] = address.split(' ');
    const detailAddress = detail.join(' ');  // 나머지는 상세주소로

    // 경기도 분당구 성남대로43번길 1
    // 경기도 성남시 분당구 구미동 158
    // 경기 성남시 분당구 구미동 181
    // city_do = "경기 성남시";
    // gu_gun = "분당구";
    // dong = "";
    // bunji = "성남대로43번길";
    // detailAddress = "1"

    console.log('=== 파싱된 주소 정보 ===');
    console.log('city_do      :', city_do);
    console.log('gu_gun       :', gu_gun);
    console.log('dong         :', dong);
    console.log('bunji        :', bunji);
    console.log('detailAddress:', detailAddress);

 
    const geoRes = await axios.get('https://apis.openapi.sk.com/tmap/geo/geocoding', {
    params: {
      version: 1,
      city_do: city_do,
      gu_gun: gu_gun,
      dong: dong,
      bunji: bunji,
      //detailAddress: detailAddress,
      addressFlag: 'F00',       // 지번/도로명 자동
      coordType: 'WGS84GEO',
      appKey: TMAP_API_KEY
    }
  });

  console.log('=== Geocoding 응답 전체 ===');
  console.log(geoRes.data);



    const coord = geoRes.data.coordinateInfo;
    //const startX = 127.1090;
    //const startY = 37.3397;
    const endX = Number(coord.lon);
    const endY = Number(coord.lat);
   // const endX = 126.910656;
    //const endY = 37.557907;
    // X : 경도, Y : 위도 -> 시작 점 : 분당 메가 포스 몰(홈플러스 방향)
    const startX = 127.10756589999941;
    const startY = 37.340588499999804;
 

    // 디버깅용 콘솔 출력
    console.log('==== Geocoding 결과 ====');
    console.log('coord:', coord);
    console.log('endX(경도):', coord.lon);
    console.log('endY(위도):', coord.lat);

    // 2. T map API 호출 
    
    
      const tmapRes = await axios.post(
      'https://apis.openapi.sk.com/tmap/routes/pedestrian?version=1',
      {
        startX, startY, endX, endY,
        startName: "출발지",
        endName:"도착지",
        reqCoordType: "WGS84GEO",
        resCoordType: "WGS84GEO",
        // ...필요한 옵션 추가
      },
      {
        headers: {
          appKey: TMAP_API_KEY,
          "Content-Type": "application/json",
        }
      }
    );
      console.log('=== 경로 응답 전체 ===');
      console.log(tmapRes.data);
   


    // 1. 결과에서 features 배열 뽑기
    const features = tmapRes.data.features;

    // 2. 좌표 추출
    const coordMap = {}; // { 'lat,lon' : turnType }
    features.forEach(feature => {
      const { geometry, properties } = feature;
      if (geometry.type === "LineString") {
        geometry.coordinates.forEach(([lon, lat]) => {
          const key = `${lat},${lon}`;
          if (!(key in coordMap)) {
            coordMap[key] = null; // 기본값
          }
        });
      } else if (geometry.type === "Point") {
        const [lon, lat] = geometry.coordinates;
        const turnType = properties.turnType;
        const key = `${lat},${lon}`;
        coordMap[key] = turnType; // turnType 등록
      }
    });

    // 3. (lat, lon, turnType) 배열 만들기
    const routeCoords = Object.entries(coordMap).map(([key, turnType]) => {
      const [lat, lon] = key.split(',').map(Number);
      return [lat, lon, turnType];
    });

    // 디버깅용 터미널 출력
    console.log('==== 경로 좌표 배열 (lat, lon, turnType) ====');
    routeCoords.forEach((item, idx) => {
      const [lat, lon, turnType] = item;
      console.log(`${idx + 1}: lat=${lat}, lon=${lon}, turnType=${turnType === null ? '없음' : turnType}`);
    });

    const fs = require('fs');

    // 파일로 저장 (예: route.txt)
    fs.writeFileSync(
      'route.txt',
      routeCoords.map(([lat, lon, turnType]) =>
        `${lat},${lon},${turnType === null ? '' : turnType}`
      ).join('\n'),
      { encoding: 'utf8' }
    );


    res.json({ ok: true });
    sendControl('start', { order_id: req.params.id });
  } catch (e) {
    console.error('order accept 오류', e);
    res.status(500).send('서버 오류');
  }
});

app.post('/api/order/:id/unlock', (_req, res) => {
  sendControl('unlock', { order_id: _req.params.id });
  res.json({ ok: true });
});
app.post('/api/order/:id/complete', (_req, res) => {
  sendControl('return', { order_id: _req.params.id });
  res.json({ ok: true });
});

// ── 반품 ───────────────────────────────────────────────
app.post('/api/return', async (req, res) => {
  const { userId, sender, itemType } = req.body;
  if (!userId || !sender?.address || !itemType) return res.status(400).send('값 누락');
  try {
    const db = await dbPromise;
    const { lastID } = await db.run(
      `INSERT INTO RETURN_REQ (MEM_ID, SEND_ADDR, SEND_DETAIL, ITEM_TYPE)
       VALUES (?, ?, ?, ?)`,
      [userId, sender.address, sender.detail, itemType]
    );
    res.json({ id: lastID, ok: true });
  } catch (e) {
    console.error('반품 신청 오류', e);
    res.status(500).send('서버 오류');
  }
});
app.get('/api/return', async (req, res) => {
  try {
    const { userId } = req.query;          // 예) /api/return?userId=ymh
    const db   = await dbPromise;

    let sql = `
      SELECT  RET_ID AS id,
              MEM_ID AS userId,
              (SEND_ADDR || ' ' || IFNULL(SEND_DETAIL,'')) AS address,
              ITEM_TYPE AS itemType,
              STATUS AS status,
              REQ_TIME
        FROM  RETURN_REQ`;

    const params = [];
    if (userId) {
      sql += ' WHERE MEM_ID = ?';
      params.push(userId);
    }
    sql += ' ORDER BY REQ_TIME DESC';

    const rows = await db.all(sql, params);
    res.json(rows);
  } catch (e) {
    console.error('return list 오류', e);
    res.status(500).send('서버 오류');
  }
});
app.post('/api/return/:id/accept', async (req, res) => {
  try {
    const db = await dbPromise;
    const { changes } = await db.run(
      `UPDATE RETURN_REQ SET STATUS='ACCEPTED' WHERE RET_ID = ?`,
      [req.params.id]
    );
    if (changes === 0) return res.status(404).send('해당 반품이 없습니다.');
    sendControl('start', { return_id: req.params.id });
    res.json({ ok: true });
  } catch (e) {
    console.error('return accept 오류', e);
    res.status(500).send('서버 오류');
  }
});

// ── 반품 요청 정지 ───────────────────────────────────
app.post('/api/return/:id/unlock', (req, res) => {
  // 여기서는 필요에 따라 dgram 으로 로봇에게 "정지" 신호를 보냅니다.
  sendControl('unlock', { order_id: req.params.id });
  res.json({ ok: true });
});

// ── 반품 요청 복귀 ───────────────────────────────────
app.post('/api/return/:id/complete', (req, res) => {
  // 다시 출발지로 돌아가라는 신호
  sendControl('return', { order_id: req.params.id });
  res.json({ ok: true });
});

// ── 주문 상세 + 잠금 ───────────────────────────────────                                     

/* GET /api/order/:id  ------------------------------------ */
app.get('/api/order/:id', async (req, res) => {
  try {
    const db  = await dbPromise;
    const row = await db.get(`
      SELECT ORD_ID AS id,
             MEM_ID AS userId,
             (REC_ADDR || ' ' || IFNULL(REC_DETAIL,'')) AS address,
             ITEM_TYPE AS itemType,
             STATUS AS status
        FROM ORDER_REQ
       WHERE ORD_ID = ?`, [req.params.id]);
    if (!row) return res.status(404).send('해당 주문이 없습니다.');
    res.json({
      id: row.id,
      userId: row.userId,
      receiver: { address: row.address },
      itemType: row.itemType,
      status: row.status
    });
  } catch (e) {
    console.error('주문 상세 조회 오류', e);
    res.status(500).send('서버 오류');
  }
});

/* POST /api/order/:id/lock  ------------------------------ */
app.post('/api/order/:id/lock', (req, res) => {
  // 실제 로봇 제어가 필요하면 sendControl('lock', …) 등 수행
  console.log(`🚩 잠금 요청 → 주문 ${req.params.id}`);
  res.json({ ok:true });
});

 // ── 반품 상세 조회 ───────────────────────────────────
app.get('/api/return/:id', async (req, res) => {
  try {
    const db = await dbPromise;
    const row = await db.get(`
      SELECT RET_ID AS id,
             MEM_ID AS userId,
             (SEND_ADDR || ' ' || IFNULL(SEND_DETAIL,'')) AS address,
             ITEM_TYPE AS itemType,
             STATUS AS status
        FROM RETURN_REQ
       WHERE RET_ID = ?`, [req.params.id]);
    if (!row) return res.status(404).send('해당 반품이 없습니다.');
    res.json({
      id: row.id,
      userId: row.userId,
      sender: { address: row.address },
      itemType: row.itemType,
      status: row.status
    });
  } catch (e) {
    console.error('반품 상세 조회 오류', e);
    res.status(500).send('서버 오류');
  }
});

/* POST /api/return/:id/lock  ─ 잠금 */
app.post('/api/return/:id/lock', (req, res) => {
  console.log(`🚩 잠금 요청 → 반품 ${req.params.id}`);
  res.json({ ok:true });
});

// SPA 라우팅
app.get('/', (_req, res) => res.redirect('/'));
app.get('*', (_req, res) => res.sendFile(path.join(buildPath, 'index.html')));

// 서버 시작
const server = createServer(app);
new Server(server, {
  cors: { origin: "http://localhost:3000", methods: ["GET","POST"] }
});
server.listen(process.env.PORT || 4000, () =>
  console.log("🚀 서버 실행 중: http://localhost:4000")
);
