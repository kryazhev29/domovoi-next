# Доработка: Кряжев А., 2026
# Запуск ELF-стендов на модели clang: флеш модели 256 КиБ задаётся здесь, минуя select_image.
# Аргументы — имена случаев firmware_suite (по умолчанию CORE). Мутации и стенды IAR-кода не проходят.
import os, sys, json, pathlib
S = pathlib.Path(__file__).resolve().parent
B = pathlib.Path(os.environ.get('DOMOVOI_COMPARE_BUILD', str(S/'build')))
os.environ.update(DOMOVOI_COMPARE_BUILD=str(B), DOMOVOI_ELF=str(B/'Release/ExePath/DomovoiNext.out'),
                  DOMOVOI_BIN=str(B/'Release/ExePath/DomovoiNext.bin'), DOMOVOI_MAP=str(B/'Release/ListPath/DomovoiNext.map'),
                  DOMOVOI_MODEL_FLASH_BYTES='0x40000')
R = pathlib.Path(os.environ.get('DOMOVOI_ROOT', S.parents[1]))
sys.path.insert(0, str(R/'Tests/asm')); sys.path.insert(0, str(R/'Tests'))
os.chdir(R)
import firmware_suite
cases = sys.argv[1:] or list(firmware_suite.CORE)
results = firmware_suite.run_cases(cases)
bad = [c for c, r in results.items() if r['status'] != 'PASS']
print('итог:', len(results) - len(bad), 'PASS,', len(bad), 'FAIL', ('— ' + ', '.join(bad)) if bad else '')
json.dump(results, open(B/'cases.json', 'w'), ensure_ascii=False, indent=1)
