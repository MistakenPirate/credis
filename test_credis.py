import socket
import subprocess
import time
import sys

PORT = 6390

def send_command(*args):
    """Send a RESP command and return the response."""
    cmd = f"*{len(args)}\r\n"
    for arg in args:
        cmd += f"${len(arg)}\r\n{arg}\r\n"

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect(("127.0.0.1", PORT))
        s.sendall(cmd.encode())
        return s.recv(1024).decode().strip()

def test_ping():
    resp = send_command("PING")
    assert resp == "+PONG", f"Expected +PONG, got {resp}"
    print("✓ PING")

def test_set_get():
    resp = send_command("SET", "name", "alice")
    assert resp == "+OK", f"Expected +OK, got {resp}"

    resp = send_command("GET", "name")
    assert resp == "$5\r\nalice", f"Expected $5\\r\\nalice, got {resp}"
    print("✓ SET/GET")

def test_update_key():
    send_command("SET", "counter", "1")
    send_command("SET", "counter", "2")
    resp = send_command("GET", "counter")
    assert resp == "$1\r\n2", f"Expected $1\\r\\n2, got {resp}"
    print("✓ UPDATE existing key")

def test_get_missing():
    resp = send_command("GET", "nonexistent")
    assert resp == "$-1", f"Expected $-1, got {resp}"
    print("✓ GET missing key returns null")

def test_exists():
    send_command("SET", "exists_test", "value")
    resp = send_command("EXISTS", "exists_test")
    assert resp == ":1", f"Expected :1, got {resp}"

    resp = send_command("EXISTS", "does_not_exist")
    assert resp == ":0", f"Expected :0, got {resp}"
    print("✓ EXISTS")

def test_del():
    send_command("SET", "to_delete", "temp")
    resp = send_command("DEL", "to_delete")
    assert resp == ":1", f"Expected :1, got {resp}"

    resp = send_command("GET", "to_delete")
    assert resp == "$-1", f"Expected $-1, got {resp}"

    resp = send_command("DEL", "already_gone")
    assert resp == ":0", f"Expected :0, got {resp}"
    print("✓ DEL")

def test_unknown_command():
    resp = send_command("FOOBAR")
    assert resp.startswith("-ERR"), f"Expected error, got {resp}"
    print("✓ Unknown command returns error")

def main():
    # Start server
    print(f"Starting credis on port {PORT}...")
    server = subprocess.Popen(["./credis", "-p", str(PORT)],
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE)
    time.sleep(0.5)

    try:
        test_ping()
        test_set_get()
        test_update_key()
        test_get_missing()
        test_exists()
        test_del()
        test_unknown_command()
        print("\nAll tests passed!")
    except AssertionError as e:
        print(f"\n✗ Test failed: {e}")
        sys.exit(1)
    except ConnectionRefusedError:
        print("✗ Could not connect to server")
        sys.exit(1)
    finally:
        server.terminate()
        server.wait()

if __name__ == "__main__":
    main()
