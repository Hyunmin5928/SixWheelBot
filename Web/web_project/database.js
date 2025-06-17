// database.js
const sqlite3 = require('sqlite3').verbose();
const { open } = require('sqlite');
const path = require('path');

// delivery.db 파일 경로 설정
const dbPath = path.resolve(__dirname, 'delivery.db');

/**
 * 프로미스 기반 SQLite 데이터베이스 초기화
 * open()을 활용해 Promise<Database> 반환
 */
async function initDB() {
  // sqlite3 드라이버 사용
  const db = await open({
    filename: dbPath,
    driver: sqlite3.Database
  });

  // 외래키 활성화 (옵션)
  await db.exec('PRAGMA foreign_keys = ON;');

  // 테이블 생성
  await db.exec(`
    CREATE TABLE IF NOT EXISTS MEMBER (
      MEM_NUM      INTEGER PRIMARY KEY AUTOINCREMENT,
      MEM_ID       TEXT    UNIQUE   NOT NULL,
      MEM_PW       TEXT             NOT NULL,
      MEM_NAME     TEXT    UNIQUE   NOT NULL,
      MEM_ZIP      TEXT,
      MEM_ADD1     TEXT,
      MEM_ADD2     TEXT,
      MEM_PHONE    TEXT             NOT NULL,
      MEM_EMAIL    TEXT    UNIQUE   NOT NULL,
      MEM_JOINDATE DATETIME         NOT NULL DEFAULT (CURRENT_TIMESTAMP),
      MEM_ADMIN    TEXT             NOT NULL DEFAULT 'N'
    );
    CREATE TABLE IF NOT EXISTS ORDERS (
      ORDERS_NUM     INTEGER PRIMARY KEY AUTOINCREMENT,
      ORDERS_USER    INTEGER       NOT NULL,
      ORDERS_PHONE   TEXT          NOT NULL,
      ORDERS_STATUS  TEXT          NOT NULL,
      ORDERS_DATE    DATETIME      NOT NULL DEFAULT (CURRENT_TIMESTAMP),
      ORDERS_COST    INTEGER       NOT NULL,
      ORDERS_ZIPCODE TEXT          NOT NULL,
      ORDERS_ADD1    TEXT          NOT NULL,
      ORDERS_ADD2    TEXT          NOT NULL,
      ORDERS_MEMO    TEXT,
      ORDERS_DEL     TEXT          NOT NULL DEFAULT 'N',
      RETURN_STATE   TEXT          NOT NULL DEFAULT 'Y',
      RETURN_DATE    DATETIME,
      RETURN_STATUS  TEXT,
      RETURN_COST    INTEGER,
      CHECK(ORDERS_DEL IN ('Y','N')),
      CHECK(RETURN_STATE IN ('Y','N'))
    );
    CREATE TABLE IF NOT EXISTS DELIVERY (
      DELIVERY_NUM  INTEGER PRIMARY KEY AUTOINCREMENT,
      DELIVERY_CNT  INTEGER       NOT NULL DEFAULT 0,
      SUCCESS_CNT   INTEGER       NOT NULL DEFAULT 0,
      FAIL_CNT      INTEGER       NOT NULL DEFAULT 0,
      UPDATE_DATE   DATETIME      NOT NULL DEFAULT (CURRENT_TIMESTAMP)
    );
  `);

  console.log('SQLite 연결 성공:', dbPath);
  return db;
}

// Promise<Database> 객체를 모듈로 내보냄
module.exports = initDB();
