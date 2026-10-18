#!/usr/bin/env python3
"""
can_flash.py - scan the CAN bus and flash a .bin via the CAN bootloader (PCAN-USB).

Uses PCANBasic.dll directly (ctypes), nothing to install except the PEAK driver.
Protocol: Project_Gateway/inc/can_id.h, bootloader: Core/Src/can_functions.c

  python can_flash.py scan
  python can_flash.py flash 20260011 [--no-start]   (flashes 20260011.bin, always via the bootloader)
  python can_flash.py shell 20260011                            (F4 closes the session)

The serial is hex, without 0x. For safety flash only accepts the file <serial>.bin
in the current directory (or --dir), so the file name always says which module it is for.

Bus speed 125 kbit/s (same as the FDCAN / bxCAN settings of the projects).
The .bin must contain the app_info header with CRC (WriteCRC.exe after the build),
the bootloader checks it before it jumps to the application.
"""
import argparse
import ctypes
import os
import struct
import sys
import time
try:
    import msvcrt
except ImportError:
    msvcrt = None

# can_id.h
ID_POLL, ID_POLL_RPY = 10, 11
ID_OPEN, ID_CLOSE, ID_OC_RES = 12, 13, 14
ID_ERASE, ID_START = 15, 16
ID_FL_DATA, ID_FL_RESULT = 17, 18
ID_ASK_CRC, ID_CRC = 19, 20
ID_BL_ACITVATE = 25
ID_SH_OPEN, ID_SH_CLOSE, ID_SH_RES, ID_SH_RX, ID_SH_TX = 30, 31, 32, 33, 34

APP_KEY_BYTES = bytes([0x55, 0xAA, 0x55, 0xAA])		# APP_KEY 0xAA55AA55, little endian

# PCANBasic
PCAN_USBBUS1 = 0x51
PCAN_BAUD_125K = 0x031C
PCAN_ERROR_OK = 0x00000
PCAN_ERROR_QRCVEMPTY = 0x00020
PCAN_MESSAGE_STANDARD = 0x00


class TPCANMsg(ctypes.Structure):
    _fields_ = [("ID", ctypes.c_uint32), ("MSGTYPE", ctypes.c_ubyte),
                ("LEN", ctypes.c_ubyte), ("DATA", ctypes.c_ubyte * 8)]


class TPCANTimestamp(ctypes.Structure):
    _fields_ = [("millis", ctypes.c_uint32), ("millis_overflow", ctypes.c_uint16),
                ("micros", ctypes.c_uint16)]


class Pcan:
    def __init__(self, channel=PCAN_USBBUS1, baud=PCAN_BAUD_125K):
        self.dll = ctypes.WinDLL("PCANBasic.dll")
        self.ch = channel
        res = self.dll.CAN_Initialize(channel, baud, 0, 0, 0)
        if res != PCAN_ERROR_OK:
            raise RuntimeError("CAN_Initialize failed, error 0x%X (PCAN-USB connected?)" % res)

    def close(self):
        self.dll.CAN_Uninitialize(self.ch)

    def flush(self):
        """Drop everything in the receive queue."""
        while self.recv(0) is not None:
            pass

    def send(self, can_id, data=b""):
        msg = TPCANMsg()
        msg.ID = can_id
        msg.MSGTYPE = PCAN_MESSAGE_STANDARD
        msg.LEN = len(data)
        for i, b in enumerate(data):
            msg.DATA[i] = b
        res = self.dll.CAN_Write(self.ch, ctypes.byref(msg))
        if res != PCAN_ERROR_OK:
            raise RuntimeError("CAN_Write failed, error 0x%X" % res)

    def recv(self, timeout):
        """Next frame (id, bytes) or None after timeout (s)."""
        end = time.time() + timeout
        msg, ts = TPCANMsg(), TPCANTimestamp()
        while True:
            res = self.dll.CAN_Read(self.ch, ctypes.byref(msg), ctypes.byref(ts))
            if res == PCAN_ERROR_OK:
                if msg.MSGTYPE & 0x02 or msg.MSGTYPE & 0x01:	# extended / RTR: ignore
                    continue
                return msg.ID, bytes(msg.DATA[:msg.LEN])
            if res != PCAN_ERROR_QRCVEMPTY:
                raise RuntimeError("CAN_Read failed, error 0x%X" % res)
            if time.time() >= end:
                return None
            time.sleep(0.0005)

    def wait_for(self, can_id, timeout, check=None):
        """Wait for a frame with this id (and check(data) true), others are skipped."""
        end = time.time() + timeout
        while True:
            left = end - time.time()
            if left <= 0:
                return None
            fr = self.recv(left)
            if fr is None:
                return None
            if fr[0] == can_id and (check is None or check(fr[1])):
                return fr[1]


def parse_serial(text):
    """Module number as hex, with or without 0x, e.g. 20260011."""
    try:
        return int(text, 16)
    except ValueError:
        raise RuntimeError("'%s' is not a hex module number" % text)


def ser_bytes(serial):
    return struct.pack("<I", serial)


# serial numbers of the known nodes, Project_Gateway/Inc/vars.h
KNOWN = {
    0x20180010: "Garage", 0x20180020: "Gateway", 0x20180066: "Test gateway",
    0x20190031: "Heliostaat", 0x20210010: "EV",
    0x20230010: "Trapled", 0x20230011: "WTW",
    0x20250010: "Enviorment", 0x20250011: "Powerwall", 0x20250012: "Velux",
    0x20260010: "TFT BG", 0x20260011: "TFT 1E",
    0xD9120300: "LCD BG", 0xD8120300: "LCD up", 0xD7120300: "Klok",
}


def cmd_scan(bus, args):
    # like the gateway 'canpoll': collect the answers for 2 s, poll twice (slow nodes)
    bus.flush()
    found = set()
    start = time.time()
    polls = [0.0, 1.0]
    while time.time() - start < args.window:
        if polls and time.time() - start >= polls[0]:
            polls.pop(0)
            bus.send(ID_POLL)
        fr = bus.recv(0.05)
        if fr and fr[0] == ID_POLL_RPY and len(fr[1]) == 4:
            found.add(struct.unpack("<I", fr[1])[0])
    for s in sorted(found):
        print("%08X %-3s %s" % (s, "ok" if s in KNOWN else "?", KNOWN.get(s, "?????")))
    for s, name in KNOWN.items():
        if s not in found:
            print("%08X ERR %s" % (s, name))
    return 0 if found else 1


def expect(res, what):
    if res is None:
        raise RuntimeError("%s: no answer" % what)
    return res


def cmd_flash(bus, args):
    serial = parse_serial(args.serial)
    sb = ser_bytes(serial)
    name = "%08X.bin" % serial
    path = os.path.join(args.dir, name)
    if not os.path.isfile(path):
        # accept a lower case file name as well
        alt = os.path.join(args.dir, name.lower())
        if os.path.isfile(alt):
            path = alt
        else:
            raise RuntimeError("file %s not found, the file name must be the module number (%s)" % (name, os.path.abspath(args.dir)))
    with open(path, "rb") as f:
        image = f.read()
    print("File %s" % path)
    if image[:4] != APP_KEY_BYTES:
        print("WARNING: no app_info key at the start of the file (was WriteCRC run?)")
    image += b"\xFF" * (-len(image) % 8)
    blocks = len(image) // 8
    print("Image %d bytes, %d frames" % (len(image), blocks))

    bus.flush()
    # always send the node into the bootloader first. An application resets and the bootloader
    # answers with ID_OC_RES; a node that already is in the bootloader answers directly.
    print("Activate bootloader...")
    bus.send(ID_BL_ACITVATE, sb)
    if bus.wait_for(ID_OC_RES, 3.0, lambda d: d == sb) is None:
        print("  no answer, trying to open the channel anyway")
    time.sleep(0.3)

    print("Open channel...")
    for attempt in range(3):
        bus.send(ID_OPEN, sb)
        if bus.wait_for(ID_OC_RES, 1.5, lambda d: d == sb) is not None:
            break
    else:
        raise RuntimeError("open: no answer (is module %08X on this bus?)" % serial)

    print("Erase (can take several seconds)...")
    bus.send(ID_ERASE, sb)
    res = expect(bus.wait_for(ID_FL_RESULT, 60.0), "erase")
    if res[0] != 0:
        raise RuntimeError("erase failed, result %d" % res[0])

    print("Write...")
    t0 = time.time()
    for i in range(blocks):
        bus.send(ID_FL_DATA, image[i * 8:i * 8 + 8])
        res = expect(bus.wait_for(ID_FL_RESULT, 2.0), "write frame %d" % i)
        if res[0] != 0:
            raise RuntimeError("write failed at offset 0x%X, result %d" % (i * 8, res[0]))
        if i % 256 == 0 or i == blocks - 1:
            print("\r  %3d%%  %d / %d bytes" % ((i + 1) * 100 // blocks, (i + 1) * 8, len(image)), end="", flush=True)
    print("\n  %.1f s" % (time.time() - t0))

    print("Verify CRC...")
    bus.send(ID_ASK_CRC, sb)
    res = expect(bus.wait_for(ID_CRC, 10.0, lambda d: len(d) == 8), "crc")
    calc, stored = struct.unpack(">II", res)
    print("  calculated 0x%08X  in file 0x%08X" % (calc, stored))
    if calc != stored:
        raise RuntimeError("CRC mismatch, NOT starting the application")

    if not args.no_start:
        print("Start application...")
        bus.send(ID_START, sb)
        res = expect(bus.wait_for(ID_FL_RESULT, 2.0), "start")
        if res[0] != 0:
            raise RuntimeError("start failed, result %d" % res[0])
    print("Done")
    return 0


# Windows console keys (second code after 0x00 / 0xE0) -> VT100 sequences
ESC = chr(27)
SPECIAL_KEYS = {"H": ESC + "[A", "P": ESC + "[B", "M": ESC + "[C", "K": ESC + "[D",
                "G": ESC + "[H", "O": ESC + "[F", "S": ESC + "[3~", "R": ESC + "[2~",
                "I": ESC + "[5~", "Q": ESC + "[6~"}
KEY_F4 = ">"	# second code of F4 (F1 = ';', F2 = '<', F3 = '=')


def enable_vt_output():
    """Let the Windows console interpret the colors / cursor codes of the node shell."""
    try:
        k32 = ctypes.windll.kernel32
        h = k32.GetStdHandle(-11)
        mode = ctypes.c_uint32()
        if k32.GetConsoleMode(h, ctypes.byref(mode)):
            k32.SetConsoleMode(h, mode.value | 0x0004)
    except (AttributeError, OSError):
        pass


def read_keys():
    """All pending keystrokes as bytes, or None when F4 was pressed."""
    out = bytearray()
    while msvcrt.kbhit():
        ch = msvcrt.getwch()
        if ord(ch) in (0x00, 0xE0):
            code = msvcrt.getwch()
            if code == KEY_F4:
                return None
            out += SPECIAL_KEYS.get(code, "").encode()
        else:
            out += ch.encode("cp1252", errors="replace")
    return bytes(out)


def cmd_shell(bus, args):
    if msvcrt is None:
        raise RuntimeError("shell needs a Windows console")
    serial = parse_serial(args.serial)
    sb = ser_bytes(serial)

    bus.flush()
    bus.send(ID_SH_OPEN, sb)
    expect(bus.wait_for(ID_SH_RES, 2.0, lambda d: d == sb), "open shell")

    enable_vt_output()
    print("[Shell 0x%08X open, F4 = close]" % serial)
    sys.stdout.flush()
    out = sys.stdout.buffer
    try:
        while True:
            keys = read_keys()
            if keys is None:
                break
            for i in range(0, len(keys), 8):
                bus.send(ID_SH_RX, keys[i:i + 8])
                time.sleep(0.002)

            fr = bus.recv(0.01)
            while fr is not None:
                if fr[0] == ID_SH_TX:
                    out.write(fr[1])
                    out.flush()
                fr = bus.recv(0)
    finally:
        bus.send(ID_SH_CLOSE, sb)
        res = bus.wait_for(ID_SH_RES, 1.0, lambda d: d == sb)
        print("[Shell closed]" if res else "[Shell close: no answer]")
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--channel", type=lambda x: int(x, 0), default=PCAN_USBBUS1, help="PCAN channel (default 0x51 = USBBUS1)")
    sub = ap.add_subparsers(dest="cmd", required=True)
    sc = sub.add_parser("scan", help="poll the bus and list the serial numbers")
    sc.add_argument("--window", type=float, default=2.5, help="seconds to collect answers (default 2.5)")
    fl = sub.add_parser("flash", help="flash a .bin")
    fl.add_argument("serial", help="module number (hex), e.g. 20260011; flashes <serial>.bin")
    fl.add_argument("--dir", default=".", help="directory with the .bin file (default: current)")
    fl.add_argument("--no-start", action="store_true", help="do not start the application afterwards")
    sh = sub.add_parser("shell", help="take over the UART shell of a node (F4 closes)")
    sh.add_argument("serial", help="module number (hex), e.g. 20250011")
    args = ap.parse_args()

    try:
        bus = Pcan(args.channel)
    except (OSError, RuntimeError) as e:
        print("ERROR:", e)
        return 2
    try:
        return {"scan": cmd_scan, "flash": cmd_flash, "shell": cmd_shell}[args.cmd](bus, args)
    except RuntimeError as e:
        print("\nERROR:", e)
        return 1
    finally:
        bus.close()


if __name__ == "__main__":
    sys.exit(main())
