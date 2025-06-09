# setup.py
import sys
from cx_Freeze import setup, Executable

# 공통 옵션
build_exe_options = {
    "packages": ["socket", "json", "time", "sqlite3", "logging", "threading", "os", "sys", "collections"],
    "include_files": [
        ("config\\config.json", "config\\config.json"),
        ("daemon_base.py", "daemon_base.py")
    ]
}

# Python 인터프리터 경로, 버전에 맞춰 조정
base = None
if sys.platform == "win32":
    base = "Console"

# 두 개의 실행 파일 정의
executables = [
    Executable("server.py", base=base, target_name="delivery_server"),
    Executable("client.py", base=base, target_name="delivery_client")
]

setup(
    name="DeliveryRobotComm",
    version="1.0",
    description="Delivery Robot UDP Comm Modules",
    options={"build_exe": build_exe_options},
    executables=executables
)
