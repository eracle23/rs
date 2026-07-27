#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Simulate UserManager password-lock / admin-lock rules against SQLite."""

from __future__ import annotations

import sqlite3
import tempfile
from datetime import datetime, timedelta
from pathlib import Path

MAX_FAILED = 5
LOCK_MINUTES = 15
LOCK_PASSWORD = 1
LOCK_ADMIN = 2


def init_db(path: Path) -> sqlite3.Connection:
    conn = sqlite3.connect(path)
    conn.execute(
        """
        CREATE TABLE users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            role TEXT NOT NULL DEFAULT 'user',
            full_name TEXT,
            email TEXT,
            created_at TEXT NOT NULL,
            last_login TEXT,
            is_active INTEGER NOT NULL DEFAULT 1,
            is_locked INTEGER NOT NULL DEFAULT 0,
            failed_attempts INTEGER NOT NULL DEFAULT 0,
            locked_at TEXT,
            lock_reason INTEGER NOT NULL DEFAULT 0
        )
        """
    )
    conn.execute(
        "INSERT INTO users (username, password_hash, created_at) VALUES (?, ?, ?)",
        ("tester", "hash_ok", datetime.now().isoformat()),
    )
    conn.commit()
    return conn


def lock_password(conn: sqlite3.Connection, user_id: int = 1) -> None:
    now = datetime.now().isoformat()
    conn.execute(
        "UPDATE users SET is_locked=1, failed_attempts=?, locked_at=?, lock_reason=? WHERE id=?",
        (MAX_FAILED, now, LOCK_PASSWORD, user_id),
    )
    conn.commit()


def lock_admin(conn: sqlite3.Connection, user_id: int = 1) -> None:
    now = datetime.now().isoformat()
    conn.execute(
        "UPDATE users SET is_locked=1, locked_at=?, lock_reason=? WHERE id=?",
        (now, LOCK_ADMIN, user_id),
    )
    conn.commit()


def can_login_password_lock(locked_at: str) -> bool:
    unlock_at = datetime.fromisoformat(locked_at) + timedelta(minutes=LOCK_MINUTES)
    return datetime.now() >= unlock_at


def main() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        db_path = Path(tmp) / "users.db"
        conn = init_db(db_path)

        # 1) password lock within 15 minutes -> blocked
        lock_password(conn)
        row = conn.execute(
            "SELECT is_locked, lock_reason, locked_at FROM users WHERE id=1"
        ).fetchone()
        assert row[0] == 1 and row[1] == LOCK_PASSWORD
        assert not can_login_password_lock(row[2]), "password lock should block before 15 min"
        print("OK: password lock blocks within 15 minutes")

        # 2) password lock after 15 minutes -> auto unlock allowed
        old = (datetime.now() - timedelta(minutes=LOCK_MINUTES + 1)).isoformat()
        conn.execute(
            "UPDATE users SET locked_at=? WHERE id=1",
            (old,),
        )
        conn.commit()
        row = conn.execute(
            "SELECT locked_at FROM users WHERE id=1"
        ).fetchone()
        assert can_login_password_lock(row[0]), "password lock should expire after 15 min"
        print("OK: password lock auto-unlock after 15 minutes")

        # 3) admin lock does not auto unlock even if locked_at is old
        lock_admin(conn)
        conn.execute(
            "UPDATE users SET locked_at=? WHERE id=1",
            ((datetime.now() - timedelta(hours=2)).isoformat(),),
        )
        conn.commit()
        row = conn.execute(
            "SELECT is_locked, lock_reason FROM users WHERE id=1"
        ).fetchone()
        assert row == (1, LOCK_ADMIN)
        print("OK: admin lock remains regardless of locked_at age")

        # 4) admin unlock clears all lock fields
        conn.execute(
            "UPDATE users SET is_locked=0, failed_attempts=0, locked_at=NULL, lock_reason=0 WHERE id=1"
        )
        conn.commit()
        row = conn.execute(
            "SELECT is_locked, failed_attempts, locked_at, lock_reason FROM users WHERE id=1"
        ).fetchone()
        assert row == (0, 0, None, 0)
        print("OK: unlock clears lock state")

        conn.close()

    print("All lock-path checks passed.")


if __name__ == "__main__":
    main()
