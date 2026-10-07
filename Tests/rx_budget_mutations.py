"""Доработка: Кряжев А., 2026. Мутации настоящего ELF: CRC, RX, срок кадра и TCP."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent/'asm'))
from firmware_suite import select_image
import os
if 'DOMOVOI_ELF' not in os.environ: select_image()
import test_logic_cleanup as logic

original=logic.rx_machine
logic.rx_contracts()
for symbol in ('user_comm_rx','server_rx_process','server_rx_begin_pass','user_comm_session',
               'crc16_calc','user_comm_rx_capacity'):
    def mutated(symbol=symbol):
        machine=original()
        assert symbol in machine.symbols, symbol
        machine.stubs[machine.addr(symbol)]=lambda:machine.ret(0)
        return machine
    logic.rx_machine=mutated
    try:
        logic.rx_contracts()
    except AssertionError:
        print('DETECTED',symbol,flush=True)
    else:
        raise AssertionError(('survived',symbol))
    finally:
        logic.rx_machine=original
print('PASS RX mutations: 6/6')
