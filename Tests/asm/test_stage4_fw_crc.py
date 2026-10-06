# Доработка: Кряжев А., 2026
"""Независимый побитовый эталон для пользователей аппаратной CRC в связанном коде."""
import random
from iar_irq_support import Machine


# Читает слово по фиксированному смещению состояния.
def word(crc, value):
    crc ^= value
    for _ in range(32):
        crc = ((crc << 1) ^ (0x04c11db7 if crc & 0x80000000 else 0)) & 0xffffffff
    return crc


# Проверяет основные сценарии модуля и возвращает результаты.
def run():
    m = Machine(); m.init(); rng = random.Random(41006)
    words = [0, 0xffffffff, 0x12345678] + [rng.getrandbits(32) for _ in range(512)]
    maximum = 0
    for length in (1, 2, 16, 256, len(words)):
        crc = m.call('fw_crc_begin'); assert crc == 0xffffffff
        for value in words[:length]:
            crc = word(crc, value)
            assert m.call('fw_crc_next', 0, value) == crc
            maximum = max(maximum, m.steps)
        # Новая задача CRC должна отбросить состояние регистра DR предыдущей задачи.
        assert m.call('fw_crc_begin') == 0xffffffff and m.crc_model.value == 0xffffffff
    return {'word_cases': sum((1, 2, 16, 256, len(words))), 'maximum_word_instructions': maximum, 'arbitrary_seed_restore': False}
