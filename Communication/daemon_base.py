import os, sys, atexit, signal

class Daemon:
    def __init__(self, pid_file, stdin="/dev/null", stdout="/dev/null", stderr="/dev/null"):
        self.stdin = stdin
        self.stdout = stdout
        self.stderr = stderr
        self.pid_file = pid_file

    def daemonize(self):
        # 첫 번째 포크
        pid = os.fork()
        if pid > 0:
            sys.exit(0)
        os.setsid()
        os.umask(0)
        # 두 번째 포크
        pid = os.fork()
        if pid > 0:
            sys.exit(0)
        # 표준 입출력 리디렉션
        sys.stdout.flush()
        sys.stderr.flush()
        with open(self.stdin, 'r') as f:
            os.dup2(f.fileno(), sys.stdin.fileno())
        with open(self.stdout, 'a+') as f:
            os.dup2(f.fileno(), sys.stdout.fileno())
        with open(self.stderr, 'a+') as f:
            os.dup2(f.fileno(), sys.stderr.fileno())
        # PID 파일 생성
        atexit.register(self.del_pid)
        with open(self.pid_file, 'w+') as f:
            f.write(str(os.getpid()))

    def del_pid(self):
        if os.path.exists(self.pid_file):
            os.remove(self.pid_file)

    def get_pid(self):
        try:
            with open(self.pid_file) as f:
                return int(f.read().strip())
        except:
            return None

    def start(self):
        pid = self.get_pid()
        if pid:
            print(f"이미 실행 중 (PID: {pid})")
            return
        self.daemonize()
        self.run()

    def stop(self):
        pid = self.get_pid()
        if not pid:
            print("실행 중이지 않음")
            return
        try:
            os.kill(pid, signal.SIGTERM)
        except OSError as e:
            print(f"종료 실패: {e}")
        self.del_pid()

    def status(self):
        pid = self.get_pid()
        if pid:
            try:
                os.kill(pid, 0)
                print(f"실행 중 (PID: {pid})")
            except OSError:
                print("PID 파일은 있으나 프로세스 없음")
        else:
            print("실행 중이지 않음")

    def restart(self):
        self.stop()
        self.start()

    def run(self):
        raise NotImplementedError