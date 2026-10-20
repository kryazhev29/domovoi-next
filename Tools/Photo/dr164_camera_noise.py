"""Мост один на USB–485: 115200 против трафика камеры 57600/38400.

Меняет только UART моста с 460800 на 115200 и перезапускает его.
Сокеты, точку доступа, плату и сервер не настраивает. Посылки и окно
наблюдения повторяют m6/m7 стенда dr164_timing.py. Один адаптер видит
ответы в паузах; коллизии внутри передачи и точное число +ERR не измеряет.
"""
import argparse
import json
from pathlib import Path
import random
import time

import serial


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="COM14")
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    records = []
    started = time.perf_counter()
    log = (args.out / "serial.jsonl").open("x", encoding="utf-8")

    def record(kind, **values):
        entry = dict(t=round(time.perf_counter() - started, 6), kind=kind, **values)
        log.write(json.dumps(entry, ensure_ascii=False) + "\n")
        log.flush()
        return entry

    with log, serial.Serial(args.port, 460800, timeout=.005, write_timeout=2) as port:
        def listen(seconds):
            received = bytearray()
            first = None
            until = time.perf_counter() + seconds
            while time.perf_counter() < until:
                data = port.read(1)
                if data:
                    if first is None:
                        first = time.perf_counter()
                    received.extend(data + port.read(port.in_waiting))
            return bytes(received), first

        def send(data):
            # flush() CP210x задерживает около секунды: ждём расчётный конец.
            end = time.perf_counter() + len(data) * 10 / port.baudrate
            port.write(data)
            while time.perf_counter() < end:
                time.sleep(.0001)
            return end

        def at(command):
            port.reset_input_buffer()
            send(command.encode("ascii") + b"\r")
            data, _ = listen(.25)
            record("at", baud=port.baudrate, command=command, rx=data.hex())
            return data

        # Читать обе допустимые исходные скорости; запись только после ответа.
        initial = None
        for baud in (460800, 115200):
            port.baudrate = baud
            at("")
            if f"+ok={baud},".encode() in at("AT+UART"):
                initial = baud
                break
        if initial is None:
            raise RuntimeError("Мост не ответил на 460800 и 115200")
        if b"+ok=cmd" not in at("AT+TMODE"):
            raise RuntimeError("Нужен исходный командный режим")
        socket_before = at("AT+SOCKB")
        uarttm = at("AT+UARTTM")
        if initial != 115200:
            if b"+ok" not in at("AT+UART=115200,8,1,NONE,NFC"):
                raise RuntimeError("Смена скорости не подтверждена")
            at("AT+Z")
            port.baudrate = 115200
            for _ in range(24):
                time.sleep(.5)
                if b"+ok=115200," in at("AT+UART"):
                    break
            else:
                raise RuntimeError("Мост после перезапуска не найден")
        at("AT+E=off")
        print("Мост 115200, командный режим; начинаю посылки.", flush=True)
        try:
            for baud in (115200, 57600, 38400):
                port.baudrate = 115200
                at("")
                rng = random.Random(7)
                rich = [13, 10] * 20 + list(range(256))
                fixed = {
                    "SYNC": bytes.fromhex("AA0D00000000"),
                    "ACK0D": bytes.fromhex("AA0E0D000000"),
                    "ACK": bytes.fromhex("AA0E00000500"),
                    "R90": bytes(rng.choice(rich) for _ in range(90)),
                    "R512": bytes(rng.choice(rich) for _ in range(512)),
                    "Z512": bytes(512), "F512": b"\xff" * 512,
                }
                port.baudrate = baud
                port.reset_input_buffer()
                rounds = 5 if baud == 115200 else 40
                counts = {}
                for rep in range(rounds):
                    blocks = list(fixed.items()) + [
                        ("RND512", bytes(rng.choice(rich) for _ in range(512)))]
                    for name, block in blocks:
                        end = send(block)
                        data, first = listen(.05)
                        counts[name] = counts.get(name, 0) + bool(data)
                        records.append(record(
                            "probe", baud=baud, rep=rep, name=name, tx=block.hex(),
                            rx=data.hex(), delay_ms=None if first is None else
                            round((first - end) * 1000, 3)))
                print(f"Камера {baud}: {counts}, по {rounds} посылок.", flush=True)
        finally:
            port.baudrate = 115200
            at("")
            final_uart = at("AT+UART")
            final_mode = at("AT+TMODE")
            final_socket = at("AT+SOCKB")
            final_uarttm = at("AT+UARTTM")
            if (b"+ok=115200," not in final_uart or b"+ok=cmd" not in final_mode
                    or final_socket != socket_before or final_uarttm != uarttm):
                raise RuntimeError("Итоговое состояние моста не подтверждено")
        summary = {}
        for probe in records:
            group = summary.setdefault(str(probe["baud"]), {})
            stat = group.setdefault(probe["name"], dict(sent=0, replies=0, delays_ms=[]))
            stat["sent"] += 1
            stat["replies"] += bool(probe["rx"])
            if probe["rx"]:
                stat["delays_ms"].append(probe["delay_ms"])
        (args.out / "summary.json").write_text(
            json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        print("Проверено: UART=115200, TMODE=cmd, SOCKB и UARTTM сохранены.", flush=True)


if __name__ == "__main__":
    main()
